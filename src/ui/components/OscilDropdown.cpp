/*
    Oscil - Dropdown Component Implementation
*/

#include "ui/components/OscilDropdown.h"

namespace oscil
{

//==============================================================================
// OscilDropdownPopup Implementation
//==============================================================================

class OscilDropdownPopup::ItemList : public juce::Component
{
public:
    explicit ItemList(OscilDropdownPopup& owner) : owner_(owner)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics& g) override
    {
        float alpha = owner_.showSpring_.position;
        if (alpha < 0.01f) return;

        int start = g.getClipBounds().getY() / ITEM_HEIGHT;
        int end = (g.getClipBounds().getBottom() + ITEM_HEIGHT) / ITEM_HEIGHT;

        start = std::max(0, start);
        end = std::min(end, (int)owner_.filteredIndices_.size());

        for (int i = start; i < end; ++i)
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(i)];
            const auto& item = owner_.items_[static_cast<size_t>(itemIndex)];
            auto itemBounds = juce::Rectangle<int>(0, i * ITEM_HEIGHT, getWidth(), ITEM_HEIGHT);

            bool isHovered = (itemIndex == owner_.hoveredIndex_);
            bool isSelected = owner_.selectedIndices_.count(itemIndex) > 0;
            bool isFocused = (i == owner_.focusedIndex_);

            paintItem(g, item, itemBounds, isHovered, isSelected, isFocused, alpha);
        }
    }

    void paintItem(juce::Graphics& g, const DropdownItem& item,
                  juce::Rectangle<int> bounds, bool isHovered, bool isSelected, bool isFocused, float alpha)
    {
        const auto& theme = owner_.theme_;

        if (item.isSeparator)
        {
            g.setColour(theme.controlBorder.withAlpha(alpha * 0.5f));
            g.fillRect(bounds.reduced(8, ITEM_HEIGHT / 2 - 1).withHeight(1));
            return;
        }

        float opacity = item.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

        // Hover/selected/focused background
        if (isSelected)
        {
            g.setColour(theme.controlActive.withAlpha(alpha * 0.15f));
            g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);
        }
        else if (isFocused)
        {
            g.setColour(theme.controlActive.withAlpha(alpha * 0.1f));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);
        }
        
        if (isHovered && item.enabled && !isSelected)
        {
            g.setColour(theme.backgroundSecondary.withAlpha(alpha * 0.8f));
            g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);
        }

        auto contentBounds = bounds.reduced(8, 0);

        // Icon
        if (item.icon.isValid())
        {
            auto iconBounds = contentBounds.removeFromLeft(20).toFloat();
            g.setOpacity(alpha * opacity);
            g.drawImage(item.icon,
                iconBounds.withSizeKeepingCentre(16, 16),
                juce::RectanglePlacement::centred);
            contentBounds.removeFromLeft(4);
        }

        // Checkbox for multi-select
        if (owner_.multiSelect_)
        {
            auto checkBounds = contentBounds.removeFromLeft(20).toFloat()
                .withSizeKeepingCentre(16, 16);

            g.setColour(theme.backgroundSecondary.withAlpha(alpha * opacity));
            g.fillRoundedRectangle(checkBounds, 3.0f);

            g.setColour((isSelected ? theme.controlActive : theme.controlBorder)
                .withAlpha(alpha * opacity));
            g.drawRoundedRectangle(checkBounds.reduced(0.5f), 3.0f, 1.0f);

            if (isSelected)
            {
                g.setColour(theme.controlActive.withAlpha(alpha * opacity));
                // Draw checkmark
                float cx = checkBounds.getCentreX();
                float cy = checkBounds.getCentreY();
                juce::Path checkPath;
                checkPath.startNewSubPath(cx - 4, cy);
                checkPath.lineTo(cx - 1, cy + 3);
                checkPath.lineTo(cx + 4, cy - 2);
                g.strokePath(checkPath, juce::PathStrokeType(1.5f));
            }

            contentBounds.removeFromLeft(4);
        }

        // Label
        g.setColour((isSelected ? theme.controlActive : theme.textPrimary)
            .withAlpha(alpha * opacity));
            
        static const juce::Font itemFont(juce::FontOptions().withHeight(13.0f));
        g.setFont(itemFont);

        if (item.description.isNotEmpty())
        {
            auto labelBounds = contentBounds.removeFromTop(ITEM_HEIGHT / 2 + 2);
            g.drawText(item.label, labelBounds, juce::Justification::centredLeft);

            g.setColour(theme.textSecondary.withAlpha(alpha * opacity * 0.8f));
            static const juce::Font descFont(juce::FontOptions().withHeight(11.0f));
            g.setFont(descFont);
            g.drawText(item.description, contentBounds, juce::Justification::centredLeft);
        }
        else
        {
            g.drawText(item.label, contentBounds, juce::Justification::centredLeft);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int index = e.y / ITEM_HEIGHT;
        if (index >= 0 && index < static_cast<int>(owner_.filteredIndices_.size()))
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
            if (owner_.hoveredIndex_ != itemIndex)
            {
                owner_.hoveredIndex_ = itemIndex;
                repaint();
            }
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (owner_.hoveredIndex_ != -1)
        {
            owner_.hoveredIndex_ = -1;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int index = e.y / ITEM_HEIGHT;
        if (index >= 0 && index < static_cast<int>(owner_.filteredIndices_.size()))
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
            const auto& item = owner_.items_[static_cast<size_t>(itemIndex)];

            if (!item.isSeparator && item.enabled)
            {
                if (owner_.onItemClicked)
                    owner_.onItemClicked(itemIndex);

                if (!owner_.multiSelect_)
                    owner_.dismiss();
            }
        }
    }

