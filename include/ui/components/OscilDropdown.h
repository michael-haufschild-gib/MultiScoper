/*
    Oscil - Dropdown Component
    Single and multi-select dropdown with searchable options
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * Dropdown item with optional icon and description
 */
struct DropdownItem
{
    juce::String id;
    juce::String label;
    juce::String description;
    juce::Image icon;
    bool enabled = true;
    bool isSeparator = false;

    static DropdownItem separator()
    {
        DropdownItem item;
        item.isSeparator = true;
        return item;
    }
};

/**
 * Dropdown popup that displays the list of options
 */
class OscilDropdownPopup : public juce::Component,
                           public ThemeManagerListener,
                           private juce::Timer
{
public:
    explicit OscilDropdownPopup(IThemeService& themeService);
    ~OscilDropdownPopup() override;

    void setItems(const std::vector<DropdownItem>& items);
    void setSelectedIndices(const std::set<int>& indices);
    void setMultiSelect(bool multiSelect) { multiSelect_ = multiSelect; }
    void setSearchable(bool searchable);

    std::function<void(int)> onItemClicked;
    std::function<void()> onDismiss;

    void show(juce::Component* parent, juce::Rectangle<int> buttonBounds);
    void dismiss();

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    class ItemList;
    friend class ItemList;

    void timerCallback() override;
    void updateFilteredItems();
    int getItemAtPosition(juce::Point<int> pos) const;
    void ensureItemVisible(int index);

    std::vector<DropdownItem> items_;
    std::vector<int> filteredIndices_;
    std::set<int> selectedIndices_;
    bool multiSelect_ = false;
    bool searchable_ = false;
    int hoveredIndex_ = -1;
    int focusedIndex_ = -1;

    std::unique_ptr<juce::TextEditor> searchField_;
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<ItemList> listComponent_;
    juce::String searchText_;

    SpringAnimation showSpring_;
    IThemeService& themeService_;
    ColorTheme theme_;

    static constexpr int ITEM_HEIGHT = 32;
    static constexpr int MAX_VISIBLE_ITEMS = 8;
    static constexpr int SEARCH_HEIGHT = 36;
    static constexpr int POPUP_PADDING = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilDropdownPopup)
};

/**
 * A dropdown/select component with extensive features
 *
 * Features:
 * - Single-select and multi-select modes
 * - Searchable options
 * - Optional item icons and descriptions
 * - Animated open/close
 * - Keyboard navigation
 * - Full accessibility support
 */
class OscilDropdown : public juce::Component,
                      public ThemeManagerListener,
                      public TestIdSupport,
                      private juce::Timer
{
public:
    OscilDropdown(IThemeService& themeService);
    OscilDropdown(IThemeService& themeService, const juce::String& placeholder);
    OscilDropdown(IThemeService& themeService, const juce::String& placeholder, const juce::String& testId);
    ~OscilDropdown() override;

    // Items management
    void addItem(const juce::String& label, const juce::String& id = {});
    void addItem(const DropdownItem& item);
    void addItems(const std::vector<juce::String>& labels);
    void addItems(const std::vector<DropdownItem>& items);
    void clearItems();

    int getNumItems() const { return static_cast<int>(items_.size()); }
    const DropdownItem& getItem(int index) const { return items_[static_cast<size_t>(index)]; }

    // Single selection
    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const;
    juce::String getSelectedId() const;
    juce::String getSelectedLabel() const;

    // Multi-selection
    void setSelectedIndices(const std::set<int>& indices, bool notify = true);
    std::set<int> getSelectedIndices() const { return selectedIndices_; }
    std::vector<juce::String> getSelectedIds() const;
    std::vector<juce::String> getSelectedLabels() const;

    // Configuration
    void setPlaceholder(const juce::String& placeholder);
    juce::String getPlaceholder() const { return placeholder_; }

    void setMultiSelect(bool multiSelect);
    bool isMultiSelect() const { return multiSelect_; }

    void setSearchable(bool searchable);
    bool isSearchable() const { return searchable_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Popup control
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const { return popupVisible_; }

    // Callbacks
    std::function<void(int)> onSelectionChanged;
    std::function<void(const std::set<int>&)> onMultiSelectionChanged;
    std::function<void(const juce::String&)> onSelectionChangedId;

    // Size hints
    int getPreferredWidth() const;
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

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void handleItemClicked(int index);
    void updateDisplayText();
    void paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds);

    std::vector<DropdownItem> items_;
    std::set<int> selectedIndices_;
    juce::String placeholder_ = "Select...";
    juce::String displayText_;
    bool multiSelect_ = false;
    bool searchable_ = false;
    bool enabled_ = true;

    bool isHovered_ = false;
    bool hasFocus_ = false;
    bool popupVisible_ = false;

    SpringAnimation hoverSpring_;
    SpringAnimation chevronSpring_;

    std::unique_ptr<OscilDropdownPopup> popup_;

    IThemeService& themeService_;
    ColorTheme theme_;

    static constexpr int CHEVRON_SIZE = 16;
    static constexpr int PADDING_H = 12;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilDropdown)
};

} // namespace oscil
