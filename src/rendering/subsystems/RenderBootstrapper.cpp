/*
    Oscil - Render Bootstrapper Implementation
*/

#include "rendering/subsystems/RenderBootstrapper.h"

#include <juce_core/juce_core.h>

namespace oscil
{

using namespace juce::gl;

// Blit shader for final output to screen
static const char* blitVertexShader = R"(
    #version 330 core
    in vec2 position;
    in vec2 texCoord;
    out vec2 vTexCoord;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        vTexCoord = texCoord;
    }
)";

static const char* blitFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    in vec2 vTexCoord;
    out vec4 fragColor;

    // ACES Filmic Tone Mapping Curve
    // Adapted for real-time rendering (Narkowicz 2015)
    vec3 aces(vec3 x) {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }

    // Simple hash for dithering
    float random(vec2 p) {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main() {
        vec4 color = texture(sourceTexture, vTexCoord);
        vec3 hdrColor = color.rgb;

        // 1. Tone Mapping (HDR -> LDR)
        // ACES gives a filmic look with nice highlight rolloff
        vec3 mapped = aces(hdrColor);

        // 2. Gamma Correction (Linear -> sRGB)
        // Standard 2.2 gamma approximation
        mapped = pow(mapped, vec3(1.0 / 2.2));

        // 3. Dithering
        // Prevents banding in dark gradients on 8-bit displays
        mapped += (random(gl_FragCoord.xy) - 0.5) * (1.0 / 255.0);

        fragColor = vec4(mapped, color.a);
    }
)";

// Simple pass-through shader for compositing into the scene FBO while staying in linear HDR space
static const char* compositeFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    in vec2 vTexCoord;
    out vec4 fragColor;

    void main() {
        fragColor = texture(sourceTexture, vTexCoord);
    }
)";

RenderBootstrapper::RenderBootstrapper() {}

RenderBootstrapper::~RenderBootstrapper() {}

bool RenderBootstrapper::initialize(juce::OpenGLContext& context)
{
    // Compile blit shader (tone map + gamma) for final output
    blitShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!blitShader_->addVertexShader(blitVertexShader) || !blitShader_->addFragmentShader(blitFragmentShader))
    {
        DBG("RenderBootstrapper: Failed to compile blit shader: " << blitShader_->getLastError());
        return false;
    }

    // Bind attribute locations BEFORE linking
    GLuint programID = blitShader_->getProgramID();
    context.extensions.glBindAttribLocation(programID, 0, "position");
    context.extensions.glBindAttribLocation(programID, 1, "texCoord");

    if (!blitShader_->link())
    {
        DBG("RenderBootstrapper: Failed to link blit shader: " << blitShader_->getLastError());
        return false;
    }

    blitTextureLoc_ = blitShader_->getUniformIDFromName("sourceTexture");
    DBG("RenderBootstrapper: Blit shader compiled, sourceTexture uniform=" << blitTextureLoc_);

    // Compile composite shader (linear pass-through)
    compositeShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compositeShader_->addVertexShader(blitVertexShader) ||
        !compositeShader_->addFragmentShader(compositeFragmentShader))
    {
        DBG("RenderBootstrapper: Failed to compile composite shader: " << compositeShader_->getLastError());
        blitShader_.reset();
        return false;
    }

    GLuint compositeProgramId = compositeShader_->getProgramID();
    context.extensions.glBindAttribLocation(compositeProgramId, 0, "position");
    context.extensions.glBindAttribLocation(compositeProgramId, 1, "texCoord");

    if (!compositeShader_->link())
    {
        DBG("RenderBootstrapper: Failed to link composite shader: " << compositeShader_->getLastError());
        compositeShader_.reset();
        blitShader_.reset();
        return false;
    }

    compositeTextureLoc_ = compositeShader_->getUniformIDFromName("sourceTexture");
    DBG("RenderBootstrapper: Composite shader compiled, sourceTexture uniform=" << compositeTextureLoc_);

    return true;
}

void RenderBootstrapper::shutdown(juce::OpenGLContext& /*context*/)
{
    blitShader_.reset();
    compositeShader_.reset();
    blitTextureLoc_ = -1;
    compositeTextureLoc_ = -1;
}

} // namespace oscil
