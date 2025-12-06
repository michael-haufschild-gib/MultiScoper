/*
    Oscil - Tabs Component
    Horizontal and vertical tab navigation with animated indicator
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * Tab item with optional icon and badge
 */
struct TabItem
{
    juce::String id;
    juce::String label;
    juce::Image icon;
    int badgeCount = 0;         // 0 = no badge
    bool enabled = true;
};

/**
 * A tabbed interface component with animated indicator
 *
 * Features:
 * - Horizontal or vertical orientation
 * - Animated selection indicator
 * - Optional icons and badge counts
 * - Keyboard navigation
 * - Full accessibility support
 */
class OscilTabs : public ThemedComponent,
                  public TestIdSupport,
                  private juce::Timer
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    enum class Variant
    {
        Default,    // Underline indicator
        Pills,      // Filled background indicator
        Bordered    // Border around selected tab
    };

    explicit OscilTabs(IThemeService& themeService);
    OscilTabs(IThemeService& themeService, Orientation orientation);
    OscilTabs(IThemeService& themeService, Orientation orientation, const juce::String& testId);
    ~OscilTabs() override;

    // Tab management
    void addTab(const juce::String& label, const juce::String& id = {});
    void addTab(const TabItem& tab);
    void addTabs(const std::vector<juce::String>& labels);
    void addTabs(const std::vector<TabItem>& tabs);
    void clearTabs();

    int getNumTabs() const { return static_cast<int>(tabs_.size()); }
    const TabItem& getTab(int index) const { return tabs_[static_cast<size_t>(index)]; }

    void setTabBadge(int index, int count);
    void setTabEnabled(int index, bool enabled);

    // Selection
    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const { return selectedIndex_; }

    juce::String getSelectedId() const;
    void setSelectedById(const juce::String& id, bool notify = true);

    // Configuration
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return orientation_; }

    void setVariant(Variant variant);
    Variant getVariant() const { return variant_; }

    void setTabWidth(int width);  // 0 = auto
    int getTabWidth() const { return tabWidth_; }

    void setTabHeight(int height);  // 0 = auto
    int getTabHeight() const { return tabHeight_; }

    void setStretchTabs(bool stretch);  // Fill available space
    bool isStretchTabs() const { return stretchTabs_; }

    // Callbacks
    std::function<void(int)> onTabChanged;
    std::function<void(const juce::String&)> onTabChangedId;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void updateAnimations();

    int getTabAtPosition(juce::Point<int> pos) const;
    juce::Rectangle<int> getTabBounds(int index) const;
    juce::Rectangle<float> getIndicatorBounds() const;

    void paintTab(juce::Graphics& g, int index, juce::Rectangle<int> bounds);
    void paintIndicator(juce::Graphics& g);
    void paintBadge(juce::Graphics& g, juce::Rectangle<int> bounds, int count);

    std::vector<TabItem> tabs_;
    int selectedIndex_ = 0;
    int hoveredIndex_ = -1;

    Orientation orientation_ = Orientation::Horizontal;
    Variant variant_ = Variant::Default;
    int tabWidth_ = 0;   // 0 = auto
    int tabHeight_ = 0;  // 0 = auto
    bool stretchTabs_ = false;

    bool hasFocus_ = false;

    // Animation for indicator position
    SpringAnimation indicatorXSpring_;
    SpringAnimation indicatorWidthSpring_;
    SpringAnimation hoverSpring_;

    float targetIndicatorX_ = 0;
    float targetIndicatorWidth_ = 0;

    static constexpr int DEFAULT_TAB_HEIGHT = 40;
    static constexpr int INDICATOR_HEIGHT = 3;
    static constexpr int TAB_PADDING_H = 16;
    static constexpr int ICON_SIZE = 16;
    static constexpr int BADGE_SIZE = 18;

    // Cached layout
    std::vector<juce::Rectangle<int>> cachedTabBounds_;
    void updateLayoutCache();

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilTabs)
};

} // namespace oscil
