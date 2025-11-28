/*
    Oscil - Dropdown Component Implementation
*/

#include "ui/components/OscilDropdown.h"

namespace oscil
{

//==============================================================================
// OscilDropdownPopup Implementation
//==============================================================================

OscilDropdownPopup::OscilDropdownPopup()
    : showSpring_(SpringPresets::snappy())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    showSpring_.position = 0.0f;
    showSpring_.target = 0.0f;
}

OscilDropdownPopup::~OscilDropdownPopup()
{
    ThemeManager::getInstance().removeListener(this);
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

void OscilDropdownPopup::setSearchable(bool searchable)
{
    searchable_ = searchable;

    if (searchable && !searchField_)
    {
        searchField_ = std::make_unique<juce::TextEditor>();
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
    int numVisible = std::min(static_cast<int>(filteredIndices_.size()), MAX_VISIBLE_ITEMS);
    int contentHeight = numVisible * ITEM_HEIGHT;
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int totalHeight = contentHeight + searchHeight + POPUP_PADDING * 2;
    int width = buttonBounds.getWidth();

    // Position below or above the button
    auto screenBounds = parent->getScreenBounds();
    auto displayBounds = juce::Desktop::getInstance().getDisplays()
        .getDisplayForPoint(screenBounds.getCentre())->userArea;

    int x = screenBounds.getX() + buttonBounds.getX();
    int y = screenBounds.getY() + buttonBounds.getBottom();

    // Flip above if not enough space below
    if (y + totalHeight > displayBounds.getBottom())
        y = screenBounds.getY() + buttonBounds.getY() - totalHeight;

    setBounds(x, y, width, totalHeight);
    addToDesktop(juce::ComponentPeer::windowIsTemporary |
                 juce::ComponentPeer::windowHasDropShadow);

    setVisible(true);
    grabKeyboardFocus();

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

    // Draw items
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int y = POPUP_PADDING + searchHeight;

    for (size_t i = 0; i < filteredIndices_.size() && i < MAX_VISIBLE_ITEMS; ++i)
    {
        int itemIndex = filteredIndices_[i];
        const auto& item = items_[static_cast<size_t>(itemIndex)];
        auto itemBounds = juce::Rectangle<int>(POPUP_PADDING, y,
            getWidth() - POPUP_PADDING * 2, ITEM_HEIGHT);

        bool isHovered = (static_cast<int>(i) == hoveredIndex_);
        bool isSelected = selectedIndices_.count(itemIndex) > 0;

        paintItem(g, item, itemBounds, isHovered, isSelected);

        y += ITEM_HEIGHT;
    }
}

void OscilDropdownPopup::paintItem(juce::Graphics& g, const DropdownItem& item,
                                    juce::Rectangle<int> bounds, bool isHovered, bool isSelected)
{
    float alpha = showSpring_.position;

    if (item.isSeparator)
    {
        g.setColour(theme_.controlBorder.withAlpha(alpha * 0.5f));
        g.fillRect(bounds.reduced(8, ITEM_HEIGHT / 2 - 1).withHeight(1));
        return;
    }

    float opacity = item.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Hover/selected background
    if (isSelected)
    {
        g.setColour(theme_.controlActive.withAlpha(alpha * 0.15f));
        g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);
    }
    else if (isHovered && item.enabled)
    {
        g.setColour(theme_.backgroundSecondary.withAlpha(alpha * 0.8f));
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
    if (multiSelect_)
    {
        auto checkBounds = contentBounds.removeFromLeft(20).toFloat()
            .withSizeKeepingCentre(16, 16);

        g.setColour(theme_.backgroundSecondary.withAlpha(alpha * opacity));
        g.fillRoundedRectangle(checkBounds, 3.0f);

        g.setColour((isSelected ? theme_.controlActive : theme_.controlBorder)
            .withAlpha(alpha * opacity));
        g.drawRoundedRectangle(checkBounds.reduced(0.5f), 3.0f, 1.0f);

        if (isSelected)
        {
            g.setColour(theme_.controlActive.withAlpha(alpha * opacity));
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
    g.setColour((isSelected ? theme_.controlActive : theme_.textPrimary)
        .withAlpha(alpha * opacity));
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));

    if (item.description.isNotEmpty())
    {
        auto labelBounds = contentBounds.removeFromTop(ITEM_HEIGHT / 2 + 2);
        g.drawText(item.label, labelBounds, juce::Justification::centredLeft);

        g.setColour(theme_.textSecondary.withAlpha(alpha * opacity * 0.8f));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(item.description, contentBounds, juce::Justification::centredLeft);
    }
    else
    {
        g.drawText(item.label, contentBounds, juce::Justification::centredLeft);
    }
}

void OscilDropdownPopup::resized()
{
    if (searchField_)
    {
        searchField_->setBounds(POPUP_PADDING, POPUP_PADDING,
            getWidth() - POPUP_PADDING * 2, SEARCH_HEIGHT - POPUP_PADDING);
    }
}

void OscilDropdownPopup::mouseDown(const juce::MouseEvent& e)
{
    int index = getItemAtPosition(e.getPosition());

    if (index >= 0 && index < static_cast<int>(filteredIndices_.size()))
    {
        int itemIndex = filteredIndices_[static_cast<size_t>(index)];
        const auto& item = items_[static_cast<size_t>(itemIndex)];

        if (!item.isSeparator && item.enabled)
        {
            if (onItemClicked)
                onItemClicked(itemIndex);

            if (!multiSelect_)
                dismiss();
        }
    }
}

void OscilDropdownPopup::mouseMove(const juce::MouseEvent& e)
{
    int newHovered = getItemAtPosition(e.getPosition());
    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();
    }
}

void OscilDropdownPopup::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();
    }
}

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
        repaint();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        focusedIndex_ = std::min(static_cast<int>(filteredIndices_.size()) - 1,
                                  focusedIndex_ + 1);
        repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey && focusedIndex_ >= 0)
    {
        int itemIndex = filteredIndices_[static_cast<size_t>(focusedIndex_)];
        const auto& item = items_[static_cast<size_t>(itemIndex)];

        if (!item.isSeparator && item.enabled && onItemClicked)
        {
            onItemClicked(itemIndex);
            if (!multiSelect_)
                dismiss();
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

int OscilDropdownPopup::getItemAtPosition(juce::Point<int> pos) const
{
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int y = POPUP_PADDING + searchHeight;

    for (size_t i = 0; i < filteredIndices_.size() && i < MAX_VISIBLE_ITEMS; ++i)
    {
        if (pos.y >= y && pos.y < y + ITEM_HEIGHT)
            return static_cast<int>(i);
        y += ITEM_HEIGHT;
    }

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

OscilDropdown::OscilDropdown()
    : hoverSpring_(SpringPresets::stiff())
    , chevronSpring_(SpringPresets::bouncy())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
    chevronSpring_.position = 0.0f;
    chevronSpring_.target = 0.0f;
}

OscilDropdown::OscilDropdown(const juce::String& placeholder)
    : OscilDropdown()
{
    placeholder_ = placeholder;
}

OscilDropdown::~OscilDropdown()
{
    ThemeManager::getInstance().removeListener(this);
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
    return (index >= 0) ? items_[static_cast<size_t>(index)].id : juce::String();
}

juce::String OscilDropdown::getSelectedLabel() const
{
    int index = getSelectedIndex();
    return (index >= 0) ? items_[static_cast<size_t>(index)].label : juce::String();
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

    popup_ = std::make_unique<OscilDropdownPopup>();
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
        displayText_ = items_[static_cast<size_t>(index)].label;
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

std::unique_ptr<juce::AccessibilityHandler> OscilDropdown::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::comboBox,
        juce::AccessibilityActions()
            .addAction(juce::AccessibilityActionType::press,
                [this] { if (enabled_) showPopup(); })
    );
}

} // namespace oscil
