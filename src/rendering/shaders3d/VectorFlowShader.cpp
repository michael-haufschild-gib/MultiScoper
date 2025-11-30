/*
    Oscil - Vector Flow Shader Implementation
*/

#include "rendering/shaders3d/VectorFlowShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* vectorVertexShader = R"(
    attribute vec3 position;
    attribute float tParam; // 0 to 1 along wave

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;
    
    varying float vT;

    void main()
    {
        gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
        vT = tParam;
    }
)";

static const char* vectorFragmentShader = R"(
    varying float vT;

    uniform vec4 uColor;
    uniform float uTime;
    uniform vec3 uFlowParams; // x=speed, y=segLen, z=gapLen

    void main()
    {
        float speed = uFlowParams.x;
        float segLen = uFlowParams.y;
        float gapLen = uFlowParams.z;
        float period = segLen + gapLen;

        // Moving coordinate
        float pos = vT - uTime * speed;
        
        // Modulo to create repeating pattern
        float m = mod(pos, period);
        
        // Discard gaps
        if (m > segLen) {
            discard;
        }

        // Fade edges of segments
        float alpha = uColor.a;
        float edge = min(m, segLen - m);
        float fade = smoothstep(0.0, segLen * 0.1, edge);
        
        gl_FragColor = vec4(uColor.rgb, alpha * fade);
    }
)";

VectorFlowShader::VectorFlowShader() = default;
VectorFlowShader::~VectorFlowShader() = default;

bool VectorFlowShader::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, vectorVertexShader, vectorFragmentShader))
    {
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    flowParamsLoc_ = shader_->getUniformIDFromName("uFlowParams");

    context.extensions.glGenVertexArrays(1, &vao_);
    context.extensions.glGenBuffers(1, &vbo_);

    compiled_ = true;
    return true;
}

void VectorFlowShader::release(juce::OpenGLContext& context)
{
    if (!compiled_) return;
    context.extensions.glDeleteBuffers(1, &vbo_);
    context.extensions.glDeleteVertexArrays(1, &vao_);
    shader_.reset();
    compiled_ = false;
}

void VectorFlowShader::update(float deltaTime)
{
    time_ += deltaTime;
}

void VectorFlowShader::setParameter(const juce::String& name, float value)
{
    if (name == "flowSpeed") flowSpeed_ = value;
}

float VectorFlowShader::getParameter(const juce::String& name) const
{
    if (name == "flowSpeed") return flowSpeed_;
    return 0.0f;
}

void VectorFlowShader::render(juce::OpenGLContext& context,
                              const WaveformData3D& data,
                              const Camera3D& camera,
                              const LightingConfig& lighting)
{
    if (!compiled_ || data.sampleCount < 2) return;
    juce::ignoreUnused(lighting);

    generateVectorMesh(data);
    if (vertices_.empty()) return;

    auto& ext = context.extensions;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive flow looks good

    shader_->use();
    
    Matrix4 model = Matrix4::translation(0, 0, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    ext.glUniform4f(colorLoc_, data.color.getFloatRed(), data.color.getFloatGreen(), data.color.getFloatBlue(), data.color.getFloatAlpha());
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform3f(flowParamsLoc_, flowSpeed_, segmentLength_, gapLength_);

    ext.glBindVertexArray(vao_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);

    // Attribs: pos(3), t(1)
    GLint posLoc = ext.glGetAttribLocation(shader_->getProgramID(), "position");
    GLint tLoc = ext.glGetAttribLocation(shader_->getProgramID(), "tParam");
    if (posLoc < 0) posLoc = 0;
    if (tLoc < 0) tLoc = 1;

    ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(tLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(tLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));

    // Draw lines or strips? Using TRIANGLE_STRIP for a thick line ribbon
    // Or just LINES for vectors?
    // Let's use a ribbon mesh generator from WaveformShader3D logic but simplified
    // Actually, let's reuse the ribbon generation but we need to do it here to populate vertices_
    // Or better: Use a custom generation for simple 3D line strip
    // If we use GL_LINE_STRIP, we can set thickness via glLineWidth
    
    glLineWidth(data.lineThickness);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertexCount_));
    
    ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(tLoc));
    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VectorFlowShader::generateVectorMesh(const WaveformData3D& data)
{
    vertices_.clear();
    vertices_.reserve(static_cast<size_t>(data.sampleCount * 4));
    
    for (int i = 0; i < data.sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(data.sampleCount - 1);
        float x = t * 2.0f - 1.0f;
        float y = data.samples[i];
        
        vertices_.push_back(x);
        vertices_.push_back(y);
        vertices_.push_back(0.0f); // Z is handled by model matrix
        vertices_.push_back(t);
    }
    
    vertexCount_ = static_cast<size_t>(data.sampleCount);
}

#endif

} // namespace oscil