private:
    OscilDropdownPopup& owner_;
};

OscilDropdownPopup::OscilDropdownPopup(IThemeService& themeService)
    : showSpring_(SpringPresets::snappy())
    , themeService_(themeService)
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);

    // Mark as popup so modal focus traps don't steal focus
    getProperties().set("isOscilPopup", true);

    theme_ = themeService_.getCurrentTheme();
    themeService_.addListener(this);

    showSpring_.position = 0.0f;
    showSpring_.target = 0.0f;

    listComponent_ = std::make_unique<ItemList>(*this);
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setViewedComponent(listComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false); // Vertical only
    viewport_->setScrollBarThickness(6);
    addAndMakeVisible(*viewport_);
}

OscilDropdownPopup::~OscilDropdownPopup()
{
    themeService_.removeListener(this);
    stopTimer();
}

void OscilDropdownPopup::setItems(const std::vector<DropdownItem>& items)
{
    items_ = items;
    updateFilteredItems();
}

void OscilDropdownPopup::setSelectedIndices(const std::set<int>& indices)
{
    selectedIndices_ = indices;
    repaint();
}

class SearchField : public juce::TextEditor
{
public:
    SearchField(OscilDropdownPopup& owner) : owner_(owner) {}
    
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::upKey || 
            key == juce::KeyPress::downKey ||
            key == juce::KeyPress::escapeKey) 
        {
            return owner_.keyPressed(key);
        }
        
        if (key == juce::KeyPress::returnKey)
        {
            if (owner_.keyPressed(key)) return true;
        }
        
        return juce::TextEditor::keyPressed(key);
    }
private:
    OscilDropdownPopup& owner_;
};

void OscilDropdownPopup::setSearchable(bool searchable)
{
    searchable_ = searchable;

    if (searchable && !searchField_)
    {
        searchField_ = std::make_unique<SearchField>(*this);
        searchField_->setMultiLine(false);
        searchField_->setTextToShowWhenEmpty("Search...", theme_.textSecondary);
        searchField_->onTextChange = [this]() {
            searchText_ = searchField_->getText();
            updateFilteredItems();
            repaint();
        };
        addAndMakeVisible(*searchField_);
    }
    else if (!searchable && searchField_)
    {
        searchField_.reset();
    }
}

