/*
    Oscil - Plasma Sine Shader Implementation
*/

#include "rendering/shaders/PlasmaSineShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* plasmaVertexShader = R"(
    attribute vec2 position;
    attribute float vParam;
    attribute float tParam;

    uniform mat4 projection;

    varying float vV;
    varying float vT;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vV = vParam;
        vT = tParam;
    }
)";

static const char* plasmaFragmentShader = R"(
    varying float vV;
    varying float vT;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float time;

    void main()
    {
        float scale = 8.0;
        float x = vT * scale;
        float y = vV * scale;
        float t = time;

        float v = 0.0;
        v += sin(x + t);
        v += sin((y + t) * 0.5);
        v += sin((x + y + t) * 0.5);
        float cx = vT * scale - scale/2.0;
        float cy = vV * scale - scale/2.0;
        v += sin(sqrt(cx*cx + cy*cy + 1.0) + t);

        v = v * 0.25 + 0.5;

        vec3 c1 = baseColor.rgb;
        vec3 c2 = vec3(c1.g, c1.b, c1.r); 
        vec3 c3 = vec3(c1.b, c1.r, c1.g);
        
        vec3 color = mix(c1, c2, v);
        color = mix(color, c3, sin(v * 3.14159));

        gl_FragColor = vec4(color, opacity);
    }
)";

struct PlasmaSineShader::GLResources
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

PlasmaSineShader::PlasmaSineShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

PlasmaSineShader::~PlasmaSineShader() = default;

#if OSCIL_ENABLE_OPENGL
bool PlasmaSineShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String v = juce::OpenGLHelpers::translateVertexShaderToV3(plasmaVertexShader);
    juce::String f = juce::OpenGLHelpers::translateFragmentShaderToV3(plasmaFragmentShader);

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

void PlasmaSineShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool PlasmaSineShader::isCompiled() const
{
    return gl_->compiled;
}

void PlasmaSineShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    juce::ignoreUnused(channel2);
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive usually looks better for plasma

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

    // Render Channel 1
    {
        std::vector<float> vertices;
        // Fill whole waveform area? Or just fill?
        // Let's fill down to zero
        buildFillGeometry(vertices, channel1, centerY, centerY, amp, params.bounds.getX(), params.bounds.getWidth());
        
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
        GLint vLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "vParam");
        GLint tLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "tParam");
        
        if (posLoc < 0) posLoc = 0;
        if (vLoc < 0) vLoc = 1;
        if (tLoc < 0) tLoc = 2;

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        
        ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        ext.glEnableVertexAttribArray(static_cast<GLuint>(tLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(tLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        
        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(tLoc));
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
