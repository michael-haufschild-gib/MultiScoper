/*
    Oscil - Settings Dropdown Component
    PRD aligned settings panel with theme, layout, and statusbar controls
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include "core/Pane.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/TestId.h"
#include <vector>

namespace oscil
{

// Forward declarations
class OscilPluginProcessor;

/**
 * Clickable theme list item matching design mockup
 */
class ThemeListItem : public juce::Component
{
public:
    ThemeListItem(const juce::String& themeName);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    const juce::String& getThemeName() const { return themeName_; }

    std::function<void(const juce::String&)> onClick;

private:
    juce::String themeName_;
    bool selected_ = false;
    bool hovered_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeListItem)
};

/**
 * Visual layout icon button matching design mockup
 */
class LayoutIconButton : public juce::Component
{
public:
    LayoutIconButton(ColumnLayout layout);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    ColumnLayout getLayout() const { return layout_; }

    std::function<void(ColumnLayout)> onClick;

private:
    ColumnLayout layout_;
    bool selected_ = false;
    bool hovered_ = false;

    void drawLayoutIcon(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayoutIconButton)
};

/**
 * Settings dropdown component providing access to:
 * - Theme selection (visual list per design)
 * - Layout mode (visual icons per design)
 * - Status bar toggle
 * PRD aligned with SettingsDropdown entity
 * Note: Timing controls moved to TimingSidebarSection
 */
class SettingsDropdown : public juce::Component,
                         public ThemeManagerListener,
                         public TestIdSupport
{
public:
    /**
     * Listener interface for settings changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void layoutModeChanged(ColumnLayout /*layout*/) {}
        virtual void statusBarVisibilityChanged(bool /*visible*/) {}
        virtual void themeEditRequested() {}
    };

    explicit SettingsDropdown(OscilPluginProcessor& processor);
    ~SettingsDropdown() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State accessors
    bool isStatusBarVisible() const { return statusBarVisible_; }
    void setStatusBarVisible(bool visible);

    ColumnLayout getCurrentLayoutMode() const { return currentLayout_; }
    void setLayoutMode(ColumnLayout layout);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    /** Returns preferred height based on number of themes */
    int getPreferredHeight() const;

private:
    OscilPluginProcessor& processor_;

    // Theme section - visual list per design
    std::unique_ptr<juce::Label> themeSectionLabel_;
    std::vector<std::unique_ptr<ThemeListItem>> themeItems_;
    std::unique_ptr<OscilButton> editThemeButton_;

    // Layout section - visual icons per design
    std::unique_ptr<juce::Label> layoutSectionLabel_;
    std::vector<std::unique_ptr<LayoutIconButton>> layoutButtons_;

    // Status bar section
    std::unique_ptr<juce::Label> statusBarLabel_;
    std::unique_ptr<OscilToggle> statusBarToggle_;

    // State
    bool statusBarVisible_ = true;
    ColumnLayout currentLayout_ = ColumnLayout::Single;

    juce::ListenerList<Listener> listeners_;

    void setupThemeSection();
    void setupLayoutSection();
    void setupStatusBarSection();

    void refreshThemeList();
    void selectTheme(const juce::String& themeName);
    void selectLayout(ColumnLayout layout);

    void notifyLayoutModeChanged();
    void notifyStatusBarVisibilityChanged();
    void notifyThemeEditRequested();

    void layoutContent(); // Helper to avoid virtual call in constructor

    static constexpr int THEME_ITEM_HEIGHT = 28;
    static constexpr int LAYOUT_ICON_SIZE = 40;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsDropdown)
};

/**
 * Popup version of settings dropdown for toolbar button
 */
class SettingsPopup : public juce::Component
{
public:
    explicit SettingsPopup(OscilPluginProcessor& processor);
    ~SettingsPopup() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    SettingsDropdown& getSettings() { return settings_; }

private:
    SettingsDropdown settings_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPopup)
};

} // namespace oscil
