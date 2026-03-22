/*
    Oscil - Waveform Pass Implementation
*/

#include "rendering/subsystems/WaveformPass.h"
#include "rendering/subsystems/RenderStats.h"

namespace oscil
{

using namespace juce::gl;

WaveformPass::WaveformPass()
    : gridRenderer_(std::make_unique<GridRenderer>())
    , registry_(std::make_unique<ShaderRegistry>())
{
}

WaveformPass::~WaveformPass()
{
    if (initialized_ && !compiledShaders_.empty())
    {
        DBG("[WaveformPass] LEAK: Destructor called without shutdown(). "
            << compiledShaders_.size() << " compiled shaders will leak GPU resources.");
    }
}

bool WaveformPass::initialize(juce::OpenGLContext& context, int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;

    context_ = &context;
    currentWidth_ = width;
    currentHeight_ = height;

    // Only compile the essential "basic" shader at startup; others are lazy-loaded.
    {
        auto basicShader = registry_->createShader("basic");
        if (basicShader && basicShader->compile(context))
        {
            compiledShaders_["basic"] = std::move(basicShader);
            RE_LOG("WaveformPass: Compiled essential 'basic' shader");
        }
        else
        {
            RE_LOG("WaveformPass: WARNING - Failed to compile basic shader!");
        }
    }

    if (gridRenderer_)
        gridRenderer_->initialize(context);

    initialized_ = true;
    return true;
}

void WaveformPass::shutdown(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    for (auto& pair : compiledShaders_)
        pair.second->release(context);
    compiledShaders_.clear();

    if (gridRenderer_)
        gridRenderer_->release(context);

    context_ = nullptr;
    initialized_ = false;
}

void WaveformPass::prepareRender(const WaveformRenderData& /*data*/, WaveformRenderState& state, float deltaTime)
{
    state.updateTiming(deltaTime);
    glDisable(GL_DEPTH_TEST);
}

WaveformPass::ViewportRect WaveformPass::computePaneViewport(const juce::Rectangle<float>& bounds) const
{
    float scale = static_cast<float>(context_->getRenderingScale());
    auto* target = context_->getTargetComponent();
    if (!target) return {0, 0, 1, 1};
    float logicalHeight = static_cast<float>(target->getHeight());

    return {
        std::max(0, static_cast<int>(bounds.getX() * scale)),
        std::max(0, static_cast<int>((logicalHeight - (bounds.getY() + bounds.getHeight())) * scale)),
        std::max(1, static_cast<int>(bounds.getWidth() * scale)),
        std::max(1, static_cast<int>(bounds.getHeight() * scale))
    };
}

WaveformShader* WaveformPass::resolveShader(const juce::String& shaderId)
{
    auto it = compiledShaders_.find(shaderId.toStdString());
    if (it != compiledShaders_.end())
        return it->second.get();

    if (context_)
    {
        auto newShader = registry_->createShader(shaderId);
        if (newShader && newShader->compile(*context_))
        {
            compiledShaders_[shaderId.toStdString()] = std::move(newShader);
            return compiledShaders_[shaderId.toStdString()].get();
        }
    }

    auto basicIt = compiledShaders_.find("basic");
    return (basicIt != compiledShaders_.end()) ? basicIt->second.get() : nullptr;
}

void WaveformPass::renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config, float accumulatedTime)
{
    WaveformShader* shader = resolveShader(shaderTypeToId(config.shaderType));
    if (!shader || !shader->isCompiled()) return;

    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;
    params.time = accumulatedTime;
    params.shaderIntensity = config.bloom.enabled ? config.bloom.intensity : 1.0f;

    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void WaveformPass::renderGrid(const WaveformRenderData& data)
{
    if (!gridRenderer_ || !context_)
        return;

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    auto vp = computePaneViewport(data.bounds);
    glViewport(vp.x, vp.y, vp.w, vp.h);

    gridRenderer_->render(*context_, data);

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

} // namespace oscil
