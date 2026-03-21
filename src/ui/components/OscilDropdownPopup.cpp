/*
    Oscil - Dropdown Popup Component Implementation
    Inner widget definitions and constructor/setup
    Event handling is in OscilDropdownPopupEvents.cpp
*/

#include "ui/components/OscilDropdown.h"

namespace oscil
{

//==============================================================================
// OscilDropdownPopup::ItemList (inner component for rendering items)
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
        const auto& theme = owner_.getTheme();

        if (item.isSeparator)
        {
            g.setColour(theme.controlBorder.withAlpha(alpha * 0.5f));
            g.fillRect(bounds.reduced(8, ITEM_HEIGHT / 2 - 1).withHeight(1));
            return;
        }

        float opacity = item.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

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

        if (item.icon.isValid())
        {
            auto iconBounds = contentBounds.removeFromLeft(20).toFloat();
            g.setOpacity(alpha * opacity);
            g.drawImage(item.icon,
                iconBounds.withSizeKeepingCentre(16, 16),
                juce::RectanglePlacement::centred);
            contentBounds.removeFromLeft(4);
        }

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

//==============================================================================
// SearchField (key delegation to popup)
//==============================================================================

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

//==============================================================================
// OscilDropdownPopup Constructor / Setup
//==============================================================================

OscilDropdownPopup::OscilDropdownPopup(IThemeService& themeService)
    : ThemedComponent(themeService)
    , showSpring_(SpringPresets::snappy())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);

    getProperties().set("isOscilPopup", true);

    showSpring_.position = 0.0f;
    showSpring_.target = 0.0f;

    listComponent_ = std::make_unique<ItemList>(*this);
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setViewedComponent(listComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    viewport_->setScrollBarThickness(6);
    addAndMakeVisible(*viewport_);
}

OscilDropdownPopup::~OscilDropdownPopup()
{
    if (searchField_)
    {
        searchField_->onTextChange = nullptr;
    }

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
        searchField_ = std::make_unique<SearchField>(*this);
        searchField_->setMultiLine(false);
        searchField_->setTextToShowWhenEmpty("Search...", getTheme().textSecondary);
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

    int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
    int viewportHeight = std::min(contentHeight, MAX_VISIBLE_ITEMS * ITEM_HEIGHT);
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int totalHeight = viewportHeight + searchHeight + POPUP_PADDING * 2;
    int width = buttonBounds.getWidth();

    auto screenBounds = parent->getScreenBounds();
    auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(screenBounds.getCentre());
    if (!display) display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (!display) return;

    int x = screenBounds.getX() + buttonBounds.getX();
    int y = screenBounds.getY() + buttonBounds.getBottom();
    if (y + totalHeight > display->userArea.getBottom())
        y = screenBounds.getY() + buttonBounds.getY() - totalHeight;

    setBounds(x, y, width, totalHeight);
    addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
    setVisible(true);

    if (listComponent_)
        listComponent_->setSize(juce::jmax(1, width - POPUP_PADDING * 2), contentHeight);

    resized();
    grabKeyboardFocus();
    focusOnFirstSelected();

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 1.0f;
    }

    if (searchField_) searchField_->grabKeyboardFocus();
}

void OscilDropdownPopup::focusOnFirstSelected()
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
        if (item.isSeparator ||
            searchText_.isEmpty() ||
            item.label.containsIgnoreCase(searchText_) ||
            item.description.containsIgnoreCase(searchText_))
        {
            filteredIndices_.push_back(static_cast<int>(i));
        }
    }

    if (listComponent_)
    {
        int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
        listComponent_->setSize(listComponent_->getWidth(), contentHeight);
    }

    focusedIndex_ = juce::jlimit(-1, static_cast<int>(filteredIndices_.size()) - 1, focusedIndex_);
    if (focusedIndex_ < 0 && !filteredIndices_.empty())
        focusedIndex_ = 0;
}

void OscilDropdownPopup::paint(juce::Graphics& g)
{
    float alpha = showSpring_.position;
    if (alpha < 0.01f) return;

    auto bounds = getLocalBounds().toFloat();
    g.setColour(getTheme().backgroundPrimary.withAlpha(alpha * 0.95f));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);
    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);
}

void OscilDropdownPopup::resized()
{
    auto bounds = getLocalBounds().reduced(POPUP_PADDING);
    if (searchable_ && searchField_)
    {
        searchField_->setBounds(bounds.removeFromTop(SEARCH_HEIGHT - 4));
        bounds.removeFromTop(4);
    }
    if (viewport_) viewport_->setBounds(bounds);
}

void OscilDropdownPopup::mouseDown(const juce::MouseEvent&) { dismiss(); }
void OscilDropdownPopup::mouseMove(const juce::MouseEvent&) {}
void OscilDropdownPopup::mouseExit(const juce::MouseEvent&) {}

bool OscilDropdownPopup::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) { dismiss(); return true; }

    if (key == juce::KeyPress::upKey)
    {
        focusedIndex_ = std::max(0, focusedIndex_ - 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        focusedIndex_ = std::min(static_cast<int>(filteredIndices_.size()) - 1, focusedIndex_ + 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey && focusedIndex_ >= 0
        && focusedIndex_ < static_cast<int>(filteredIndices_.size()))
    {
        int itemIndex = filteredIndices_[static_cast<size_t>(focusedIndex_)];
        const auto& item = items_[static_cast<size_t>(itemIndex)];
        if (!item.isSeparator && item.enabled && onItemClicked)
        {
            onItemClicked(itemIndex);
            if (!multiSelect_) dismiss();
        }
        return true;
    }

    return false;
}

void OscilDropdownPopup::focusLost(FocusChangeType)
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusedComp != this && !isParentOf(focusedComp))
        dismiss();
}

void OscilDropdownPopup::ensureItemVisible(int index)
{
    if (!viewport_ || index < 0) return;
    int itemY = index * ITEM_HEIGHT;
    int viewY = viewport_->getViewPositionY();
    int viewH = viewport_->getViewHeight();

    if (itemY < viewY)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY);
    else if (itemY + ITEM_HEIGHT > viewY + viewH)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY + ITEM_HEIGHT - viewH);
}

int OscilDropdownPopup::getItemAtPosition(juce::Point<int>) const { return -1; }

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

} // namespace oscil
