/*
    Oscil - Metal Compute Backend
    GPU compute shader support for Apple Silicon (Tier 4)
    
    This backend is only available on macOS with Apple Silicon.
    It requires user opt-in and host compatibility checks.
*/

#pragma once

#include <juce_core/juce_core.h>

#if JUCE_MAC

namespace oscil
{

// Forward declarations
struct WaveformRenderData;
struct WaveformRenderState;
class GPUCapabilities;

/**
 * Metal compute backend for Apple Silicon.
 * Provides GPU-accelerated waveform processing using Metal compute shaders.
 * 
 * IMPORTANT: This is an opt-in feature that may cause issues with some hosts.
 * Only enabled when:
 * 1. Running on Apple Silicon
 * 2. User explicitly enables "GPU Compute" in settings
 * 3. Host is not in the problematic hosts list
 * 
 * Thread Safety:
 * - All methods must be called from the main/render thread
 * - Audio thread must NEVER call any methods on this class
 */
class MetalComputeBackend
{
public:
    MetalComputeBackend();
    ~MetalComputeBackend();

    /**
     * Check if Metal compute is available on this system.
     * @return true if Apple Silicon with Metal 3+ support
     */
    static bool isAvailable();

    /**
     * Initialize the Metal compute pipeline.
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * Shutdown and release all Metal resources.
     */
    void shutdown();

    /**
     * Check if the backend is initialized and ready for use.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    /**
     * Process waveform samples and generate vertex data.
     * This is the main compute operation.
     * 
     * @param samples Pointer to sample data (already on CPU, will be uploaded)
     * @param sampleCount Number of samples
     * @param waveformId ID of the waveform being processed
     * @param config Visual configuration for the waveform
     * @return true if processing succeeded
     */
    bool processWaveform(const float* samples, 
                         size_t sampleCount,
                         int waveformId,
                         const WaveformRenderState& config);

    /**
     * Get the processed vertex data for rendering.
     * Call this after processWaveform() to get the GPU-generated vertices.
     * 
     * @param waveformId ID of the waveform
     * @param vertexCount Output: number of vertices generated
     * @return Pointer to vertex data (valid until next processWaveform call)
     */
    const float* getVertexData(int waveformId, size_t& vertexCount) const;

    /**
     * Get performance statistics.
     */
    struct Stats
    {
        double lastComputeTimeMs = 0.0;
        double avgComputeTimeMs = 0.0;
        uint64_t totalComputeCalls = 0;
    };
    [[nodiscard]] Stats getStats() const { return stats_; }

private:
    // Metal implementation is in the .mm file (Objective-C++)
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    bool initialized_ = false;
    Stats stats_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalComputeBackend)
};

} // namespace oscil

#endif // JUCE_MAC













