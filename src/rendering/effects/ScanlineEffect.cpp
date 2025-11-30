/*
    Oscil - Scanline Effect Implementation
*/

#include "rendering/effects/ScanlineEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* scanlineFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float density;
    uniform vec2 resolution;
    uniform float phosphorGlow;

    varying vec2 vTexCoord;

    void main()
    {
        vec4 color = texture2D(sourceTexture, vTexCoord);

        // Calculate scanline pattern
        float scanline = sin(vTexCoord.y * resolution.y * density * 3.14159) * 0.5 + 0.5;

        // Apply scanline darkening
        float darkness = mix(1.0, scanline, intensity);

        // Optional phosphor glow between scanlines
        float glow = 0.0;
        if (phosphorGlow > 0.0)
        {
            // Sample neighboring pixels for glow
            vec2 offset = vec2(0.0, 1.0 / resolution.y);
            vec3 above = texture2D(sourceTexture, vTexCoord - offset).rgb;
            vec3 below = texture2D(sourceTexture, vTexCoord + offset).rgb;
            glow = (dot(above, vec3(0.333)) + dot(below, vec3(0.333))) * 0.5 * phosphorGlow;
        }

        vec3 result = color.rgb * darkness + vec3(glow * 0.1);

        gl_FragColor = vec4(result, color.a);
    }
)";

ScanlineEffect::ScanlineEffect()
{
}

ScanlineEffect::~ScanlineEffect() = default;

bool ScanlineEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, scanlineFragmentShader))
    {
        DBG("ScanlineEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    densityLoc_ = shader_->getUniformIDFromName("density");
    resolutionLoc_ = shader_->getUniformIDFromName("resolution");
    phosphorGlowLoc_ = shader_->getUniformIDFromName("phosphorGlow");

    compiled_ = true;
    DBG("ScanlineEffect: Compiled successfully");
    return true;
}

void ScanlineEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool ScanlineEffect::isCompiled() const
{
    return compiled_;
}

void ScanlineEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    juce::ignoreUnused(deltaTime);

    if (!compiled_ || !source || !destination)
        return;

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    source->bindTexture(0);
    ext.glUniform1i(textureLoc_, 0);
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);
    ext.glUniform1f(densityLoc_, settings_.density);
    ext.glUniform2f(resolutionLoc_,
        static_cast<float>(source->width),
        static_cast<float>(source->height));
    ext.glUniform1f(phosphorGlowLoc_, settings_.phosphorGlow ? 1.0f : 0.0f);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
