#pragma once

#include "FrameBudgetGuard.h"
#include "GLErrorGuard.h"
#include "GPUCapabilities.h"
#include "IEffectProvider.h"
#include "RenderCommon.h"
#include "VisualConfiguration.h"
#include "subsystems/EffectPipeline.h"
#include "subsystems/RenderBootstrapper.h"
#include "subsystems/RenderStats.h"
#include "subsystems/WaveformPass.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#if OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>

namespace oscil
{

// Forward declaration
struct WaveformRenderData;

/**
 * Entry in the waveform render batch.
 * C10 FIX: Stores waveform ID and copies of config data rather than raw pointers
 * to WaveformRenderState, avoiding dangling pointer issues if oscillators are
 * deleted during rendering. For operations requiring the actual state (like
 * trails history FBO), we look up by ID with validation.
 */
struct BatchEntry
{
    int waveformId = 0;                    // Unique ID for validation and lookup
    const WaveformRenderData* data = nullptr;  // Pointer to caller's data (valid within frame)
    VisualConfiguration visualConfig;      // Copy of config (WaveformRenderState has unique_ptr - not copyable)
    float lastFrameTime = 0.0f;           // Copy of timing state
    float accumulatedTime = 0.0f;         // Copy of timing state
    ShaderType shaderType = ShaderType::Basic2D;
    bool hasPostProcessing = false;
    size_t originalIndex = 0;  // Preserves original render order within groups
};

/**
 * Group of waveforms sharing the same shader type.
 * Enables single shader bind for multiple waveform draws.
 */
struct BatchGroup
{
    ShaderType shaderType = ShaderType::Basic2D;
    std::vector<BatchEntry> entries;
    
    void clear() { entries.clear(); }
    void reserve(size_t n) { entries.reserve(n); }
};

/**
 * Central orchestrator for all rendering subsystems.
 * Manages framebuffers, post-processing effects,
 * 3D rendering, and per-waveform visual states.
 *
 * THREAD SAFETY (H7):
 * This class is designed to be used EXCLUSIVELY from the OpenGL rendering thread.
 * - All rendering methods (beginFrame, renderWaveform, endFrame, flushBatch) must be
 *   called from the OpenGL thread that owns the context.
 * - OpenGL contexts are NOT thread-safe; only one thread may use a context at a time.
 * - The waveformStatesMutex_ protects waveformStates_ for cross-thread config updates
 *   from UI thread (setWaveformConfig) to render thread.
 * - Resize operations are deferred via atomics (pendingWidth_, pendingHeight_) to
 *   avoid racing with the render thread.
 *
 * CONTEXT OWNERSHIP:
 * - The OpenGL context is owned by WaveformComponent (via JUCE's OpenGLRenderer).
 * - RenderEngine holds a raw pointer to the context (context_) which is valid between
 *   initialize() and shutdown() calls.
 * - Never access context_ after shutdown() has been called.
 *
 * Error Handling:
 * - initialize() returns false if OpenGL context unavailable
 * - Automatic fallback to simpler OpenGL tiers (Baseline) on repeated GPU failures
 * - Check getCurrentTier() to determine current rendering capability
 * - syncWaveforms() should be called to clean up stale waveform entries
 */
class RenderEngine : public IEffectProvider
{
public:
    RenderEngine();
    ~RenderEngine() override;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(juce::OpenGLContext& context);
    void shutdown();
    void resize(int width, int height);
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Per-Frame Rendering
    // ========================================================================

    void beginFrame(float deltaTime);
    void renderWaveform(const WaveformRenderData& data);
    void endFrame();

    // ========================================================================
    // Batched Rendering API (reduces state changes)
    // ========================================================================

    /**
     * Begin collecting waveforms for batched rendering.
     * Call this once at the start of the frame after beginFrame().
     */
    void beginBatch();

    /**
     * Add a waveform to the current batch.
     * Waveforms will be sorted and rendered efficiently in flushBatch().
     * @param data The waveform render data to add
     */
    void addToBatch(const WaveformRenderData& data);

    /**
     * Render all collected waveforms with optimized state changes.
     * Groups waveforms by shader type and minimizes FBO/shader binds.
     */
    void flushBatch();

    // ========================================================================
    // Waveform State Management
    // ========================================================================

    void registerWaveform(int waveformId);
    void unregisterWaveform(int waveformId);
    [[nodiscard]] std::optional<VisualConfiguration> getWaveformConfig(int waveformId);
    [[nodiscard]] bool hasWaveform(int waveformId);
    void setWaveformConfig(int waveformId, const VisualConfiguration& config);
    void clearAllWaveforms();
    void syncWaveforms(const std::unordered_set<int>& activeIds);

    // ========================================================================
    // Subsystem Access
    // ========================================================================

    Camera3D* getCamera() const { return waveformPass_->getCamera(); }
    FramebufferPool* getFramebufferPool() const { return effectPipeline_->getFramebufferPool(); }
    EnvironmentMapManager* getEnvironmentMapManager() const { return waveformPass_->getEnvironmentMapManager(); }
    TextureManager* getTextureManager() const { return waveformPass_->getTextureManager(); }

    // ========================================================================
    // Global Settings
    // ========================================================================

    void setQualityLevel(QualityLevel level);
    [[nodiscard]] QualityLevel getQualityLevel() const { return qualityLevel_; }

    void setBackgroundColour(juce::Colour colour) { backgroundColour_ = colour; }
    [[nodiscard]] juce::Colour getBackgroundColour() const { return backgroundColour_; }

