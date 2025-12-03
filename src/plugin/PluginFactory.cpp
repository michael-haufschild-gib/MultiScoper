/*
    Oscil - Plugin Factory Implementation
*/

#include "plugin/PluginFactory.h"
#include "core/InstanceRegistry.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

namespace
{
    class DefaultPluginFactory : public PluginFactory
    {
    public:
        DefaultPluginFactory() = default;
    };

    DefaultPluginFactory defaultFactoryInstance;
    PluginFactory* currentFactory = &defaultFactoryInstance;
}

PluginFactory& PluginFactory::getInstance()
{
    return *currentFactory;
}

void PluginFactory::setInstance(PluginFactory* factory)
{
    currentFactory = factory ? factory : &defaultFactoryInstance;
}

std::unique_ptr<juce::AudioProcessor> PluginFactory::createPluginProcessor()
{
    return std::make_unique<OscilPluginProcessor>(
        InstanceRegistry::getInstance(),
        ThemeManager::getInstance()
    );
}

} // namespace oscil