void OscilDropdownPopup::show(juce::Component* parent, juce::Rectangle<int> buttonBounds)
{
    if (!parent) return;

    updateFilteredItems();

    // Calculate popup size
    // Use minimum height if list is empty but we have search
    int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
    
    // Calculate max allowed height for viewport
    int maxViewportHeight = MAX_VISIBLE_ITEMS * ITEM_HEIGHT;
    int viewportHeight = std::min(contentHeight, maxViewportHeight);
    
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int totalHeight = viewportHeight + searchHeight + POPUP_PADDING * 2;
    int width = buttonBounds.getWidth();

    // Position below or above the button
    auto screenBounds = parent->getScreenBounds();
    // Guard against null display (getDisplayForPoint could return nullptr)
    auto* display = juce::Desktop::getInstance().getDisplays()
        .getDisplayForPoint(screenBounds.getCentre());
    if (display == nullptr)
        display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (display == nullptr)
        return;  // No display available, cannot show popup
    auto displayBounds = display->userArea;

    int x = screenBounds.getX() + buttonBounds.getX();
    int y = screenBounds.getY() + buttonBounds.getBottom();

    // Flip above if not enough space below
    if (y + totalHeight > displayBounds.getBottom())
        y = screenBounds.getY() + buttonBounds.getY() - totalHeight;

    setBounds(x, y, width, totalHeight);
    addToDesktop(juce::ComponentPeer::windowIsTemporary |
                 juce::ComponentPeer::windowHasDropShadow);

    setVisible(true);
    
    // Configure List and Viewport
    if (listComponent_)
    {
        // Width accounts for scrollbar if needed? Viewport handles that.
        // We set the size of the list component to full width. 
        // If vertical scrollbar appears, it overlaps or we need to reduce width?
        // JUCE Viewport overlays scrollbars by default or not? 
        // Default is not overlay. So content width is viewport width - scrollbar width.
        // We can set list width to viewport width. Viewport will handle layout.
        // Actually, if we set list width to viewport width, and scrollbar is needed, 
        // the list might be wider than visible area.
        // But DropdownItem paints full width.
        
        listComponent_->setSize(width - POPUP_PADDING * 2, contentHeight);
    }
        
    resized(); // Updates viewport bounds

    grabKeyboardFocus();
    
    // Scroll to selection
    if (!selectedIndices_.empty())
    {
         for (size_t i = 0; i < filteredIndices_.size(); ++i)
         {
             if (selectedIndices_.count(filteredIndices_[i]))
             {
                 focusedIndex_ = static_cast<int>(i);
                 ensureItemVisible(focusedIndex_);
                 break;
             }
         }
    }

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 1.0f;
    }

    if (searchField_)
        searchField_->grabKeyboardFocus();
}

void OscilDropdownPopup::dismiss()
{
    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        setVisible(false);
        if (onDismiss) onDismiss();
    }
}

void OscilDropdownPopup::updateFilteredItems()
{
    filteredIndices_.clear();

    for (size_t i = 0; i < items_.size(); ++i)
    {
        const auto& item = items_[i];

        if (item.isSeparator)
        {
            filteredIndices_.push_back(static_cast<int>(i));
            continue;
        }

        if (searchText_.isEmpty() ||
            item.label.containsIgnoreCase(searchText_) ||
            item.description.containsIgnoreCase(searchText_))
        {
            filteredIndices_.push_back(static_cast<int>(i));
        }
    }
    
    if (listComponent_)
    {
        int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
        // Maintain width
        listComponent_->setSize(listComponent_->getWidth(), contentHeight);
    }
    
    // Clamp focused index
    if (focusedIndex_ >= static_cast<int>(filteredIndices_.size()))
        focusedIndex_ = static_cast<int>(filteredIndices_.size()) - 1;
    if (focusedIndex_ < 0 && !filteredIndices_.empty())
        focusedIndex_ = 0;
}

void OscilDropdownPopup::paint(juce::Graphics& g)
{
    float alpha = showSpring_.position;
    if (alpha < 0.01f) return;

    auto bounds = getLocalBounds().toFloat();

    // Background with shadow effect
    g.setColour(theme_.backgroundPrimary.withAlpha(alpha * 0.98f));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);

    g.setColour(theme_.controlBorder.withAlpha(alpha * 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);
    
    // Items are painted by listComponent_
}

void OscilDropdownPopup::resized()
{
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int y = POPUP_PADDING;
    
    if (searchField_)
    {
        searchField_->setBounds(POPUP_PADDING, POPUP_PADDING,
            getWidth() - POPUP_PADDING * 2, SEARCH_HEIGHT - POPUP_PADDING);
        y += searchHeight;
    }
    
    if (viewport_)
    {
        viewport_->setBounds(POPUP_PADDING, y, 
                             getWidth() - POPUP_PADDING * 2, 
                             getHeight() - y - POPUP_PADDING);
        
        // Ensure list component has correct width (minus scrollbar if needed?)
        if (listComponent_)
        {
             // If scrollbar is visible, we might want to reduce width?
             // Viewport::getVerticalScrollBar()->isVisible()
             // But simpler to just let it be full width.
             listComponent_->setSize(viewport_->getWidth(), listComponent_->getHeight());
        }
    }
}