    [[nodiscard]] float getDeltaTime() const { return stats_.getDeltaTime(); }
    [[nodiscard]] float getTime() const { return stats_.getTime(); }

    // ========================================================================
    // Effect Management
    // ========================================================================

    PostProcessEffect* getEffect(const juce::String& effectId) override;

    void setGlobalPostProcessingEnabled(bool enabled);
    [[nodiscard]] bool isGlobalPostProcessingEnabled() const;

    // ========================================================================
    // Frame Budget Management
    // ========================================================================

    /**
     * Set the target frame time budget in milliseconds.
     * Default is 16.67ms (60 FPS).
     */
    void setFrameBudgetMs(double targetMs);

    /**
     * Get statistics from the last frame's budget tracking.
     */
    [[nodiscard]] FrameBudgetGuard::FrameStats getLastFrameBudgetStats() const;

    // ========================================================================
    // Tiered Rendering & Fallback System
    // ========================================================================

    /**
     * Get the current rendering tier.
     * Higher tiers use more advanced GPU features.
     */
    [[nodiscard]] RenderTier getCurrentTier() const { return errorStats_.currentTier.load(std::memory_order_acquire); }

    /**
     * Set the rendering tier manually.
     * Use for testing or user preference override.
     */
    void setRenderTier(RenderTier tier) { errorStats_.currentTier.store(tier, std::memory_order_release); }

    /**
     * Check if a fallback has been triggered due to GL errors.
     */
    [[nodiscard]] bool isFallbackTriggered() const { return errorStats_.fallbackTriggered.load(std::memory_order_acquire); }

    /**
     * Reset fallback state (call after recovering from errors).
     */
    void resetFallbackState() { errorStats_.reset(); }

    /**
     * Get GL error statistics for diagnostics.
     */
    [[nodiscard]] const GLErrorStats& getErrorStats() const { return errorStats_; }

    /**
     * Get detected GPU capabilities.
     * Valid only after initialize() has been called.
     */
    [[nodiscard]] const GPUCapabilities& getGPUCapabilities() const { return gpuCapabilities_; }

    /**
     * Trigger a fallback to the baseline rendering tier.
     * Called automatically when GL errors are detected.
     */
    void triggerFallback();

    /**
     * Record a GL error for diagnostics.
     */
    void recordGLError(const char* operation, GLenum error);

    // Prevent copying
    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;

private:
    // Internal rendering methods
    void blitToScreen();
    void executeComposite(Framebuffer* source, const VisualConfiguration& config);
    void applyPendingResize();  // C11 FIX: Apply deferred resize at frame start

    // Batched rendering methods
    void sortBatchByShaderType();
    void renderBatchedGrids();
    void renderBatchGroup(BatchGroup& group);
    void renderBatchGroupInstanced(BatchGroup& group);  // Tier 2: Instanced rendering
    void renderSingleWaveform(const BatchEntry& entry);

    // Subsystems
    std::unique_ptr<RenderBootstrapper> bootstrapper_;
    std::unique_ptr<EffectPipeline> effectPipeline_;
    std::unique_ptr<WaveformPass> waveformPass_;
    RenderStats stats_;

    // Per-waveform states (protected by waveformStatesMutex_)
    mutable juce::SpinLock waveformStatesMutex_;
    std::unordered_map<int, WaveformRenderState> waveformStates_;

    // Context and state
    juce::OpenGLContext* context_ = nullptr;
    QualityLevel qualityLevel_ = QualityLevel::Normal;
    juce::Colour backgroundColour_{juce::Colours::transparentBlack};

    // Flags
    bool initialized_ = false;

    // Current frame state
    int currentWidth_ = 0;
    int currentHeight_ = 0;
    std::chrono::high_resolution_clock::time_point frameStartTime_;

    // C11 FIX: Pending resize state (atomics for thread-safe deferred resize)
    // UI thread sets pending dimensions, render thread applies at frame start
    std::atomic<int> pendingWidth_{0};
    std::atomic<int> pendingHeight_{0};
    std::atomic<bool> resizePending_{false};

    // ========================================================================
    // Batch Rendering State (pre-allocated to avoid runtime allocations)
    // ========================================================================
    
    static constexpr size_t kMaxBatchSize = 32;  // Typical max waveforms
    
    // All batch entries collected during addToBatch()
    std::vector<BatchEntry> batchEntries_;
    
    // Groups organized by shader type for efficient rendering
    std::unordered_map<ShaderType, BatchGroup> batchGroups_;
    
    // Sorted order of shader types for deterministic rendering
    std::vector<ShaderType> batchShaderOrder_;
    
    // Whether we're currently collecting a batch
    bool batchActive_ = false;

    // Reused vectors for renderBatchGroup/renderBatchGroupInstanced (avoids per-batch allocation)
    std::vector<const BatchEntry*> noPPEntries_;
    std::vector<const BatchEntry*> withPPEntries_;
    std::unordered_map<BlendMode, std::vector<const BatchEntry*>> blendGroups_;

    // ========================================================================
    // Frame Budget Guard (prevents UI freezes)
    // ========================================================================
    
    FrameBudgetGuard frameBudgetGuard_;
    FrameBudgetGuard::Config budgetConfig_;

    // ========================================================================
    // GL Error Tracking & Fallback System
    // ========================================================================
    
    GLErrorStats errorStats_;
    GPUCapabilities gpuCapabilities_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
