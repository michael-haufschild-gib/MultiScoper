#pragma once
#include "ui/components/ThemedComponent.h"
#include "rendering/VisualConfiguration.h"

namespace oscil
{

class PresetEditorTab : public ThemedComponent
{
public:
    PresetEditorTab(IThemeService& themeService) : ThemedComponent(themeService) {}
    virtual ~PresetEditorTab() = default;

    virtual void setConfiguration(const VisualConfiguration& config) = 0;
    virtual void updateConfiguration(VisualConfiguration& config) = 0;

    std::function<void()> onConfigChanged;

protected:
    void notifyChanged()
    {
        if (onConfigChanged)
            onConfigChanged();
    }
};

}
