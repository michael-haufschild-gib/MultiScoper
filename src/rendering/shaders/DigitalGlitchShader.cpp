/*
    Oscil - Digital Glitch Shader Implementation
*/

#include "rendering/shaders/DigitalGlitchShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* glitchVertexShader = R"(
    attribute vec2 position;
    attribute float distFromCenter;
    attribute float tParam;

    uniform mat4 projection;
    uniform float time;

    varying float vDistFromCenter;

    float rand(float n) { return fract(sin(n) * 43758.5453123); }

    void main()
    {
        vec2 pos = position;
        
        // Glitch logic
        // Create random blocky offsets
        float blockIdx = floor(tParam * 30.0);
        float r = rand(blockIdx + floor(time * 10.0));
        
        if (r > 0.9) {
            // Y offset
            pos.y += (rand(r) - 0.5) * 100.0;
            // X offset
            pos.x += (rand(r + 1.0) - 0.5) * 20.0;
        }

        gl_Position = projection * vec4(pos, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
    }
)";

static const char* glitchFragmentShader = R"(
    varying float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;

    void main()
    {
        float dist = abs(vDistFromCenter);
        
        // Techy/Scanline look
        float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
        
        gl_FragColor = vec4(baseColor.rgb, alpha * opacity);
    }
)";

struct DigitalGlitchShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint timeLoc = -1;
};
#endif

DigitalGlitchShader::DigitalGlitchShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

DigitalGlitchShader::~DigitalGlitchShader() = default;

#if OSCIL_ENABLE_OPENGL
bool DigitalGlitchShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String v = juce::OpenGLHelpers::translateVertexShaderToV3(glitchVertexShader);
    juce::String f = juce::OpenGLHelpers::translateFragmentShaderToV3(glitchFragmentShader);

    if (!gl_->program->addVertexShader(v) || !gl_->program->addFragmentShader(f) || !gl_->program->link())
    {
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->timeLoc = gl_->program->getUniformIDFromName("time");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void DigitalGlitchShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool DigitalGlitchShader::isCompiled() const
{
    return gl_->compiled;
}

void DigitalGlitchShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    juce::ignoreUnused(channel2);
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    gl_->program->use();

    auto* target = context.getTargetComponent();
    if (!target) return;
    
    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());
    
    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    ext.glUniform4f(gl_->baseColorLoc, 
        params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->timeLoc, params.time);

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float height = params.bounds.getHeight();
    float centerY = params.bounds.getCentreY();
    float amp = height * 0.45f * params.verticalScale;

    {
        std::vector<float> vertices;
        buildLineGeometry(vertices, channel1, centerY, amp, params.lineWidth, params.bounds.getX(), params.bounds.getWidth());
        
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
        GLint distLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
        GLint tLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "tParam");
        
        if (posLoc < 0) posLoc = 0;
        if (distLoc < 0) distLoc = 1;
        if (tLoc < 0) tLoc = 2;

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        
        ext.glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        ext.glEnableVertexAttribArray(static_cast<GLuint>(tLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(tLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        
        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(tLoc));
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
