/*
    Oscil - Post-Process Effect Base Class Implementation
*/

#include "rendering/effects/PostProcessEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

// Standard fullscreen quad vertex shader
// Uses legacy GLSL syntax - JUCE's translateVertexShaderToV3 handles platform compatibility
static const char* fullscreenVertexShaderSource = R"(
    attribute vec2 position;
    attribute vec2 texCoord;

    varying vec2 vTexCoord;

    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
        vTexCoord = texCoord;
    }
)";

const char* PostProcessEffect::getFullscreenVertexShader()
{
    return fullscreenVertexShaderSource;
}

bool PostProcessEffect::compileEffectShader(
    juce::OpenGLShaderProgram& program,
    const char* fragmentSource)
{
    // Translate shaders for cross-platform compatibility
    juce::String translatedVertex =
        juce::OpenGLHelpers::translateVertexShaderToV3(fullscreenVertexShaderSource);
    juce::String translatedFragment =
        juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentSource);

    if (!program.addVertexShader(translatedVertex))
    {
        DBG("PostProcessEffect: Vertex shader failed: " << program.getLastError());
        return false;
    }

    if (!program.addFragmentShader(translatedFragment))
    {
        DBG("PostProcessEffect: Fragment shader failed: " << program.getLastError());
        return false;
    }

    // CRITICAL: Bind attributes to match FramebufferPool's fixed layout
    // FramebufferPool::renderFullscreenQuad() assumes location 0 is position and 1 is texCoord
    if (auto* context = juce::OpenGLContext::getCurrentContext())
    {
        context->extensions.glBindAttribLocation(program.getProgramID(), 0, "position");
        context->extensions.glBindAttribLocation(program.getProgramID(), 1, "texCoord");
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
