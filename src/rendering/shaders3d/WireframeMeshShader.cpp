/*
    Oscil - Wireframe Mesh Shader Implementation
*/

#include "rendering/shaders3d/WireframeMeshShader.h"
#include <cmath>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* wireframeVertexShader = R"(
    attribute vec3 aPosition;
    attribute float aDepth;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;
    uniform float uTime;

    varying float vDepth;
    varying float vGlow;

    void main()
    {
        vec4 worldPos = uModel * vec4(aPosition, 1.0);
        gl_Position = uProjection * uView * worldPos;

        vDepth = aDepth;

        // Distance-based glow falloff
        float dist = length(gl_Position.xyz);
        vGlow = 1.0 / (1.0 + dist * 0.5);
    }
)";

static const char* wireframeFragmentShader = R"(
    uniform vec4 uColor;
    uniform float uGlow;
    uniform float uTime;

    varying float vDepth;
    varying float vGlow;

    void main()
    {
        // Depth-based fade (farther = more transparent)
        float depthFade = 1.0 - vDepth * 0.5;
        depthFade = max(depthFade, 0.2);

        // Pulse effect
        float pulse = sin(uTime * 2.0 + vDepth * 5.0) * 0.1 + 0.9;

        // Calculate final alpha with glow
        float alpha = uColor.a * depthFade * vGlow * pulse;

        // Add glow bloom to color
        vec3 glowColor = uColor.rgb * (1.0 + uGlow * vGlow);

        gl_FragColor = vec4(glowColor, alpha);
    }
)";

WireframeMeshShader::WireframeMeshShader() = default;

WireframeMeshShader::~WireframeMeshShader() = default;

bool WireframeMeshShader::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    auto& ext = context.extensions;

    // Compile shader
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, wireframeVertexShader, wireframeFragmentShader))
    {
        DBG("WireframeMeshShader: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    glowLoc_ = shader_->getUniformIDFromName("uGlow");

    // Create VAO
    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);

    // Create VBO
    ext.glGenBuffers(1, &vbo_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Setup vertex attributes (position: 3, depth: 1)
    GLint posAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aPosition");
    GLint depthAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aDepth");

    const int stride = 4 * sizeof(float);

    if (posAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib), 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    }

    if (depthAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(depthAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(depthAttrib), 1, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(3 * sizeof(float)));
    }

    ext.glBindVertexArray(0);

    compiled_ = true;
    DBG("WireframeMeshShader: Compiled successfully");
    return true;
}

void WireframeMeshShader::release(juce::OpenGLContext& context)
{
    if (!compiled_)
        return;

    auto& ext = context.extensions;

    if (vbo_ != 0)
    {
        ext.glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    shader_.reset();
    compiled_ = false;
}

void WireframeMeshShader::render(juce::OpenGLContext& context,
                                 const WaveformData3D& data,
                                 const Camera3D& camera,
                                 const LightingConfig& lighting)
{
    juce::ignoreUnused(lighting);  // Wireframe doesn't use full lighting

    if (!compiled_ || !data.samples || data.sampleCount < 2)
        return;

    // Generate wireframe mesh
    generateWireframeMesh(data);

    if (vertexCount_ == 0)
        return;

    auto& ext = context.extensions;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(data.lineThickness);

    shader_->use();

    // Set matrix uniforms
    Matrix4 model = Matrix4::translation(0, 0, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    // Set color
    ext.glUniform4f(colorLoc_,
                    data.color.getFloatRed(),
                    data.color.getFloatGreen(),
                    data.color.getFloatBlue(),
                    data.color.getFloatAlpha());

    // Set additional uniforms
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform1f(glowLoc_, lineGlow_);

    // Bind VAO and draw
    ext.glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, vertexCount_);
    ext.glBindVertexArray(0);

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void WireframeMeshShader::update(float deltaTime)
{
    time_ += deltaTime * scrollSpeed_;
    if (time_ > 1000.0f)
        time_ = std::fmod(time_, 1000.0f);
}

void WireframeMeshShader::generateWireframeMesh(const WaveformData3D& data)
{
    vertices_.clear();

    // Each vertex: position (3) + depth (1) = 4 floats
    // We generate horizontal lines (along waveform) and vertical lines (grid depth)

    int xSegments = std::min(data.sampleCount, gridDensity_ * 4);
    int zSegments = gridDensity_;

    // Reserve space for lines
    // Horizontal lines: zSegments lines, each with xSegments-1 segments = 2 vertices per segment
    // Vertical lines: xSegments lines across depth
    size_t estimatedVertices = static_cast<size_t>(zSegments * (xSegments - 1) * 2 +
                                                   xSegments * (zSegments - 1) * 2);
    vertices_.reserve(estimatedVertices * 4);

    float scrollOffset = std::fmod(time_, 1.0f);

    // Generate horizontal lines (along X, at different Z depths)
    for (int z = 0; z < zSegments; ++z)
    {
        float zNorm = static_cast<float>(z) / static_cast<float>(zSegments - 1);
        float zPos = (zNorm - 0.5f) * gridDepth_;
        float depth = zNorm;  // For fading

        // Offset by scroll
        zPos -= scrollOffset * gridDepth_ / static_cast<float>(zSegments);
        if (zPos < -gridDepth_ * 0.5f)
            zPos += gridDepth_;

        for (int x = 0; x < xSegments - 1; ++x)
        {
            float t1 = static_cast<float>(x) / static_cast<float>(xSegments - 1);
            float t2 = static_cast<float>(x + 1) / static_cast<float>(xSegments - 1);

            int idx1 = static_cast<int>(t1 * static_cast<float>(data.sampleCount - 1));
            int idx2 = static_cast<int>(t2 * static_cast<float>(data.sampleCount - 1));

            float x1 = t1 * 2.0f - 1.0f;
            float x2 = t2 * 2.0f - 1.0f;
            float y1 = data.samples[idx1];
            float y2 = data.samples[idx2];

            // Vertex 1
            vertices_.push_back(x1);
            vertices_.push_back(y1);
            vertices_.push_back(zPos);
            vertices_.push_back(depth);

            // Vertex 2
            vertices_.push_back(x2);
            vertices_.push_back(y2);
            vertices_.push_back(zPos);
            vertices_.push_back(depth);
        }
    }

    // Generate vertical lines (along Z, at sample positions)
    int xStep = std::max(1, data.sampleCount / gridDensity_);
    for (int x = 0; x < data.sampleCount; x += xStep)
    {
        float t = static_cast<float>(x) / static_cast<float>(data.sampleCount - 1);
        float xPos = t * 2.0f - 1.0f;
        float y = data.samples[x];

        for (int z = 0; z < zSegments - 1; ++z)
        {
            float zNorm1 = static_cast<float>(z) / static_cast<float>(zSegments - 1);
            float zNorm2 = static_cast<float>(z + 1) / static_cast<float>(zSegments - 1);

            float zPos1 = (zNorm1 - 0.5f) * gridDepth_;
            float zPos2 = (zNorm2 - 0.5f) * gridDepth_;

            // Apply scroll offset
            zPos1 -= scrollOffset * gridDepth_ / static_cast<float>(zSegments);
            zPos2 -= scrollOffset * gridDepth_ / static_cast<float>(zSegments);

            // Wrap around
            if (zPos1 < -gridDepth_ * 0.5f) zPos1 += gridDepth_;
            if (zPos2 < -gridDepth_ * 0.5f) zPos2 += gridDepth_;

            // Vertex 1
            vertices_.push_back(xPos);
            vertices_.push_back(y);
            vertices_.push_back(zPos1);
            vertices_.push_back(zNorm1);

            // Vertex 2
            vertices_.push_back(xPos);
            vertices_.push_back(y);
            vertices_.push_back(zPos2);
            vertices_.push_back(zNorm2);
        }
    }

    if (vertices_.empty())
    {
        vertexCount_ = 0;
        return;
    }

    // Upload to GPU
    auto* glContext = juce::OpenGLContext::getCurrentContext();
    if (!glContext)
        return;

    auto& ext = glContext->extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                     vertices_.data(), GL_DYNAMIC_DRAW);

    vertexCount_ = static_cast<int>(vertices_.size() / 4);
}

void WireframeMeshShader::setParameter(const juce::String& name, float value)
{
    if (name == "gridDensity")
        gridDensity_ = static_cast<int>(value);
    else if (name == "gridDepth")
        gridDepth_ = value;
    else if (name == "lineGlow")
        lineGlow_ = value;
    else if (name == "perspectiveStrength")
        perspectiveStrength_ = value;
    else if (name == "scrollSpeed")
        scrollSpeed_ = value;
}

float WireframeMeshShader::getParameter(const juce::String& name) const
{
    if (name == "gridDensity")
        return static_cast<float>(gridDensity_);
    if (name == "gridDepth")
        return gridDepth_;
    if (name == "lineGlow")
        return lineGlow_;
    if (name == "perspectiveStrength")
        return perspectiveStrength_;
    if (name == "scrollSpeed")
        return scrollSpeed_;
    return 0.0f;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
