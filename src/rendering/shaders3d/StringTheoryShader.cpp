/*
    Oscil - String Theory Shader Implementation
*/

#include "rendering/shaders3d/StringTheoryShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* stringVertexShader = R"(
    #version 330 core
    in vec3 position;
    in float stringIndex;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;
    uniform float uTime;
    
    void main()
    {
        vec3 pos = position;
        
        // Add vibration based on string index and time
        float vib = sin(uTime * 5.0 + stringIndex * 2.0 + pos.x * 3.14);
        pos.y += vib * 0.02; // Subtle vibration
        
        gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
    }
)";

static const char* stringFragmentShader = R"(
    #version 330 core
    uniform vec4 uColor;
    out vec4 fragColor;

    // Convert sRGB to linear color space for correct rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main()
    {
        vec3 linearColor = sRGBToLinear(uColor.rgb);
        fragColor = vec4(linearColor, uColor.a);
    }
)";

StringTheoryShader::StringTheoryShader() = default;
StringTheoryShader::~StringTheoryShader()
{
#if OSCIL_ENABLE_OPENGL
    if (compiled_)
    {
        std::cerr << "[StringTheoryShader] LEAK: Destructor called without release(). "
                  << "GPU resources may have leaked." << std::endl;
        jassertfalse;
    }
#endif
}

bool StringTheoryShader::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, stringVertexShader, stringFragmentShader))
    {
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    timeLoc_ = shader_->getUniformIDFromName("uTime");

    context.extensions.glGenVertexArrays(1, &vao_);
    context.extensions.glGenBuffers(1, &vbo_);

    compiled_ = true;
    return true;
}

void StringTheoryShader::release(juce::OpenGLContext& context)
{
    if (!compiled_) return;
    context.extensions.glDeleteBuffers(1, &vbo_);
    context.extensions.glDeleteVertexArrays(1, &vao_);
    shader_.reset();
    compiled_ = false;
}

void StringTheoryShader::update(float deltaTime)
{
    time_ += deltaTime;
}

void StringTheoryShader::setParameter(const juce::String& name, float value)
{
    if (name == "stringCount") stringCount_ = static_cast<int>(value);
}

float StringTheoryShader::getParameter(const juce::String& name) const
{
    if (name == "stringCount") return static_cast<float>(stringCount_);
    return 0.0f;
}

void StringTheoryShader::render(juce::OpenGLContext& context,
                                const WaveformData3D& data,
                                const Camera3D& camera,
                                const LightingConfig& lighting)
{
    if (!compiled_ || data.sampleCount < 2) return;
    juce::ignoreUnused(lighting);

    // Calculate xSpread to fill the screen width and halfHeight for vertical scaling
    float xSpread = 1.0f;
    float halfHeight = 1.0f;

    if (camera.getProjection() == CameraProjection::Orthographic)
    {
        float height = camera.getOrthoSize();
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }
    else
    {
        float dist = (camera.getPosition() - camera.getTarget()).length();
        float fovRad = camera.getFOV() * 3.14159265f / 180.0f;
        float height = 2.0f * dist * std::tan(fovRad * 0.5f);
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }

    generateStringMesh(data, xSpread);
    if (vertices_.empty()) return;

    auto& ext = context.extensions;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    shader_->use();
    
    // Scale amplitude by halfHeight to map normalized amplitude to world space
    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    ext.glUniform4f(colorLoc_, data.color.getFloatRed(), data.color.getFloatGreen(), data.color.getFloatBlue(), data.color.getFloatAlpha());
    ext.glUniform1f(timeLoc_, time_);

    ext.glBindVertexArray(vao_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);

    // Attribs: pos(3), stringIndex(1)
    GLint posLoc = ext.glGetAttribLocation(shader_->getProgramID(), "position");
    GLint idxLoc = ext.glGetAttribLocation(shader_->getProgramID(), "stringIndex");
    if (posLoc < 0) posLoc = 0;
    if (idxLoc < 0) idxLoc = 1;

    ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(idxLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(idxLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));

    glLineWidth(data.lineThickness);
    // Draw lines. Since we have multiple disconnected strings, we can use GL_LINES or multiple GL_LINE_STRIP calls.
    // For efficiency in one draw call, we can use GL_LINE_STRIP with restart index or degenerate vertices.
    // But simplest here with a single buffer: generate vertices for lines (segments), i.e., GL_LINES.
    // That means duplicating vertices.
    
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount_));
    
    ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(idxLoc));
    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Restore GL state
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void StringTheoryShader::generateStringMesh(const WaveformData3D& data, float xSpread)
{
    vertices_.clear();
    // Estimate size: samples * count * 2 (for lines) * 4 floats
    vertices_.reserve(static_cast<size_t>(data.sampleCount * stringCount_ * 2 * 4));
    
    for (int s = 0; s < stringCount_; ++s)
    {
        float z = (static_cast<float>(s) - static_cast<float>(stringCount_ - 1) * 0.5f) * stringSpacing_;
        float idx = static_cast<float>(s);
        
        // Generate lines for this string
        for (int i = 0; i < data.sampleCount - 1; ++i)
        {
            float t1 = static_cast<float>(i) / static_cast<float>(data.sampleCount - 1);
            float t2 = static_cast<float>(i+1) / static_cast<float>(data.sampleCount - 1);
            
            float x1 = (t1 * 2.0f - 1.0f) * xSpread;
            float x2 = (t2 * 2.0f - 1.0f) * xSpread;
            
            float y1 = data.samples[i];
            float y2 = data.samples[i+1];
            
            // Vertex 1
            vertices_.push_back(x1);
            vertices_.push_back(y1);
            vertices_.push_back(z);
            vertices_.push_back(idx);
            
            // Vertex 2
            vertices_.push_back(x2);
            vertices_.push_back(y2);
            vertices_.push_back(z);
            vertices_.push_back(idx);
        }
    }
    
    vertexCount_ = vertices_.size() / 4;
}

#endif

} // namespace oscil
