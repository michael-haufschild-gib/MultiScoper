/*
    Oscil - Instanced Waveform Shader Implementation
*/

#include "rendering/shaders/InstancedShader.h"
#include "rendering/WaveformGLRenderer.h"
#include "rendering/WaveformRenderState.h"
#include "BinaryData.h"

#include <iostream>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

#define INSTANCED_LOG(msg) DBG("[InstancedShader] " << msg)

struct InstancedShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;                // Required on macOS Core Profile
    GLuint ubo = 0;
    GLuint uboBindingPoint = 0;  // UBO binding point index

    // Uniform locations
    GLint projectionLoc = -1;

    // Attribute locations
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};

InstancedShader::InstancedShader()
    : gl_(std::make_unique<GLResources>())
{
    // Initialize instance data to zeros
    std::memset(&instanceData_, 0, sizeof(instanceData_));
}

InstancedShader::~InstancedShader()
{
    // Check for both compiled state AND orphaned VAO/UBO resources
    if (compiled_ || (gl_ && (gl_->vao != 0 || gl_->ubo != 0)))
    {
        std::cerr << "[InstancedShader] LEAK: Destructor called without release(). "
                  << "GPU resources may have leaked. (compiled=" << compiled_
                  << ", vao=" << (gl_ ? gl_->vao : 0)
                  << ", ubo=" << (gl_ ? gl_->ubo : 0) << ")" << std::endl;
        jassertfalse;
    }
}

