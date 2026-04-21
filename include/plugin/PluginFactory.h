/*
    MultiScoper - Plugin Factory
    Composition root for dependency injection.
    Owns all core services and creates plugin instances with proper dependencies.
*/

#pragma once

#include "plugin/PluginProcessor.h"

#include <memory>

namespace multiscoper
{

// Forward declarations
class ThemeManager;
class InstanceRegistry;
class ShaderRegistry;
class MemoryBudgetManager;
class GlobalPreferences;
class PresetManager;

/**
 * Composition root for the MultiScoper plugin.
 *
 * This is the ONLY singleton in the system. It owns all core services
 * and creates plugin instances with proper dependency injection.
 *
 * The factory owns:
 * - ThemeManager (implements IThemeService)
 * - InstanceRegistry (implements IInstanceRegistry)
 * - ShaderRegistry
 * - MemoryBudgetManager
 * - GlobalPreferences
 */
class PluginFactory
{
public:
    /// Construct the factory and initialize all owned services.
    PluginFactory();
    virtual ~PluginFactory();

    /**
     * Creates a new instance of the plugin processor.
     * Connects all necessary dependencies from owned services.
     */
    virtual std::unique_ptr<juce::AudioProcessor> createPluginProcessor();

    /**
     * Global access to the factory instance.
     * This is the composition root - only this singleton is allowed.
     */
    static PluginFactory& getInstance();

    /**
     * Replaces the global factory instance.
     * @param factory The new factory to use. Ownership is NOT transferred.
     */
    static void setInstance(PluginFactory* factory);

    /**
     * Access to owned services (for test harness and special cases).
     * Normal code should receive services via dependency injection,
     * not by calling these methods.
     */
    /// Get the global theme manager (implements IThemeService).
    ThemeManager& getThemeManager();
    /// Get the shared instance registry for multi-instance coordination.
    InstanceRegistry& getInstanceRegistry();
    /// Get the shader registry for GPU waveform shaders.
    ShaderRegistry& getShaderRegistry();
    /// Get the memory budget manager for capture buffer allocation.
    MemoryBudgetManager& getMemoryBudgetManager();
    /// Get the global user preferences (persisted separately from project state).
    GlobalPreferences& getGlobalPreferences();
    /// Get the visual preset manager.
    PresetManager& getPresetManager();

private:
    // Owned services. Construction order: GlobalPreferences comes first so
    // ThemeManager can consult the persisted default theme when it's built.
    std::unique_ptr<GlobalPreferences> globalPreferences_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<InstanceRegistry> instanceRegistry_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<PresetManager> presetManager_;

    // Small adapter that listens to theme changes and writes them to
    // GlobalPreferences. Owned here so it's destroyed before ThemeManager.
    class ThemePreferenceSync;
    std::unique_ptr<ThemePreferenceSync> themePreferenceSync_;

    // Prevent copying
    PluginFactory(const PluginFactory&) = delete;
    PluginFactory& operator=(const PluginFactory&) = delete;
};

} // namespace multiscoper
