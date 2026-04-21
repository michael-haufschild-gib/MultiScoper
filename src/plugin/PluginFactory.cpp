/*
    MultiScoper - Plugin Factory Implementation
    Composition root for dependency injection.
*/

#include "plugin/PluginFactory.h"

#include "core/GlobalPreferences.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"

#include "rendering/PresetManager.h"
#include "rendering/ShaderRegistry.h"

#include <atomic>

namespace multiscoper
{

namespace
{
// The single factory instance - composition root
// Using atomic for thread-safe access from multiple threads
std::atomic<PluginFactory*> currentFactory{nullptr};
} // namespace

/// Bridges ThemeManager → GlobalPreferences so the user's active theme
/// survives across DAW sessions.
class PluginFactory::ThemePreferenceSync final : public ThemeManagerListener
{
public:
    ThemePreferenceSync(ThemeManager& themes, GlobalPreferences& prefs) : themes_(themes), prefs_(prefs)
    {
        themes_.addListener(this);
    }

    ~ThemePreferenceSync() override { themes_.removeListener(this); }

    void themeChanged(const ColorTheme& newTheme) override { prefs_.setDefaultTheme(newTheme.name); }

    ThemePreferenceSync(const ThemePreferenceSync&) = delete;
    ThemePreferenceSync& operator=(const ThemePreferenceSync&) = delete;

private:
    ThemeManager& themes_;
    GlobalPreferences& prefs_;
};

PluginFactory::PluginFactory()
    // Construct globalPreferences_ first so ThemeManager can consult the
    // persisted default theme; declaration order in PluginFactory.h is
    // authoritative for member init, so it must match this intent.
    : globalPreferences_(std::make_unique<GlobalPreferences>())
    , themeManager_(std::make_unique<ThemeManager>())
    , instanceRegistry_(std::make_unique<InstanceRegistry>())
    , shaderRegistry_(std::make_unique<ShaderRegistry>())
    , memoryBudgetManager_(std::make_unique<MemoryBudgetManager>())
    , presetManager_(std::make_unique<PresetManager>())
{
    // Restore the user's last-selected theme if the preference names one.
    const auto savedTheme = globalPreferences_->getDefaultTheme();
    if (savedTheme.isNotEmpty() && themeManager_->getTheme(savedTheme) != nullptr)
    {
        themeManager_->setCurrentTheme(savedTheme);
    }

    // Persist future theme changes back to preferences.
    themePreferenceSync_ = std::make_unique<ThemePreferenceSync>(*themeManager_, *globalPreferences_);
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
    PluginProcessorConfig const config{.instanceRegistry = *instanceRegistry_,
                                       .themeService = *themeManager_,
                                       .shaderRegistry = *shaderRegistry_,
                                       .presetManager = *presetManager_,
                                       .memoryBudgetManager = *memoryBudgetManager_};
    return std::make_unique<MultiScoperPluginProcessor>(config);
}

ThemeManager& PluginFactory::getThemeManager() { return *themeManager_; }

InstanceRegistry& PluginFactory::getInstanceRegistry() { return *instanceRegistry_; }

ShaderRegistry& PluginFactory::getShaderRegistry() { return *shaderRegistry_; }

MemoryBudgetManager& PluginFactory::getMemoryBudgetManager() { return *memoryBudgetManager_; }

GlobalPreferences& PluginFactory::getGlobalPreferences() { return *globalPreferences_; }

PresetManager& PluginFactory::getPresetManager() { return *presetManager_; }

} // namespace multiscoper