bool InstancedShader::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    INSTANCED_LOG("Compiling instanced shader...");

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    // Load vertex shader from binary data
    juce::String vertexCode = juce::String::createStringFromData(
        BinaryData::waveform_instanced_vert, 
        BinaryData::waveform_instanced_vertSize);
    
    if (!gl_->program->addVertexShader(vertexCode))
    {
        INSTANCED_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }
    INSTANCED_LOG("Vertex shader compiled OK");

    // Load fragment shader from binary data
    juce::String fragmentCode = juce::String::createStringFromData(
        BinaryData::waveform_instanced_frag, 
        BinaryData::waveform_instanced_fragSize);
    
    if (!gl_->program->addFragmentShader(fragmentCode))
    {
        INSTANCED_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }
    INSTANCED_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())
    {
        INSTANCED_LOG("Shader linking FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }
    INSTANCED_LOG("Shader linked OK");

    auto& ext = context.extensions;
    GLuint programId = static_cast<GLuint>(gl_->program->getProgramID());

    // Get uniform block index for InstanceData and bind to binding point
    // Using juce::gl:: functions directly as they're not in OpenGLExtensionFunctions
    GLuint blockIndex = juce::gl::glGetUniformBlockIndex(programId, "InstanceData");
    if (blockIndex == GL_INVALID_INDEX)
    {
        INSTANCED_LOG("Failed to find uniform block 'InstanceData'");
        gl_->program.reset();
        return false;
    }
    gl_->uboBindingPoint = 0;
    juce::gl::glUniformBlockBinding(programId, blockIndex, gl_->uboBindingPoint);
    INSTANCED_LOG("Bound InstanceData block (index " << static_cast<int>(blockIndex) << ") to binding point " << static_cast<int>(gl_->uboBindingPoint));

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    INSTANCED_LOG("Projection uniform location: " << gl_->projectionLoc);

    // Get attribute locations (for reference, though VAO should handle these)
    gl_->positionLoc = ext.glGetAttribLocation(programId, "position");
    gl_->distFromCenterLoc = ext.glGetAttribLocation(programId, "distFromCenter");
    INSTANCED_LOG("Attribute locations - position: " << gl_->positionLoc 
                  << ", distFromCenter: " << gl_->distFromCenterLoc);

    // Create VAO (REQUIRED on macOS Core Profile - styleguide constitutional rule #5)
    ext.glGenVertexArrays(1, &gl_->vao);
    if (gl_->vao == 0)
    {
        INSTANCED_LOG("Failed to create VAO");
        gl_->program.reset();
        return false;
    }
    INSTANCED_LOG("Created VAO: " << static_cast<int>(gl_->vao));

    // Create UBO for instance data
    ext.glGenBuffers(1, &gl_->ubo);
    if (gl_->ubo == 0)
    {
        INSTANCED_LOG("Failed to create UBO");
        ext.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
        gl_->program.reset();
        return false;
    }

    // Allocate UBO storage
    ext.glBindBuffer(GL_UNIFORM_BUFFER, gl_->ubo);
    ext.glBufferData(GL_UNIFORM_BUFFER, sizeof(InstanceDataUBO), nullptr, GL_DYNAMIC_DRAW);
    ext.glBindBuffer(GL_UNIFORM_BUFFER, 0);

    compiled_ = true;
    INSTANCED_LOG("Compilation successful, VAO: " << static_cast<int>(gl_->vao) << ", UBO: " << static_cast<int>(gl_->ubo));
    return true;
}

void InstancedShader::release(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    // Always clean up VAO/UBO if they were created, regardless of compile status
    // This prevents resource leaks if compile() fails after creating these resources
    if (gl_->ubo != 0)
    {
        ext.glDeleteBuffers(1, &gl_->ubo);
        gl_->ubo = 0;
    }

    if (gl_->vao != 0)
    {
        ext.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
    }

    gl_->program.reset();
    compiled_ = false;

    INSTANCED_LOG("Resources released");
}

void InstancedShader::bind(juce::OpenGLContext& context)
{
    if (!compiled_ || !gl_->program)
        return;

    gl_->program->use();

    // Bind VAO first (REQUIRED on macOS Core Profile)
    context.extensions.glBindVertexArray(gl_->vao);

    // Bind UBO to the binding point so shader can access instance data
    // Using juce::gl:: function directly as it's not in OpenGLExtensionFunctions
    juce::gl::glBindBufferBase(GL_UNIFORM_BUFFER, gl_->uboBindingPoint, gl_->ubo);
}

void InstancedShader::unbind(juce::OpenGLContext& context)
{
    // Unbind VAO when done
    context.extensions.glBindVertexArray(0);
    // UBO unbinding is optional - the next shader bind will overwrite anyway
}

void InstancedShader::updateInstanceData(
    juce::OpenGLContext& context,
    const std::vector<std::pair<const WaveformRenderData*, const WaveformRenderState*>>& entries,
    float viewportWidth, float viewportHeight)
{
    if (!compiled_)
        return;

    instanceCount_ = std::min(entries.size(), kMaxInstancedWaveforms);
    
    if (instanceCount_ == 0)
        return;

    // Fill instance data
    for (size_t i = 0; i < instanceCount_; ++i)
    {
        const auto* data = entries[i].first;
        const auto* state = entries[i].second;

        if (!data || !state)
            continue;

        const auto& bounds = data->bounds;
        const auto& config = state->visualConfig;

        // Transform: convert bounds to normalized coordinates
        // The shader expects position in clip space after transform
        float xOffset = bounds.getX() / viewportWidth * 2.0f - 1.0f;
        float yOffset = 1.0f - bounds.getY() / viewportHeight * 2.0f;  // Flip Y
        float xScale = bounds.getWidth() / viewportWidth * 2.0f;
        float yScale = bounds.getHeight() / viewportHeight * 2.0f;

        instanceData_.transforms[i] = { xOffset, yOffset, xScale, yScale };

        // Color
        instanceData_.colors[i] = {
            data->colour.getFloatRed(),
            data->colour.getFloatGreen(),
            data->colour.getFloatBlue(),
            data->colour.getFloatAlpha()
        };

        // Params - use default glow intensity since VisualConfiguration doesn't have it directly
        constexpr float kDefaultGlowIntensity = 0.5f;
        instanceData_.params[i] = {
            data->opacity,
            data->lineWidth,
            kDefaultGlowIntensity,
            0.0f  // Reserved
        };

        // Viewport (normalized)
        instanceData_.viewports[i] = {
            bounds.getX() / viewportWidth,
            bounds.getY() / viewportHeight,
            bounds.getWidth() / viewportWidth,
            bounds.getHeight() / viewportHeight
        };
    }

    // Upload to GPU
    auto& ext = context.extensions;
    ext.glBindBuffer(GL_UNIFORM_BUFFER, gl_->ubo);
    ext.glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(InstanceDataUBO), &instanceData_);
    ext.glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void InstancedShader::setProjection(const float* projectionMatrix)
{
    if (!compiled_ || !gl_->program || gl_->projectionLoc < 0)
        return;

    // Assumes shader is already bound
    gl_->program->setUniformMat4("projection", projectionMatrix, 1, false);
}

void InstancedShader::render(juce::OpenGLContext& context,
                             const std::vector<float>& channel1,
                             const std::vector<float>* channel2,
                             const ShaderRenderParams& params)
{
    (void)context;
    (void)channel1;
    (void)channel2;
    (void)params;
    
    // InstancedShader is not designed for per-waveform rendering.
    // Use updateInstanceData() and instanced draw calls instead.
    // This method exists only to satisfy the WaveformShader interface.
    jassertfalse;  // Should not be called
}

#endif // OSCIL_ENABLE_OPENGL

} // namespace oscil

