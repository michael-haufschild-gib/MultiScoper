/*
    Oscil - Accordion Component
    Animated expandable/collapsible sections
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "ui/components/ThemedComponent.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"
#include "ui/layout/sections/DynamicHeightContent.h"

namespace oscil
{

/**
 * Individual accordion section with animated expand/collapse
 */
class OscilAccordionSection : public ThemedComponent,
                              public TestIdSupport,
                              private juce::Timer
{
public:
    explicit OscilAccordionSection(IThemeService& themeService, const juce::String& title = "");
    OscilAccordionSection(IThemeService& themeService, const juce::String& title, const juce::String& testId);
    ~OscilAccordionSection() override;

    // Configuration
    void setTitle(const juce::String& title);
    juce::String getTitle() const { return title_; }

    void setIcon(const juce::Image& icon);
    void clearIcon();

    /**
     * Set the content component to display in this section.
     * @param content Component to display. NOT owned by this section - caller must
     *                ensure content lifetime exceeds this section's lifetime.
     *                Pass nullptr to clear the content.
     */
    void setContent(juce::Component* content);
    juce::Component* getContent() const { return content_; }

    // State
    void setExpanded(bool expanded, bool animate = true);
    bool isExpanded() const { return expanded_; }
    void toggle();

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Callbacks
    std::function<void(bool)> onExpandedChanged;
    std::function<void()> onHeightChanged;

    // Size
    int getHeaderHeight() const { return HEADER_HEIGHT; }
    int getContentHeight() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    void timerCallback() override;
    void updateAnimations();
    void contentHeightChanged();

    void paintHeader(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds);

    juce::String title_;
    juce::Image icon_;
    juce::Component* content_ = nullptr;
    DynamicHeightContent* dynamicContent_ = nullptr;
    bool expanded_ = false;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool hasFocus_ = false;
    bool mouseDownInHeader_ = false;  // Track if mouseDown occurred in header for proper click detection

    SpringAnimation expandSpring_;     // 0 = collapsed, 1 = expanded
    SpringAnimation hoverSpring_;
    SpringAnimation chevronSpring_;    // Rotation animation

    int lastReportedHeight_ = -1;      // For optimizing parent layout updates

    static constexpr int HEADER_HEIGHT = 40;
    static constexpr int CHEVRON_SIZE = 16;
    static constexpr int ICON_SIZE = 18;
    static constexpr int PADDING_H = 12;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilAccordionSection)
};

/**
 * Container for multiple accordion sections
 *
 * Features:
 * - Single or multi-expand mode
 * - Animated expand/collapse with spring physics
 * - Optional icons
 * - Keyboard navigation
 */
class OscilAccordion : public ThemedComponent
{
public:
    explicit OscilAccordion(IThemeService& themeService);
    ~OscilAccordion() override;

    // Section management
    OscilAccordionSection* addSection(const juce::String& title);
    OscilAccordionSection* addSection(const juce::String& title, juce::Component* content);
    void removeSection(int index);
    void clearSections();

    int getNumSections() const { return static_cast<int>(sections_.size()); }
    OscilAccordionSection* getSection(int index);

    // Multi-expand control
    void setAllowMultiExpand(bool allow);
    bool getAllowMultiExpand() const { return allowMultiExpand_; }

    // Expand control
    void expandSection(int index);
    void collapseSection(int index);
    void expandAll();
    void collapseAll();

    // Configuration
    void setSpacing(int spacing);
    int getSpacing() const { return spacing_; }

    // Size
    int getPreferredHeight() const;
    void updateContentHeight();

    // Component overrides
    void resized() override;

private:
    void handleSectionExpanded(int index, bool expanded);
    void layoutSections();

    std::vector<std::unique_ptr<OscilAccordionSection>> sections_;
    bool allowMultiExpand_ = false;
    int spacing_ = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilAccordion)
};

} // namespace oscil
