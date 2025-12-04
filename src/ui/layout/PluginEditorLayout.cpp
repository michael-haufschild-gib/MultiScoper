/*
    Oscil - Plugin Editor Layout Implementation
*/

#include "ui/layout/PluginEditorLayout.h"

namespace oscil
{

PluginEditorLayout::PluginEditorLayout(juce::Component& editor,
                                       juce::Viewport& viewport,
                                       PaneContainerComponent& content,
                                       SidebarComponent& sidebar,
                                       StatusBarComponent& statusBar,
                                       OscilPluginProcessor& processor)
    : editor_(editor)
    , viewport_(viewport)
    , content_(content)
    , sidebar_(sidebar)
    , statusBar_(statusBar)
    , processor_(processor)
{
}

void PluginEditorLayout::resized()
{
    auto bounds = editor_.getLocalBounds();

    // Status bar
    auto statusBarBounds = bounds.removeFromBottom(STATUS_BAR_HEIGHT);
    statusBar_.setBounds(statusBarBounds);

    // Sidebar on right
    int sidebarWidth = sidebar_.getEffectiveWidth();
    auto sidebarBounds = bounds.removeFromRight(sidebarWidth);
    sidebar_.setBounds(sidebarBounds);

    // Main content area (left of sidebar)
    viewport_.setBounds(bounds);
}

void PluginEditorLayout::updateLayout(const std::vector<std::unique_ptr<PaneComponent>>& paneComponents)
{
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Use viewport's own bounds - this is the space available for content
    // Note: getViewArea() returns the visible portion of CONTENT, not viewport size
    auto availableArea = viewport_.getLocalBounds();

    // Ensure minimum dimensions to prevent zero-size components
    int availableWidth = std::max(MIN_CONTENT_WIDTH, availableArea.getWidth());
    int availableHeight = std::max(MIN_CONTENT_HEIGHT, availableArea.getHeight());
    availableArea = juce::Rectangle<int>(0, 0, availableWidth, availableHeight);

    // Calculate content size based on number of panes
    int numPanes = static_cast<int>(paneComponents.size());
    if (numPanes == 0)
    {
        content_.setSize(availableArea.getWidth(), availableArea.getHeight());
        return;
    }

    int numCols = layoutManager.getColumnCount();
    content_.setColumnCount(numCols);
    int numRows = (numPanes + numCols - 1) / numCols;
    int paneHeight = std::max(MIN_PANE_HEIGHT, availableArea.getHeight() / std::max(1, numRows));

    content_.setSize(availableArea.getWidth(), paneHeight * numRows);

    // Position panes
    for (int i = 0; i < numPanes; ++i)
    {
        auto bounds = layoutManager.getPaneBounds(i,
                                                  juce::Rectangle<int>(0, 0, content_.getWidth(), content_.getHeight()));
        paneComponents[static_cast<size_t>(i)]->setBounds(bounds);
    }
}

} // namespace oscil
