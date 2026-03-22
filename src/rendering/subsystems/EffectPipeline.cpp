/*
    Oscil - Effect Pipeline Implementation
*/

#include "rendering/subsystems/EffectPipeline.h"

#include <array>

#include "rendering/effects/BloomEffect.h"
#include "rendering/effects/ChromaticAberrationEffect.h"
#include "rendering/effects/ColorGradeEffect.h"
#include "rendering/effects/FilmGrainEffect.h"
#include "rendering/effects/RadialBlurEffect.h"
#include "rendering/effects/ScanlineEffect.h"
#include "rendering/effects/TiltShiftEffect.h"
#include "rendering/effects/TrailsEffect.h"
#include "rendering/effects/VignetteEffect.h"
#include "rendering/subsystems/RenderStats.h" // For logging macros

namespace oscil
{

EffectPipeline::EffectPipeline()
    : fbPool_(std::make_unique<FramebufferPool>())
    , sceneFBO_(std::make_unique<Framebuffer>())
{
}

EffectPipeline::~EffectPipeline()
{
    // Debug assertion to catch missing shutdown() calls
    jassert(context_ == nullptr && "EffectPipeline::shutdown() must be called before destruction");
}

bool EffectPipeline::initialize(juce::OpenGLContext& context, int width, int height)
{
    context_ = &context;

    // Initialize framebuffer pool
    if (!fbPool_->initialize(context, width, height))
    {
        RE_LOG("EffectPipeline: Failed to initialize framebuffer pool");
        return false;
    }

    // Create scene FBO
    if (!sceneFBO_->create(context, width, height, 0, juce::gl::GL_RGBA16F, false))
    {
        RE_LOG("EffectPipeline: Failed to create scene FBO");
        fbPool_->shutdown(context);
        return false;
    }

    initializeEffects();

    return true;
}

void EffectPipeline::shutdown(juce::OpenGLContext& context)
{
    releaseEffects();
    sceneFBO_->destroy(context);
    fbPool_->shutdown(context);
    context_ = nullptr;
}

void EffectPipeline::resize(juce::OpenGLContext& context, int width, int height)
{
    fbPool_->resize(context, width, height);
    sceneFBO_->resize(context, width, height);
}

void EffectPipeline::createEffectInstances()
{
    effects_["vignette"] = std::make_unique<VignetteEffect>();
    effects_["film_grain"] = std::make_unique<FilmGrainEffect>();
    effects_["bloom"] = std::make_unique<BloomEffect>();
    effects_["trails"] = std::make_unique<TrailsEffect>();
    effects_["tilt_shift"] = std::make_unique<TiltShiftEffect>();
    effects_["color_grade"] = std::make_unique<ColorGradeEffect>();
    effects_["chromatic_aberration"] = std::make_unique<ChromaticAberrationEffect>();
    effects_["scanlines"] = std::make_unique<ScanlineEffect>();
    effects_["radial_blur"] = std::make_unique<RadialBlurEffect>();
}

void EffectPipeline::buildEffectChain()
{
    effectChain_.clear();

    auto configureVirtual = [](PostProcessEffect* e, const VisualConfiguration& c) {
        e->configure(c);
    };

    struct ChainEntry { const char* id; bool (*isEnabled)(const VisualConfiguration&); };
    static constexpr auto entries = std::to_array<ChainEntry>({
        {"bloom",                [](const VisualConfiguration& c) { return c.bloom.enabled; }},
        {"radial_blur",          [](const VisualConfiguration& c) { return c.radialBlur.enabled; }},
        {"tilt_shift",           [](const VisualConfiguration& c) { return c.tiltShift.enabled; }},
        {"color_grade",          [](const VisualConfiguration& c) { return c.colorGrade.enabled; }},
        {"chromatic_aberration", [](const VisualConfiguration& c) { return c.chromaticAberration.enabled; }},
        {"scanlines",            [](const VisualConfiguration& c) { return c.scanlines.enabled; }},
        {"vignette",             [](const VisualConfiguration& c) { return c.vignette.enabled; }},
        {"film_grain",           [](const VisualConfiguration& c) { return c.filmGrain.enabled; }},
    });

    for (const auto& entry : entries)
        effectChain_.addStep({entry.id, entry.isEnabled, configureVirtual});
}

void EffectPipeline::initializeEffects()
{
    createEffectInstances();
    buildEffectChain();
}

void EffectPipeline::releaseEffects()
{
    if (context_ == nullptr)
    {
        effects_.clear();
        return;
    }

    for (auto& pair : effects_)
    {
        pair.second->release(*context_);
    }
    effects_.clear();
}

PostProcessEffect* EffectPipeline::getEffect(const juce::String& effectId)
{
    auto it = effects_.find(effectId);
    if (it != effects_.end())
    {
        auto* effect = it->second.get();
        // LAZY EFFECT COMPILATION
        if (effect && !effect->isCompiled() && context_)
        {
            if (effect->compile(*context_))
            {
                RE_LOG("Lazy-compiled effect: " << effectId.toStdString());
            }
            else
            {
                RE_LOG("Failed to lazy-compile effect: " << effectId.toStdString());
            }
        }
        return effect;
    }
    return nullptr;
}

Framebuffer* EffectPipeline::applyPostProcessing(Framebuffer* source, WaveformRenderState& state, juce::OpenGLContext& context, float deltaTime, juce::OpenGLShaderProgram* compositeShader, GLint compositeLoc)
{
    const auto& config = state.visualConfig;
    Framebuffer* current = source;

    // Step 1: Apply trails effect BEFORE the generic effect chain
    // Trails requires per-waveform history FBO from WaveformRenderState
    if (config.trails.enabled && state.trailsEnabled &&
        state.historyFBO && state.historyFBO->isValid())
    {
        auto* trailsEffect = dynamic_cast<TrailsEffect*>(getEffect("trails"));
        if (trailsEffect && trailsEffect->isCompiled())
        {
            trailsEffect->setSettings(config.trails);

            // Get a destination FBO from the pool
            Framebuffer* trailsDest = fbPool_->getPingFBO();
            if (trailsDest && trailsDest->isValid())
            {
                // Apply trails blending current frame with history
                trailsEffect->applyWithHistory(
                    context,
                    current,                // Current frame
                    state.historyFBO.get(), // Previous frame history
                    trailsDest,             // Output
                    *fbPool_,
                    deltaTime
                );

                // Copy the result back to historyFBO for the next frame
                // We need to pass compositeShader to applyPostProcessing or have EffectPipeline hold a reference to Bootstrapper.
                // Now we have it passed in.
                copyFramebuffer(context, trailsDest, state.historyFBO.get(), compositeShader, compositeLoc);

                // Use trails output as input for the rest of the chain
                current = trailsDest;
            }
        }
    }

    // Step 2: Apply the generic effect chain (stateless effects)
    // We need to pass 'this' as IEffectProvider
    return effectChain_.process(context, current, *fbPool_, deltaTime, config, *this);
}

void EffectPipeline::copyFramebuffer(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination, juce::OpenGLShaderProgram* compositeShader, GLint compositeTextureLoc)
{
    if (!source || !destination || !source->isValid() || !destination->isValid() || !compositeShader)
        return;

    // Use composite shader for simple copy (linear pass-through)
    destination->bind();
    juce::gl::glDisable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glDisable(juce::gl::GL_BLEND);

    compositeShader->use();
    source->bindTexture(0);
    context.extensions.glUniform1i(compositeTextureLoc, 0);

    fbPool_->renderFullscreenQuad();

    destination->unbind();
}

void EffectPipeline::applyGlobalEffects(Framebuffer* /*sceneFBO*/, juce::OpenGLContext& /*context*/)
{
    // Currently no global effects in the original code other than what's in endFrame?
    // Original endFrame just called applyGlobalEffects() which was empty/placeholder or
    // just blit.
    // RenderEngine.cpp:613 calls applyGlobalEffects().
    // Let's check RenderEngine.cpp content for applyGlobalEffects implementation.
    // It wasn't shown in the first view_file.
}

void EffectPipeline::setQualityLevel(QualityLevel level)
{
    switch (level)
    {
        case QualityLevel::Eco:
            if (auto* bloom = getEffect("bloom"))
                bloom->setEnabled(false);
            if (auto* trails = getEffect("trails"))
                trails->setEnabled(false);
            if (auto* ca = getEffect("chromatic_aberration"))
                ca->setEnabled(false);
            break;

        case QualityLevel::Normal:
            if (auto* bloom = getEffect("bloom"))
                bloom->setEnabled(true);
            if (auto* trails = getEffect("trails"))
                trails->setEnabled(true);
            if (auto* ca = getEffect("chromatic_aberration"))
                ca->setEnabled(true);
            break;

        case QualityLevel::Ultra:
            for (auto& [id, effect] : effects_)
            {
                effect->setEnabled(true);
            }
            break;
    }
}

} // namespace oscil
