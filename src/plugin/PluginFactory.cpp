/*
    Oscil - Plugin Factory Implementation
    Composition root for dependency injection.
*/

#include "plugin/PluginFactory.h"
#include "core/AudioCapturePool.h"
#include "core/CaptureThread.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/OscilState.h"  // for GlobalPreferences
#include "core/VisualPresetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include <atomic>

namespace oscil
{

namespace
{
    // The single factory instance - composition root
    // Using atomic for thread-safe access from multiple threads
    std::atomic<PluginFactory*> currentFactory{nullptr};

    // Flag to track if static destruction has begun
    // This prevents access to destroyed static objects
    std::atomic<bool> staticDestructionStarted{false};
}

PluginFactory::PluginFactory()
    : themeManager_(std::make_unique<ThemeManager>())
    , instanceRegistry_(std::make_unique<InstanceRegistry>())
    , shaderRegistry_(std::make_unique<ShaderRegistry>())
    , memoryBudgetManager_(std::make_unique<MemoryBudgetManager>())
    , globalPreferences_(std::make_unique<GlobalPreferences>())
    , visualPresetManager_(std::make_unique<VisualPresetManager>())
    , audioCapturePool_(std::make_unique<AudioCapturePool>())
    , captureThread_(nullptr)  // Created after pool
{
    // Initialize the preset manager
    visualPresetManager_->initialize();

    // Create capture thread with reference to pool
    // Thread is started here and shared across all plugin instances
    captureThread_ = std::make_unique<CaptureThread>(*audioCapturePool_);
    captureThread_->startCapturing();
}

PluginFactory::~PluginFactory()
{
    // H46 FIX: Mark static destruction as started to prevent dangling access
    staticDestructionStarted.store(true, std::memory_order_release);

    // H46 FIX: Clear currentFactory if it points to this instance
    // This prevents other code from accessing a destroyed factory
    PluginFactory* expected = this;
    currentFactory.compare_exchange_strong(expected, nullptr,
        std::memory_order_acq_rel, std::memory_order_acquire);

    // CRITICAL: Stop capture thread BEFORE destroying the pool
    // This ensures no thread is accessing the pool during destruction
    if (captureThread_)
    {
        captureThread_->stopCapturing();
    }
    // Unique_ptrs handle destruction in correct order (reverse of initialization)
}

PluginFactory& PluginFactory::getInstance()
{
    // H46 FIX: Check if static destruction has started
    // This prevents access to destroyed static objects during shutdown
    if (staticDestructionStarted.load(std::memory_order_acquire))
    {
        // During static destruction, we cannot safely return a reference
        // This should only happen during program exit, not normal operation
        // Log and return the existing factory if still valid
        PluginFactory* factory = currentFactory.load(std::memory_order_acquire);
        if (factory != nullptr)
        {
            return *factory;
        }
        // If we get here during static destruction with no valid factory,
        // this is a programming error - but we must return something
        // Create a thread-local fallback to avoid UB
        static thread_local PluginFactory fallback;
        return fallback;
    }

    // Lazy initialization - create on first access
    // This is the ONLY singleton pattern in the system
    static PluginFactory instance;

    // Thread-safe check and set using compare_exchange
    PluginFactory* expected = nullptr;
    currentFactory.compare_exchange_strong(expected, &instance,
        std::memory_order_acq_rel, std::memory_order_acquire);

    return *currentFactory.load(std::memory_order_acquire);
}

void PluginFactory::setInstance(PluginFactory* factory)
{
    // Allow overriding for testing
    static PluginFactory defaultInstance;
    currentFactory.store(factory ? factory : &defaultInstance, std::memory_order_release);
}

std::unique_ptr<juce::AudioProcessor> PluginFactory::createPluginProcessor()
{
    return std::make_unique<OscilPluginProcessor>(
        *instanceRegistry_,
        *themeManager_,
        *shaderRegistry_,
        *memoryBudgetManager_,
        *audioCapturePool_,
        *captureThread_
    );
}

ThemeManager& PluginFactory::getThemeManager()
{
    return *themeManager_;
}

InstanceRegistry& PluginFactory::getInstanceRegistry()
{
    return *instanceRegistry_;
}

ShaderRegistry& PluginFactory::getShaderRegistry()
{
    return *shaderRegistry_;
}

MemoryBudgetManager& PluginFactory::getMemoryBudgetManager()
{
    return *memoryBudgetManager_;
}

GlobalPreferences& PluginFactory::getGlobalPreferences()
{
    return *globalPreferences_;
}

VisualPresetManager& PluginFactory::getVisualPresetManager()
{
    return *visualPresetManager_;
}

AudioCapturePool& PluginFactory::getAudioCapturePool()
{
    return *audioCapturePool_;
}

CaptureThread& PluginFactory::getCaptureThread()
{
    return *captureThread_;
}

} // namespace oscil