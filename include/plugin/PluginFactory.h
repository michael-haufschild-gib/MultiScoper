/*
    Oscil - Plugin Factory
    Abstracts the creation of the plugin processor and its dependencies.
    Facilitates testing by allowing dependency injection logic to be centralized.
*/

#pragma once

#include "plugin/PluginProcessor.h"
#include <memory>

namespace oscil
{

class PluginFactory
{
public:
    virtual ~PluginFactory() = default;

    /**
     * Creates a new instance of the plugin processor.
     * Connects all necessary dependencies (ThemeService, InstanceRegistry, etc.).
     */
    virtual std::unique_ptr<juce::AudioProcessor> createPluginProcessor();

    /**
     * Global access to the factory instance.
     * Can be replaced for testing.
     */
    static PluginFactory& getInstance();

    /**
     * Replaces the global factory instance.
     * @param factory The new factory to use. Ownership is NOT transferred.
     */
    static void setInstance(PluginFactory* factory);

protected:
    PluginFactory() = default;
};

} // namespace oscil
