#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/dropdown/DropdownItem.h"
#include "ui/animation/OscilAnimationService.h"
#include <set>

namespace oscil
{

/**
 * Dropdown popup that displays the list of options
 * Uses JUCE Animator for VBlank-synced animations
 */
class OscilDropdownPopup : public ThemedComponent
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
    bool keyPressed(const juce::KeyPress& key) override;
    void focusLost(FocusChangeType cause) override;

private:
    class ItemList;
    friend class ItemList;

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

    // Animation state
    float showProgress_ = 0.0f;
    OscilAnimationService* animService_ = nullptr;
    ScopedAnimator showAnimator_;

    static constexpr int ITEM_HEIGHT = 32;
    static constexpr int MAX_VISIBLE_ITEMS = 8;
    static constexpr int SEARCH_HEIGHT = 36;
    static constexpr int POPUP_PADDING = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilDropdownPopup)
};

}
