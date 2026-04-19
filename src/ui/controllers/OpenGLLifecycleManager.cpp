/*
    MultiScoper - OpenGL Lifecycle Manager Implementation
*/

#include "ui/controllers/OpenGLLifecycleManager.h"

#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"

namespace multiscoper
{

OpenGLLifecycleManager::OpenGLLifecycleManager(juce::AudioProcessorEditor& editor) : editor_(editor)
{
#if MULTISCOPER_ENABLE_OPENGL
    renderer_ = std::make_unique<WaveformGLRenderer>();
    renderer_->setContext(&context_);
#endif
}

OpenGLLifecycleManager::~OpenGLLifecycleManager() { detach(); }

void OpenGLLifecycleManager::detach()
{
#if MULTISCOPER_ENABLE_OPENGL
    if (!isDetached_)
    {
        context_.detach();
        isDetached_ = true;
    }
#endif
}

void OpenGLLifecycleManager::setGpuRenderingEnabled(bool enabled)
{
#if MULTISCOPER_ENABLE_OPENGL
    gpuRenderingEnabled_ = enabled;

    if (enabled && !context_.isAttached())
    {
        // Configure pixel format for transparency and quality
        juce::OpenGLPixelFormat format;
        format.redBits = 8;
        format.greenBits = 8;
        format.blueBits = 8;
        format.alphaBits = 8;
        format.depthBufferBits = 24;
        format.stencilBufferBits = 8;
        format.multisamplingLevel = 0; // We handle AA in shaders/post-proc

        context_.setPixelFormat(format);
        context_.setRenderer(renderer_.get());
        // Continuous repainting drives a VSync-rate redraw even when nothing
        // has changed — 16 idle editors cost a full core that way.  We drive
        // repaints explicitly from the editor's 60Hz timer when waveform
        // data actually changed (see triggerRepaint() / updateWaveformData()).
        context_.setContinuousRepainting(false);
        context_.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
        context_.attachTo(editor_);
        isDetached_ = false;
        DBG("GPU rendering enabled - OpenGL context attached");
    }
    else if (!enabled && context_.isAttached() && !isDetached_)
    {
        context_.detach();
        isDetached_ = true;
        DBG("GPU rendering disabled - OpenGL context detached");
    }
#else
    juce::ignoreUnused(enabled);
#endif
}

void OpenGLLifecycleManager::clearAllWaveforms()
{
#if MULTISCOPER_ENABLE_OPENGL
    if (renderer_)
        renderer_->clearAllWaveforms();
#endif
}

bool OpenGLLifecycleManager::updateWaveformData(const std::vector<std::unique_ptr<PaneComponent>>& paneComponents)
{
#if MULTISCOPER_ENABLE_OPENGL
    if (!renderer_ || !gpuRenderingEnabled_)
        return false;

    constexpr float kSilenceEpsilon = 1.0e-4f;
    bool anySignal = false;

    for (const auto& pane : paneComponents)
    {
        if (!pane)
            continue;

        for (size_t i = 0; i < pane->getOscillatorCount(); ++i)
        {
            auto* waveform = pane->getWaveformAt(i);
            if (!waveform)
                continue;

            waveform->forceUpdateWaveformData();

            WaveformRenderData data;
            waveform->populateGLRenderData(data);

            renderer_->registerWaveform(data.id);
            renderer_->updateWaveform(data);

            if (waveform->getPeakLevel() > kSilenceEpsilon)
                anySignal = true;
        }
    }

    return anySignal;
#else
    juce::ignoreUnused(paneComponents);
    return false;
#endif
}

void OpenGLLifecycleManager::triggerRepaint()
{
#if MULTISCOPER_ENABLE_OPENGL
    if (gpuRenderingEnabled_ && context_.isAttached())
        context_.triggerRepaint();
#endif
}

void OpenGLLifecycleManager::setBackgroundColour(juce::Colour colour)
{
#if MULTISCOPER_ENABLE_OPENGL
    if (renderer_)
        renderer_->setBackgroundColour(colour);
#else
    juce::ignoreUnused(colour);
#endif
}

} // namespace multiscoper
