/*
    Oscil - Waveform Render State Implementation
*/

#include "rendering/WaveformRenderState.h"
#include <algorithm>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

void WaveformRenderState::enableTrails(juce::OpenGLContext& context, int width, int height)
{
    if (trailsEnabled.load(std::memory_order_relaxed) && historyFBO && historyFBO->isValid())
    {
        // Already enabled, just resize if needed
        resizeHistoryFBO(context, width, height);
        return;
    }

    if (!historyFBO)
        historyFBO = std::make_unique<Framebuffer>();

    // Create history FBO with standard format (no depth needed)
    if (historyFBO->create(context, width, height, 0, GL_RGBA8, false))
    {
        trailsEnabled.store(true, std::memory_order_release);
        // Clear the history FBO initially
        historyFBO->bind();
        historyFBO->clear(juce::Colours::black, false);
        historyFBO->unbind();
        DBG("WaveformRenderState: Trails enabled for waveform " << waveformId);
    }
    else
    {
        DBG("WaveformRenderState: Failed to create history FBO for waveform " << waveformId);
    }
}

void WaveformRenderState::disableTrails(juce::OpenGLContext& context)
{
    if (historyFBO)
    {
        historyFBO->destroy(context);
        historyFBO.reset();
    }
    trailsEnabled.store(false, std::memory_order_release);
    DBG("WaveformRenderState: Trails disabled for waveform " << waveformId);
}

void WaveformRenderState::resizeHistoryFBO(juce::OpenGLContext& context, int width, int height)
{
    if (historyFBO && historyFBO->isValid())
    {
        if (historyFBO->width != width || historyFBO->height != height)
        {
            if (!historyFBO->resize(context, width, height))
                DBG("WaveformRenderState: historyFBO resize failed");
        }
    }
}

void WaveformRenderState::pushSamples(const std::vector<float>& samples)
{
    if (samples.empty())
        return;

    // Add new samples at the front
    sampleHistory.push_front(samples);

    // Remove old samples if we exceed max depth
    while (static_cast<int>(sampleHistory.size()) > maxHistoryDepth)
    {
        sampleHistory.pop_back();
    }
}

void WaveformRenderState::updateTiming(float deltaTime)
{
    lastFrameTime = deltaTime;
    accumulatedTime += deltaTime;

    // Wrap accumulated time to prevent overflow
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
    trailsEnabled.store(false, std::memory_order_release);
    sampleHistory.clear();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
