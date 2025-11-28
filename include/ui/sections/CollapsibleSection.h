/*
    Oscil - Collapsible Section Component
    Generic wrapper that adds accordion-style collapse/expand behavior to any section
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/sections/DynamicHeightContent.h"
#include "ui/components/TestId.h"
#include <functional>

namespace oscil
{

/**
 * Generic collapsible section wrapper component
 * Adds a clickable header bar with title and caret indicator that expands/collapses content
 */
class CollapsibleSection : public juce::Component,
                           public ThemeManagerListener,
                           public TestIdSupport
{
public:
    /**
     * Construct a collapsible section
     * @param title The title displayed in the header bar
     * @param testIdPrefix Prefix for test IDs (e.g., "sidebar_timing")
     */
    CollapsibleSection(const juce::String& title, const juce::String& testIdPrefix);
    ~CollapsibleSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Set the content component to wrap
     * This component will be shown/hidden based on collapsed state
     */
    void setContent(juce::Component* content);

    /**
     * Get the content component
     */
    juce::Component* getContent() const { return content_; }

    /**
     * Set collapsed state
     */
    void setCollapsed(bool collapsed);

    /**
     * Get collapsed state
     */
    bool isCollapsed() const { return collapsed_; }

    /**
     * Toggle collapsed state
     */
    void toggleCollapsed();

    /**
     * Calculate the preferred height based on collapsed state
     * When collapsed: returns just header height
     * When expanded: returns header height + content's preferred height
     */
    int getPreferredHeight() const;

    /**
     * Notify that content's preferred height has changed
     * Call this when the wrapped content changes its height dynamically
     */
    void contentHeightChanged();

    /**
     * Callback when collapse state changes
     */
    std::function<void(bool collapsed)> onCollapseStateChanged;

    /**
     * Callback when the section needs relayout (e.g., content height changed)
     */
    std::function<void()> onLayoutNeeded;

    // Header bar height constant
    static constexpr int HEADER_HEIGHT = 28;

private:
    juce::String title_;
    juce::String testIdPrefix_;
    juce::Component* content_ = nullptr;
    DynamicHeightContent* dynamicContent_ = nullptr;  // Cached if content implements DynamicHeightContent
    bool collapsed_ = false;

    // Cached theme colors
    juce::Colour headerBackground_;
    juce::Colour headerText_;
    juce::Colour caretColour_;
    juce::Colour borderColour_;

    /**
     * Draw the caret indicator (triangle pointing right when collapsed, down when expanded)
     */
    void drawCaret(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /**
     * Get the clickable header area bounds
     */
    juce::Rectangle<int> getHeaderBounds() const;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsibleSection)
};

} // namespace oscil
