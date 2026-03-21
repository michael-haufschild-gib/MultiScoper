/*
    Oscil - Waveform GL Renderer - Rendering Methods
    OpenGL render loop and debug visualization
*/

#include "rendering/WaveformGLRenderer.h"

#if OSCIL_ENABLE_OPENGL

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"
#include "rendering/RenderEngine.h"
#include <unordered_set>

// Debug-only logging macro — no output in release builds
#define GL_LOG(msg) DBG("[GL] " << msg)

namespace oscil
{

using namespace juce::gl;

static constexpr bool DEBUG_RENDER_MODE = false;
std::vector<WaveformRenderData> WaveformGLRenderer::collectWaveformsToRender()
{
    std::vector<WaveformRenderData> result;
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    result.reserve(waveforms_.size());
    for (const auto& pair : waveforms_)
    {
        if constexpr (DEBUG_RENDER_MODE)
        {
            if (!pair.second.bounds.isEmpty())
                result.push_back(pair.second);
        }
        else
        {
            if (pair.second.visible)
                result.push_back(pair.second);
        }
    }
    return result;
}

void WaveformGLRenderer::renderWithEngine(const std::vector<WaveformRenderData>& waveformsToRender,
                                           float deltaTime)
{
    const juce::ScopedWriteLock lock(engineLock_);

    if (renderEngine_ && renderEngine_->isInitialized())
    {
        {
            std::unordered_set<int> activeIds;
            for (const auto& data : waveformsToRender)
                activeIds.insert(data.id);
            renderEngine_->syncWaveforms(activeIds);
        }

        renderEngine_->beginFrame(deltaTime);

        for (const auto& data : waveformsToRender)
        {
            if (!renderEngine_->hasWaveform(data.id))
                renderEngine_->registerWaveform(data.id);
            renderEngine_->setWaveformConfig(data.id, data.visualConfig);
            renderEngine_->renderWaveform(data);
        }

        renderEngine_->endFrame();
    }
    else
    {
        juce::OpenGLHelpers::clear(backgroundColour_);
    }
}

void WaveformGLRenderer::renderOpenGL()
{
    if (!contextReady_.load())
        return;

    jassert(juce::OpenGLHelpers::isContextActive());
    juce::OpenGLHelpers::clear(juce::Colours::transparentBlack);

    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::min(std::chrono::duration<float>(now - lastFrameTime_).count(), 0.1f);
    lastFrameTime_ = now;

    const float desktopScale = static_cast<float>(context_->getRenderingScale());
    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent) return;

    auto width = static_cast<GLsizei>(static_cast<float>(targetComponent->getWidth()) * desktopScale);
    auto height = static_cast<GLsizei>(static_cast<float>(targetComponent->getHeight()) * desktopScale);
    glViewport(0, 0, width, height);

    if (renderEngine_ && renderEngine_->isInitialized())
    {
        if (width != lastResizeWidth_ || height != lastResizeHeight_)
        {
            renderEngine_->resize(width, height);
            lastResizeWidth_ = width;
            lastResizeHeight_ = height;
        }
    }

    auto waveformsToRender = collectWaveformsToRender();

    if constexpr (DEBUG_RENDER_MODE)
    {
        juce::OpenGLHelpers::clear(backgroundColour_);
        renderDebugRect(juce::Rectangle<float>(50.0f, 50.0f, 200.0f, 200.0f), juce::Colour(0xFFFF0000));
        renderDebugRect(juce::Rectangle<float>(300.0f, 100.0f, 150.0f, 150.0f), juce::Colour(0xFF0000FF));
        for (const auto& data : waveformsToRender)
            renderDebugRect(data.bounds, data.colour);
    }
    else
    {
        renderWithEngine(waveformsToRender, deltaTime);
    }
}

void WaveformGLRenderer::setupDebugProjection(juce::OpenGLExtensionFunctions& ext,
                                               GLint projLoc, GLint colorLoc,
                                               float viewportWidth, float viewportHeight,
                                               juce::Colour colour)
{
    float projection[16] = {
        2.0f / viewportWidth, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / viewportHeight, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    ext.glUniform4f(colorLoc, colour.getFloatRed(), colour.getFloatGreen(),
                    colour.getFloatBlue(), 1.0f);
}

void WaveformGLRenderer::renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour)
{
    if (!debugShader_ || !debugShaderCompiled_ || !context_)
        return;

    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent) return;

    auto& ext = context_->extensions;

    float viewportWidth = std::max(1.0f, static_cast<float>(targetComponent->getWidth()));
    float viewportHeight = std::max(1.0f, static_cast<float>(targetComponent->getHeight()));

    float x1 = bounds.getX(), y1 = bounds.getY();
    float x2 = bounds.getRight(), y2 = bounds.getBottom();
    float vertices[] = { x1,y1, x2,y1, x1,y2, x2,y1, x2,y2, x1,y2 };

    debugShader_->use();

    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);

    GLint projLoc = ext.glGetUniformLocation(static_cast<GLuint>(programID), "projection");
    GLint colorLoc = ext.glGetUniformLocation(static_cast<GLuint>(programID), "color");

    setupDebugProjection(ext, projLoc, colorLoc, viewportWidth, viewportHeight, colour);

    glDisable(GL_DEPTH_TEST);
    while (glGetError() != GL_NO_ERROR) {}

    ext.glBindVertexArray(debugVAO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
    ext.glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    GLuint posAttrib = static_cast<GLuint>(positionLoc >= 0 ? positionLoc : 0);
    ext.glEnableVertexAttribArray(posAttrib);
    ext.glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    ext.glDisableVertexAttribArray(posAttrib);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
