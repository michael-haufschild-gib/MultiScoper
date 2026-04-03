/*
    Oscil - Waveform Render State Implementation
*/

#include "rendering/WaveformRenderState.h"

#if OSCIL_ENABLE_OPENGL

using namespace juce::gl;

namespace oscil
{

void WaveformRenderState::enableTrails(juce::OpenGLContext& context, int width, int height)
{
    if (trailsEnabled && historyFBO && historyFBO->isValid())
    {
        resizeHistoryFBO(context, width, height);
        return;
    }

    if (!historyFBO)
        historyFBO = std::make_unique<Framebuffer>();

    if (historyFBO->create(context, width, height, 0, GL_RGBA8, false))
    {
        trailsEnabled = true;
        historyFBO->bind();
        historyFBO->clear(juce::Colours::black, false);
        historyFBO->unbind();
    }
}

void WaveformRenderState::disableTrails(juce::OpenGLContext& context)
{
    if (historyFBO)
    {
        historyFBO->destroy(context);
        historyFBO.reset();
    }
    trailsEnabled = false;
}

void WaveformRenderState::resizeHistoryFBO(juce::OpenGLContext& context, int width, int height) const
{
    if (historyFBO && historyFBO->isValid())
    {
        if (historyFBO->width != width || historyFBO->height != height)
            historyFBO->resize(context, width, height);
    }
}

void WaveformRenderState::updateTiming(float deltaTime)
{
    lastFrameTime = deltaTime;
    accumulatedTime += deltaTime;

    if (accumulatedTime > 1000.0f)
        accumulatedTime = std::fmod(accumulatedTime, 1000.0f);
}

void WaveformRenderState::release(juce::OpenGLContext& context)
{
    if (historyFBO)
    {
        historyFBO->destroy(context);
        historyFBO.reset();
    }
    trailsEnabled = false;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
