/*
    Oscil - OpenGL Error Guard
    RAII wrapper for GL error checking with automatic fallback mechanism
*/

#pragma once

#include <juce_core/juce_core.h>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>

namespace oscil
{

using namespace juce::gl;

// Forward declaration
class RenderEngine;

/**
 * Rendering tier levels for fallback system.
 * Higher tiers require more advanced GPU features.
 */
enum class RenderTier
{
    Baseline = 0,   // OpenGL 3.2 - guaranteed to work everywhere
    Instanced = 1,  // OpenGL 3.3+ - instanced rendering with UBOs
    SSBO = 2,       // OpenGL 4.3+ - shader storage buffer objects
    Compute = 3     // OpenGL 4.3+ or Metal - compute shaders (opt-in)
};

/**
 * Converts GLenum error code to human-readable string
 */
inline const char* glErrorToString(GLenum err)
{
    switch (err)
    {
        case GL_NO_ERROR:          return "GL_NO_ERROR";
        case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:                   return "UNKNOWN_GL_ERROR";
    }
}

/**
 * Statistics about GL errors encountered during rendering.
 * Used for diagnostics and deciding when to trigger fallback.
 */
struct GLErrorStats
{
    std::atomic<uint32_t> totalErrors{0};
    std::atomic<uint32_t> errorsThisFrame{0};
    std::atomic<uint32_t> consecutiveErrorFrames{0};
    std::atomic<bool> fallbackTriggered{false};
    std::atomic<RenderTier> currentTier{RenderTier::Baseline};
    
    // Last error info (for logging)
    std::atomic<GLenum> lastError{GL_NO_ERROR};
    juce::String lastOperation;
    mutable juce::SpinLock lastOperationLock;
    
    void recordError(GLenum err, const char* operation)
    {
        totalErrors.fetch_add(1, std::memory_order_relaxed);
        errorsThisFrame.fetch_add(1, std::memory_order_relaxed);
        lastError.store(err, std::memory_order_relaxed);
        
        {
            juce::SpinLock::ScopedLockType lock(lastOperationLock);
            lastOperation = operation;
        }
    }
    
    void beginFrame()
    {
        uint32_t errorsLastFrame = errorsThisFrame.exchange(0, std::memory_order_relaxed);
        if (errorsLastFrame > 0)
        {
            consecutiveErrorFrames.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            consecutiveErrorFrames.store(0, std::memory_order_relaxed);
        }
    }
    
    void reset()
    {
        totalErrors.store(0, std::memory_order_relaxed);
        errorsThisFrame.store(0, std::memory_order_relaxed);
        consecutiveErrorFrames.store(0, std::memory_order_relaxed);
        fallbackTriggered.store(false, std::memory_order_relaxed);
        lastError.store(GL_NO_ERROR, std::memory_order_relaxed);
    }
    
    [[nodiscard]] bool shouldTriggerFallback() const
    {
        // Trigger fallback if we have errors for 3+ consecutive frames
        // or if we hit a critical error like OUT_OF_MEMORY
        return consecutiveErrorFrames.load(std::memory_order_relaxed) >= 3 ||
               lastError.load(std::memory_order_relaxed) == GL_OUT_OF_MEMORY;
    }
};

/**
 * RAII guard for OpenGL error checking.
 * 
 * Clears pending errors on construction, checks for new errors on destruction.
 * If errors are detected, records them and optionally triggers tier fallback.
 * 
 * Usage:
 * ```cpp
 * void RenderEngine::renderWaveform(...) {
 *     GLErrorGuard guard(errorStats_, "renderWaveform");
 *     // ... OpenGL calls ...
 * } // Errors checked here automatically
 * ```
 */
class GLErrorGuard
{
public:
    /**
     * Constructs the guard and clears any pending GL errors.
     * @param stats Reference to error statistics tracker
     * @param operation Name of the operation being guarded (for logging)
     * @param criticalOperation If true, errors will trigger immediate fallback
     */
    GLErrorGuard(GLErrorStats& stats, const char* operation, bool criticalOperation = false)
        : stats_(stats)
        , operation_(operation)
        , criticalOperation_(criticalOperation)
    {
        // Clear any pending errors from previous operations
        clearPendingErrors();
    }
    
    ~GLErrorGuard()
    {
        checkErrors();
    }
    
    // Non-copyable, non-movable
    GLErrorGuard(const GLErrorGuard&) = delete;
    GLErrorGuard& operator=(const GLErrorGuard&) = delete;
    GLErrorGuard(GLErrorGuard&&) = delete;
    GLErrorGuard& operator=(GLErrorGuard&&) = delete;
    
    /**
     * Manually check for errors mid-operation.
     * Useful for pinpointing which specific GL call caused an error.
     * @return true if no errors, false if errors were detected
     */
    [[nodiscard]] bool checkNow()
    {
        return checkErrors();
    }
    
    /**
     * Check if any errors have been detected so far.
     */
    [[nodiscard]] bool hasError() const { return errorDetected_; }
    
private:
    void clearPendingErrors()
    {
        // Consume all pending errors (max 100 to avoid infinite loop)
        int cleared = 0;
        while (glGetError() != GL_NO_ERROR && cleared < 100)
        {
            ++cleared;
        }
    }
    
    bool checkErrors()
    {
        using namespace juce::gl;
        
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return true;
        
        errorDetected_ = true;
        
        // Record the first error
        stats_.recordError(err, operation_);
        
        // Log the error (throttled to avoid spam)
        // Use thread_local to avoid ODR issues with static in header
        thread_local int logCounter = 0;
        if (++logCounter % 60 == 0)
        {
            DBG("GLErrorGuard: Error in " << operation_ << ": " << glErrorToString(err));
        }
        
        // Consume any additional errors
        while (glGetError() != GL_NO_ERROR) {}
        
        // Critical errors trigger immediate fallback flag
        if (criticalOperation_ || err == GL_OUT_OF_MEMORY)
        {
            stats_.fallbackTriggered.store(true, std::memory_order_release);
        }
        
        return false;
    }
    
    GLErrorStats& stats_;
    const char* operation_;
    bool criticalOperation_;
    bool errorDetected_ = false;
};

/**
 * Scoped tier override for testing fallback behavior.
 * Temporarily forces a specific rendering tier.
 */
class ScopedTierOverride
{
public:
    ScopedTierOverride(GLErrorStats& stats, RenderTier tier)
        : stats_(stats)
        , previousTier_(stats.currentTier.load(std::memory_order_relaxed))
    {
        stats_.currentTier.store(tier, std::memory_order_release);
    }
    
    ~ScopedTierOverride()
    {
        stats_.currentTier.store(previousTier_, std::memory_order_release);
    }
    
private:
    GLErrorStats& stats_;
    RenderTier previousTier_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL

