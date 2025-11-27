/*
    Oscil - Accordion Component
    Animated expandable/collapsible sections
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"

namespace oscil
{

/**
 * Individual accordion section with animated expand/collapse
 */
class OscilAccordionSection : public juce::Component,
                              public ThemeManagerListener,
                              private juce::Timer
{
public:
    OscilAccordionSection();
    explicit OscilAccordionSection(const juce::String& title);
    ~OscilAccordionSection() override;

    // Configuration
    void setTitle(const juce::String& title);
    juce::String getTitle() const { return title_; }

    void setIcon(const juce::Image& icon);
    void clearIcon();

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

    // Size
    int getHeaderHeight() const { return HEADER_HEIGHT; }
    int getContentHeight() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void timerCallback() override;
    void updateAnimations();

    void paintHeader(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds);

    juce::String title_;
    juce::Image icon_;
    juce::Component* content_ = nullptr;
    bool expanded_ = false;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool hasFocus_ = false;

    SpringAnimation expandSpring_;     // 0 = collapsed, 1 = expanded
    SpringAnimation hoverSpring_;
    SpringAnimation chevronSpring_;    // Rotation animation

    ColorTheme theme_;

    static constexpr int HEADER_HEIGHT = 40;
    static constexpr int CHEVRON_SIZE = 16;
    static constexpr int ICON_SIZE = 18;
    static constexpr int PADDING_H = 12;

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
class OscilAccordion : public juce::Component,
                       public ThemeManagerListener
{
public:
    OscilAccordion();
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

    // Component overrides
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void handleSectionExpanded(int index, bool expanded);
    void layoutSections();

    juce::OwnedArray<OscilAccordionSection> sections_;
    bool allowMultiExpand_ = false;
    int spacing_ = 1;

    ColorTheme theme_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilAccordion)
};

} // namespace oscil
