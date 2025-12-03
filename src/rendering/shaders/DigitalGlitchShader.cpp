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
    #version 330 core
    in vec2 position;
    in float distFromCenter;
    in float tParam;

    uniform mat4 projection;
    uniform float time;

    out float vDistFromCenter;

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
    #version 330 core
    in float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;

    out vec4 fragColor;

    // Convert sRGB to linear color space
    // Input colors from juce::Colour are in sRGB, but we need linear for correct
    // blending and tonemapping in the rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main()
    {
        float dist = abs(vDistFromCenter);

        // Techy/Scanline look
        float alpha = 1.0 - smoothstep(0.8, 1.0, dist);

        // Convert sRGB input to linear for correct rendering
        vec3 linearColor = sRGBToLinear(baseColor.rgb);

        fragColor = vec4(linearColor, alpha * opacity);
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
    
    if (!gl_->program->addVertexShader(glitchVertexShader) || !gl_->program->addFragmentShader(glitchFragmentShader) || !gl_->program->link())
    {
        DBG("DigitalGlitchShader: Shader compilation failed: " << gl_->program->getLastError());
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

    // Calculate layout based on stereo/mono mode
    float height = params.bounds.getHeight();
    float centerY1, centerY2;
    float amp1, amp2;

    if (params.isStereo && channel2 != nullptr)
    {
        // Stereo stacked layout: L on top half, R on bottom half
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amp1 = halfHeight * 0.45f * params.verticalScale;
        amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        // Mono layout: single channel centered
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amp1 = height * 0.45f * params.verticalScale;
        amp2 = amp1;
    }

    // Get attribute locations once
    GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint distLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
    GLint tLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "tParam");
    if (posLoc < 0) posLoc = 0;
    if (distLoc < 0) distLoc = 1;
    if (tLoc < 0) tLoc = 2;

    // Render Channel 1
    {
        std::vector<float> vertices;
        buildLineGeometry(vertices, channel1, centerY1, amp1, params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

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

    // Render Channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        std::vector<float> vertices;
        buildLineGeometry(vertices, *channel2, centerY2, amp2, params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

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
