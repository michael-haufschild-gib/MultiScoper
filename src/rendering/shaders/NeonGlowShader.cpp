/*
    Oscil - Neon Glow Shader Implementation
*/

#include "rendering/shaders/NeonGlowShader.h"
#include <cmath>
#include <iostream>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* neonVertexShader = R"(
    attribute vec2 position;
    attribute float distFromCenter;
    attribute float t;

    uniform mat4 projection;

    varying float vDistFromCenter;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
    }
)";

static const char* neonFragmentShader = R"(
    varying float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float glowIntensity;

    void main()
    {
        float dist = abs(vDistFromCenter);

        // Neon look: Hot white core, colored falloff
        
        // Core: Sharp, bright, white-ish center
        float core = 1.0 - smoothstep(0.05, 0.2, dist);
        
        // Glow: Smooth exponential falloff
        float glow = exp(-dist * 3.5) * glowIntensity;

        vec3 colorRGB = baseColor.rgb;
        
        // Core is mostly white
        vec3 coreColor = mix(colorRGB, vec3(1.0), 0.7);
        
        // Glow is pure color
        vec3 glowColor = colorRGB;

        // Combine
        vec3 finalRGB = mix(glowColor * glow * 1.5, coreColor * 2.0, core);
        
        // Alpha
        float alpha = opacity * (core + glow * 0.8) * baseColor.a;
        
        gl_FragColor = vec4(finalRGB, alpha);
    }
)";

struct NeonGlowShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint glowIntensityLoc = -1;
};
#endif

NeonGlowShader::NeonGlowShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

NeonGlowShader::~NeonGlowShader() = default;

#if OSCIL_ENABLE_OPENGL
bool NeonGlowShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String v = juce::OpenGLHelpers::translateVertexShaderToV3(neonVertexShader);
    juce::String f = juce::OpenGLHelpers::translateFragmentShaderToV3(neonFragmentShader);

    if (!gl_->program->addVertexShader(v) || !gl_->program->addFragmentShader(f) || !gl_->program->link())
    {
        DBG("NeonGlowShader compile error: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void NeonGlowShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool NeonGlowShader::isCompiled() const
{
    return gl_->compiled;
}

void NeonGlowShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    juce::ignoreUnused(channel2);
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;

    // Additive blending is key for neon
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    gl_->program->use();

    // Setup projection (same as BasicShader logic)
    auto* target = context.getTargetComponent();
    if (!target) return;
    
    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());
    
    // Simple ortho mapping
    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f, // Flip Y so 0 is top
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    // Note: BasicShader inverted Y in a specific way. Let's match that:
    // BasicShader:
    // 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
    // where top=0, bottom=height. -> 2.0 / -height -> -2.0/h. Correct.
    // Translation Y: -(top + bottom)/(top-bottom) -> -height / -height = 1.0. Correct.
    
    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    
    ext.glUniform4f(gl_->baseColorLoc, 
        params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->glowIntensityLoc, 1.0f); // Strong glow by default

    // Prepare geometry
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float height = params.bounds.getHeight();
    float centerY = params.bounds.getCentreY();
    float amp = height * 0.45f * params.verticalScale;

    // Render Channel 1
    {
        std::vector<float> vertices;
        // Wide geometry for the glow
        float width = params.lineWidth * 12.0f; 
        buildLineGeometry(vertices, channel1, centerY, amp, width, params.bounds.getX(), params.bounds.getWidth());
        
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        // Attributes: pos(2), dist(1), t(1)
        // We only need pos and dist for neon. BasicShader uses 0 and 1.
        GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
        GLint distLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
        
        if (posLoc < 0) posLoc = 0;
        if (distLoc < 0) distLoc = 1;

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        
        ext.glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        // Draw multiple passes for extra bloom-like accumulation
        for (int i = 0; i < 3; ++i) {
            ext.glUniform1f(gl_->glowIntensityLoc, 1.0f - (i * 0.2f));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        }
        
        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
    }
    
    // TODO: Channel 2 support if needed (copy paste logic)

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