void OscilDropdownPopup::mouseDown(const juce::MouseEvent&)
{
    dismiss();
}

void OscilDropdownPopup::mouseMove(const juce::MouseEvent&) {}
void OscilDropdownPopup::mouseExit(const juce::MouseEvent&) {}

bool OscilDropdownPopup::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        focusedIndex_ = std::max(0, focusedIndex_ - 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        focusedIndex_ = std::min(static_cast<int>(filteredIndices_.size()) - 1,
                                  focusedIndex_ + 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey && focusedIndex_ >= 0)
    {
        if (focusedIndex_ < static_cast<int>(filteredIndices_.size()))
        {
            int itemIndex = filteredIndices_[static_cast<size_t>(focusedIndex_)];
            const auto& item = items_[static_cast<size_t>(itemIndex)];

            if (!item.isSeparator && item.enabled && onItemClicked)
            {
                onItemClicked(itemIndex);
                if (!multiSelect_)
                    dismiss();
            }
        }
        return true;
    }

    return false;
}

void OscilDropdownPopup::focusLost(FocusChangeType)
{
    // Only dismiss if focus moved outside the popup
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusedComp != this && !isParentOf(focusedComp))
        dismiss();
}

void OscilDropdownPopup::ensureItemVisible(int index)
{
    if (!viewport_ || index < 0) return;
    
    int itemY = index * ITEM_HEIGHT;
    int itemBottom = itemY + ITEM_HEIGHT;
    
    int viewY = viewport_->getViewPositionY();
    int viewH = viewport_->getViewHeight();
    int viewBottom = viewY + viewH;
    
    if (itemY < viewY)
    {
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY);
    }
    else if (itemBottom > viewBottom)
    {
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemBottom - viewH);
    }
}

int OscilDropdownPopup::getItemAtPosition(juce::Point<int>) const
{
    return -1;
}

void OscilDropdownPopup::timerCallback()
{
    showSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);

    if (showSpring_.isSettled())
    {
        stopTimer();
        if (showSpring_.position < 0.1f)
        {
            setVisible(false);
            if (onDismiss) onDismiss();
        }
    }

    if (listComponent_) listComponent_->repaint();
    repaint();
}

void OscilDropdownPopup::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    if (searchField_)
        searchField_->setTextToShowWhenEmpty("Search...", theme_.textSecondary);
    repaint();
}

//==============================================================================
// OscilDropdown Implementation
//==============================================================================

OscilDropdown::OscilDropdown(IThemeService& themeService)
    : hoverSpring_(SpringPresets::stiff())
    , chevronSpring_(SpringPresets::bouncy())
    , themeService_(themeService)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = themeService_.getCurrentTheme();
    themeService_.addListener(this);

    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
    chevronSpring_.position = 0.0f;
    chevronSpring_.target = 0.0f;
}

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder)
    : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
}

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder, const juce::String& testId)
    : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
    setTestId(testId);
}

void OscilDropdown::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilDropdown::~OscilDropdown()
{
    themeService_.removeListener(this);
    stopTimer();
}

void OscilDropdown::addItem(const juce::String& label, const juce::String& id)
{
    DropdownItem item;
    item.id = id.isEmpty() ? label : id;
    item.label = label;
    items_.push_back(item);
}

void OscilDropdown::addItem(const DropdownItem& item)
{
    items_.push_back(item);
}

void OscilDropdown::addItems(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
        addItem(label);
}

void OscilDropdown::addItems(const std::vector<DropdownItem>& items)
{
    for (const auto& item : items)
        addItem(item);
}

void OscilDropdown::clearItems()
{
    items_.clear();
    selectedIndices_.clear();
    updateDisplayText();
    repaint();
}

void OscilDropdown::setSelectedIndex(int index, bool notify)
{
    std::set<int> newSelection;
    if (index >= 0 && index < static_cast<int>(items_.size()))
        newSelection.insert(index);

    setSelectedIndices(newSelection, notify);
}

int OscilDropdown::getSelectedIndex() const
{
    return selectedIndices_.empty() ? -1 : *selectedIndices_.begin();
}

juce::String OscilDropdown::getSelectedId() const
{
    int index = getSelectedIndex();
    return (index >= 0 && index < static_cast<int>(items_.size()))
        ? items_[static_cast<size_t>(index)].id : juce::String();
}

