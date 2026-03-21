/*
    Oscil - Plugin Factory Implementation
    Composition root for dependency injection.
*/

#include "plugin/PluginFactory.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/OscilState.h"  // for GlobalPreferences
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/PresetManager.h"
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
    , presetManager_(std::make_unique<PresetManager>())
{
}

PluginFactory::~PluginFactory() = default;

PluginFactory& PluginFactory::getInstance()
{
    // Lazy initialization - create on first access
    // This is the ONLY singleton pattern in the system
    static PluginFactory instance;

    PluginFactory* current = currentFactory.load(std::memory_order_acquire);
    return current ? *current : instance;
}

void PluginFactory::setInstance(PluginFactory* factory)
{
    // Store override (nullptr clears it, falling back to the static default)
    currentFactory.store(factory, std::memory_order_release);
}

std::unique_ptr<juce::AudioProcessor> PluginFactory::createPluginProcessor()
{
    PluginProcessorConfig config{
        *instanceRegistry_,
        *themeManager_,
        *shaderRegistry_,
        *presetManager_,
        *memoryBudgetManager_
    };
    return std::make_unique<OscilPluginProcessor>(config);
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

PresetManager& PluginFactory::getPresetManager()
{
    return *presetManager_;
}

} // namespace oscil