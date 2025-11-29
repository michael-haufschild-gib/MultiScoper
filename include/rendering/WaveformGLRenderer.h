/*
    Oscil - Waveform OpenGL Renderer
    Implements juce::OpenGLRenderer for GPU-accelerated waveform rendering
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>
#include "rendering/WaveformShader.h"
#include <vector>
#include <unordered_map>
#include <atomic>

namespace oscil
{

// Forward declarations
class ShaderRegistry;

/**
 * Data required to render a single waveform on the GL thread.
 * This struct is copied to ensure thread-safety between UI and GL threads.
 */
struct WaveformRenderData
{
    int id = 0;                          // Unique identifier
    juce::Rectangle<float> bounds;       // Screen position and size
    std::vector<float> channel1;         // First channel samples
    std::vector<float> channel2;         // Second channel (stereo only)
    juce::Colour colour{ 0xFF00FF00 };   // Waveform color
    float opacity = 1.0f;                // Opacity (0-1)
    float lineWidth = 1.5f;              // Line thickness
    bool isStereo = false;               // Whether to render channel2
    juce::String shaderId = "neon_glow"; // Shader to use
    bool visible = true;                 // Whether to render this waveform
};

/**
 * OpenGL renderer for waveform visualizations.
 * Implements juce::OpenGLRenderer to handle GPU rendering on a background thread.
 *
 * Thread Safety:
 * - updateWaveform() is called from the message thread
 * - renderOpenGL() is called from the OpenGL thread
 * - Uses SpinLock for fast, non-blocking synchronization
 */
class WaveformGLRenderer : public juce::OpenGLRenderer
{
public:
    WaveformGLRenderer();
    ~WaveformGLRenderer() override;

    // juce::OpenGLRenderer interface
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    /**
     * Set the OpenGL context (must be called before rendering)
     */
    void setContext(juce::OpenGLContext* context);

    /**
     * Register a waveform for rendering
     * @param id Unique identifier for this waveform
     */
    void registerWaveform(int id);

    /**
     * Unregister a waveform
     * @param id The waveform identifier
     */
    void unregisterWaveform(int id);

    /**
     * Update waveform data for rendering (thread-safe)
     * Call this from the message thread when waveform data changes
     * @param data The render data to update
     */
    void updateWaveform(const WaveformRenderData& data);

    /**
     * Set the background color for clearing
     */
    void setBackgroundColour(juce::Colour colour);

    /**
     * Check if renderer is ready for GPU rendering
     */
    bool isReady() const { return contextReady_.load(); }

    /**
     * Get number of registered waveforms
     */
    int getWaveformCount() const;

private:
    void compileShaders();
    void renderWaveform(const WaveformRenderData& data);

    juce::OpenGLContext* context_ = nullptr;
    std::atomic<bool> contextReady_{ false };

    // Thread-safe waveform data storage
    // Using SpinLock for fast synchronization (short critical sections)
    juce::SpinLock dataLock_;
    std::unordered_map<int, WaveformRenderData> waveforms_;

    // Render state
    juce::Colour backgroundColour_{ 0xFF1A1A1A };
    bool shadersCompiled_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGLRenderer)
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