juce::String OscilDropdown::getSelectedLabel() const
{
    int index = getSelectedIndex();
    return (index >= 0 && index < static_cast<int>(items_.size()))
        ? items_[static_cast<size_t>(index)].label : juce::String();
}

void OscilDropdown::setSelectedIndices(const std::set<int>& indices, bool notify)
{
    if (selectedIndices_ == indices)
        return;

    selectedIndices_ = indices;
    updateDisplayText();
    repaint();

    if (notify)
    {
        if (!multiSelect_ && onSelectionChanged)
            onSelectionChanged(getSelectedIndex());

        if (multiSelect_ && onMultiSelectionChanged)
            onMultiSelectionChanged(selectedIndices_);

        if (onSelectionChangedId)
            onSelectionChangedId(getSelectedId());
    }
}

std::vector<juce::String> OscilDropdown::getSelectedIds() const
{
    std::vector<juce::String> result;
    for (int index : selectedIndices_)
        if (index >= 0 && index < static_cast<int>(items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].id);
    return result;
}

std::vector<juce::String> OscilDropdown::getSelectedLabels() const
{
    std::vector<juce::String> result;
    for (int index : selectedIndices_)
        if (index >= 0 && index < static_cast<int>(items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].label);
    return result;
}

void OscilDropdown::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    updateDisplayText();
    repaint();
}

void OscilDropdown::setMultiSelect(bool multiSelect)
{
    if (multiSelect_ != multiSelect)
    {
        multiSelect_ = multiSelect;
        if (!multiSelect && selectedIndices_.size() > 1)
        {
            // Keep only first selection
            int first = *selectedIndices_.begin();
            selectedIndices_.clear();
            selectedIndices_.insert(first);
            updateDisplayText();
        }
        repaint();
    }
}

void OscilDropdown::setSearchable(bool searchable)
{
    searchable_ = searchable;
}

void OscilDropdown::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilDropdown::showPopup()
{
    if (!enabled_ || items_.empty())
        return;

    popupVisible_ = true;

    popup_ = std::make_unique<OscilDropdownPopup>(themeService_);
    popup_->setItems(items_);
    popup_->setSelectedIndices(selectedIndices_);
    popup_->setMultiSelect(multiSelect_);
    popup_->setSearchable(searchable_);

    popup_->onItemClicked = [this](int index) {
        handleItemClicked(index);
    };

    popup_->onDismiss = [this]() {
        popupVisible_ = false;
        popup_.reset();
        chevronSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
        repaint();
    };

    popup_->show(this, getLocalBounds());

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        chevronSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        chevronSpring_.position = 1.0f;
    }

    repaint();
}

void OscilDropdown::hidePopup()
{
    if (popup_)
        popup_->dismiss();
}

void OscilDropdown::handleItemClicked(int index)
{
    if (multiSelect_)
    {
        if (selectedIndices_.count(index))
            selectedIndices_.erase(index);
        else
            selectedIndices_.insert(index);

        updateDisplayText();

        if (popup_)
            popup_->setSelectedIndices(selectedIndices_);

        if (onMultiSelectionChanged)
            onMultiSelectionChanged(selectedIndices_);
    }
    else
    {
        setSelectedIndex(index);
    }

    repaint();
}

void OscilDropdown::updateDisplayText()
{
    if (selectedIndices_.empty())
    {
        displayText_ = placeholder_;
    }
    else if (selectedIndices_.size() == 1)
    {
        int index = *selectedIndices_.begin();
        if (index >= 0 && index < static_cast<int>(items_.size()))
            displayText_ = items_[static_cast<size_t>(index)].label;
        else
            displayText_ = placeholder_;
    }
    else
    {
        displayText_ = juce::String(selectedIndices_.size()) + " selected";
    }
}

int OscilDropdown::getPreferredWidth() const
{
    int maxWidth = 100;

    for (const auto& item : items_)
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, item.label, 0, 0);
        int width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        maxWidth = std::max(maxWidth, width);
    }

    return maxWidth + PADDING_H * 2 + CHEVRON_SIZE + 8;
}

int OscilDropdown::getPreferredHeight() const
{
    return ComponentLayout::INPUT_HEIGHT;
}

void OscilDropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverSpring_.position;

    // Background
    auto bgColour = theme_.backgroundSecondary;
    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.05f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_SM);

    // Border
    auto borderColour = popupVisible_ ? theme_.controlActive
                      : hasFocus_ ? theme_.controlActive
                      : theme_.controlBorder;

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);

    // Focus ring
    if (hasFocus_ && enabled_)
    {
        g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(
            bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
            ComponentLayout::RADIUS_SM + ComponentLayout::FOCUS_RING_OFFSET,
            ComponentLayout::FOCUS_RING_WIDTH
        );
    }

    // Text
    auto textBounds = bounds.reduced(PADDING_H, 0);
    textBounds.removeFromRight(CHEVRON_SIZE + 8);

    bool isPlaceholder = selectedIndices_.empty();
    g.setColour((isPlaceholder ? theme_.textSecondary : theme_.textPrimary)
        .withAlpha(opacity));
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
    g.drawText(displayText_, textBounds, juce::Justification::centredLeft);

    // Chevron
    auto chevronBounds = bounds.removeFromRight(CHEVRON_SIZE + PADDING_H)
        .withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);

    paintChevron(g, chevronBounds);
}

void OscilDropdown::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float rotation = chevronSpring_.position * juce::MathConstants<float>::pi;

    g.setColour(theme_.textSecondary.withAlpha(opacity));

    juce::Path chevron;
    float size = bounds.getWidth() * 0.4f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    chevron.startNewSubPath(cx - size, cy - size * 0.3f);
    chevron.lineTo(cx, cy + size * 0.3f);
    chevron.lineTo(cx + size, cy - size * 0.3f);

    // Apply rotation around center
    chevron.applyTransform(
        juce::AffineTransform::rotation(rotation, cx, cy)
    );

    g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void OscilDropdown::resized()
{
    // No child components in main dropdown
}

void OscilDropdown::mouseDown(const juce::MouseEvent&)
{
    if (!enabled_) return;

    if (popupVisible_)
        hidePopup();
    else
        showPopup();
}

void OscilDropdown::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 1.0f;
        repaint();
    }
}

void OscilDropdown::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 0.0f;
        repaint();
    }
}

bool OscilDropdown::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_) return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        if (popupVisible_)
            hidePopup();
        else
            showPopup();
        return true;
    }

    if (key == juce::KeyPress::escapeKey && popupVisible_)
    {
        hidePopup();
        return true;
    }

    // Arrow key navigation when closed
    if (!popupVisible_ && !items_.empty())
    {
        int current = getSelectedIndex();

        if (key == juce::KeyPress::upKey)
        {
            setSelectedIndex(std::max(0, current - 1));
            return true;
        }

        if (key == juce::KeyPress::downKey)
        {
            setSelectedIndex(std::min(static_cast<int>(items_.size()) - 1,
                                       current + 1));
            return true;
        }
    }

    return false;
}

void OscilDropdown::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilDropdown::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilDropdown::timerCallback()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    hoverSpring_.update(dt);
    chevronSpring_.update(dt);

    if (hoverSpring_.isSettled() && chevronSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilDropdown::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

// Custom accessibility handler for dropdown with selection announcements
class OscilDropdownAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilDropdownAccessibilityHandler(OscilDropdown& dropdown)
        : juce::AccessibilityHandler(dropdown, juce::AccessibilityRole::comboBox,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press, [&dropdown] { if (dropdown.isEnabled()) dropdown.showPopup(); })
                .addAction(juce::AccessibilityActionType::showMenu, [&dropdown] { if (dropdown.isEnabled()) dropdown.showPopup(); })
        )
        , dropdown_(dropdown)
    {
    }

    juce::String getTitle() const override
    {
        return dropdown_.getPlaceholder();
    }

    juce::String getDescription() const override
    {
        juce::String selection = dropdown_.getSelectedLabel();
        if (selection.isEmpty())
            return "No selection";

        int numItems = dropdown_.getNumItems();
        return "Selected: " + selection + " (" + juce::String(numItems) + " options available)";
    }

    juce::String getHelp() const override
    {
        return "Press Space or Enter to open dropdown. Use arrow keys to navigate.";
    }

private:
    OscilDropdown& dropdown_;
};

std::unique_ptr<juce::AccessibilityHandler> OscilDropdown::createAccessibilityHandler()
{
    return std::make_unique<OscilDropdownAccessibilityHandler>(*this);
}

} // namespace oscil
