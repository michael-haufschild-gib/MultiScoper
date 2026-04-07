/*
    Oscil - Post-Process Effect Base Class Implementation
*/

#include "rendering/effects/PostProcessEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

// Standard fullscreen quad vertex shader
// Modernized for GLSL 3.30 Core Profile
static const char* fullscreenVertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 texCoord;

    out vec2 vTexCoord;

    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
        vTexCoord = texCoord;
    }
)";

const char* PostProcessEffect::getFullscreenVertexShader() { return fullscreenVertexShaderSource; }

bool PostProcessEffect::compileEffectShader(juce::OpenGLShaderProgram& program, const char* fragmentSource)
{
    if (!program.addVertexShader(fullscreenVertexShaderSource))
    {
        DBG("PostProcessEffect: Vertex shader failed: " << program.getLastError());
        return false;
    }

    if (!program.addFragmentShader(fragmentSource))
    {
        DBG("PostProcessEffect: Fragment shader failed: " << program.getLastError());
        return false;
    }

    // CRITICAL: Bind attributes to match FramebufferPool's fixed layout
    // FramebufferPool::renderFullscreenQuad() assumes location 0 is position and 1 is texCoord
    if (juce::OpenGLContext::getCurrentContext() != nullptr)
    {
        juce::OpenGLExtensionFunctions::glBindAttribLocation(program.getProgramID(), 0, "position");
        juce::OpenGLExtensionFunctions::glBindAttribLocation(program.getProgramID(), 1, "texCoord");
    }

    if (!program.link())
    {
        DBG("PostProcessEffect: Link failed: " << program.getLastError());
        return false;
    }

    return true;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
