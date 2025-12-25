/*
    Oscil - Plugin Factory Implementation
    Composition root for dependency injection.
*/

#include "plugin/PluginFactory.h"
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
}

PluginFactory::PluginFactory()
    : themeManager_(std::make_unique<ThemeManager>())
    , instanceRegistry_(std::make_unique<InstanceRegistry>())
    , shaderRegistry_(std::make_unique<ShaderRegistry>())
    , memoryBudgetManager_(std::make_unique<MemoryBudgetManager>())
    , globalPreferences_(std::make_unique<GlobalPreferences>())
    , visualPresetManager_(std::make_unique<VisualPresetManager>())
{
    // Initialize the preset manager
    visualPresetManager_->initialize();
}

PluginFactory::~PluginFactory() = default;

PluginFactory& PluginFactory::getInstance()
{
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
        *memoryBudgetManager_
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

} // namespace oscil