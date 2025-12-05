/*
    Oscil - Plugin Factory
    Composition root for dependency injection.
    Owns all core services and creates plugin instances with proper dependencies.
*/

#pragma once

#include "plugin/PluginProcessor.h"
#include <memory>

namespace oscil
{

// Forward declarations
class ThemeManager;
class InstanceRegistry;
class ShaderRegistry;
class MemoryBudgetManager;
class GlobalPreferences;

/**
 * Composition root for the Oscil plugin.
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
    ThemeManager& getThemeManager();
    InstanceRegistry& getInstanceRegistry();
    ShaderRegistry& getShaderRegistry();
    MemoryBudgetManager& getMemoryBudgetManager();
    GlobalPreferences& getGlobalPreferences();

private:
    // Owned services - these are the single instances for the plugin
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<InstanceRegistry> instanceRegistry_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<GlobalPreferences> globalPreferences_;

    // Prevent copying
    PluginFactory(const PluginFactory&) = delete;
    PluginFactory& operator=(const PluginFactory&) = delete;
};

} // namespace oscil
