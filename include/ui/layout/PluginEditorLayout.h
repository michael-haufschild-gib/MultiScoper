/*
    Oscil - Plugin Editor Layout
    Handles component positioning and layout calculations
*/

#pragma once

#include "ui/layout/PaneComponent.h"
#include "ui/layout/PaneContainerComponent.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/panels/StatusBarComponent.h"

#include "plugin/PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

class PluginEditorLayout
{
public:
    PluginEditorLayout(juce::Component& editor,
                       juce::Viewport& viewport,
                       PaneContainerComponent& content,
                       SidebarComponent& sidebar,
                       StatusBarComponent& statusBar,
                       OscilPluginProcessor& processor);

    void resized();
    void updateLayout(const std::vector<std::unique_ptr<PaneComponent>>& paneComponents);

private:
    juce::Component& editor_;
    juce::Viewport& viewport_;
    PaneContainerComponent& content_;
    SidebarComponent& sidebar_;
    StatusBarComponent& statusBar_;
    OscilPluginProcessor& processor_;

    static constexpr int STATUS_BAR_HEIGHT = 24;
    static constexpr int MIN_CONTENT_WIDTH = 400;
    static constexpr int MIN_CONTENT_HEIGHT = 300;
    static constexpr int MIN_PANE_HEIGHT = 200;
};

} // namespace oscil
