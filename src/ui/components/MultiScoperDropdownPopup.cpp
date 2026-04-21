/*
    MultiScoper - Dropdown Popup Component Implementation
    Inner widget definitions and constructor/setup
    Event handling is in MultiScoperDropdownPopupEvents.cpp
*/

#include "ui/components/MultiScoperDropdown.h"
#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

#include <utility>

namespace multiscoper
{

//==============================================================================
// MultiScoperDropdownPopup::ItemList (inner component for rendering items)
//==============================================================================

class MultiScoperDropdownPopup::ItemList : public juce::Component
{
public:
    explicit ItemList(MultiScoperDropdownPopup& owner) : owner_(owner)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics& g) override
    {
        float const alpha = owner_.showSpring_.position;
        if (alpha < 0.01f)
            return;

        int start = g.getClipBounds().getY() / ITEM_HEIGHT;
        int end = (g.getClipBounds().getBottom() + ITEM_HEIGHT) / ITEM_HEIGHT;

        start = std::max(0, start);
        end = std::min(end, static_cast<int>(owner_.filteredIndices_.size()));

        for (int i = start; i < end; ++i)
        {
            int const itemIndex = owner_.filteredIndices_[static_cast<size_t>(i)];
            const auto& item = owner_.items_[static_cast<size_t>(itemIndex)];
            auto itemBounds = juce::Rectangle<int>(0, i * ITEM_HEIGHT, getWidth(), ITEM_HEIGHT);

            bool const isHovered = (itemIndex == owner_.hoveredIndex_);
            bool const isSelected = owner_.selectedIndices_.contains(itemIndex);
            bool const isFocused = (i == owner_.focusedIndex_);

            paintItem(g, item, itemBounds, isHovered, isSelected, isFocused, alpha);
        }
    }

    void paintItemBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds, bool isHovered, bool isSelected,
                             bool isFocused, bool enabled, float alpha, const SurfaceStyle& glass)
    {
        if (isSelected)
        {
            // Selected: accentSubtle fill (multiply animation alpha into the
            // designed tint — replacing it would paint a solid accent row).
            g.setColour(glass.accentSubtle.withMultipliedAlpha(alpha));
            g.fillRect(bounds.toFloat());
        }
        else if (isFocused)
        {
            g.setColour(glass.accent.withAlpha(alpha * 0.1f));
            g.drawRect(bounds.toFloat(), 1.0f);
        }

        if (isHovered && enabled && !isSelected)
        {
            // Hover: bgHover fill (multiply animation alpha — bgHover already
            // carries a 0.08 design tint).
            g.setColour(glass.bgHover.withMultipliedAlpha(alpha));
            g.fillRect(bounds.toFloat());
        }
    }

    void paintCheckbox(juce::Graphics& g, juce::Rectangle<int>& contentBounds, bool isSelected, float alpha,
                       float opacity, const SurfaceStyle& glass)
    {
        auto checkBounds = contentBounds.removeFromLeft(20).toFloat().withSizeKeepingCentre(16, 16);

        g.setColour(glass.bgGlass.withMultipliedAlpha(alpha * opacity));
        g.fillRect(checkBounds);

        // accent/borderDefault already have designed alphas; multiply so
        // the animation/disabled fade scales them rather than replacing them.
        g.setColour((isSelected ? glass.accent : glass.borderDefault).withMultipliedAlpha(alpha * opacity));
        g.drawRect(checkBounds, 1.0f);

        if (isSelected)
        {
            g.setColour(glass.accent.withMultipliedAlpha(alpha * opacity));
            float const cx = checkBounds.getCentreX();
            float const cy = checkBounds.getCentreY();
            juce::Path checkPath;
            checkPath.startNewSubPath(cx - 4, cy);
            checkPath.lineTo(cx - 1, cy + 3);
            checkPath.lineTo(cx + 4, cy - 2);
            g.strokePath(checkPath, juce::PathStrokeType(1.5f));
        }

        contentBounds.removeFromLeft(4);
    }

    void paintItem(juce::Graphics& g, const DropdownItem& item, juce::Rectangle<int> bounds, bool isHovered,
                   bool isSelected, bool isFocused, float alpha)
    {
        const auto& glass = owner_.getSurface();
        const auto& theme = owner_.getTheme();

        if (item.isSeparator)
        {
            // Separator: borderSubtle 1px line (multiply animation alpha)
            g.setColour(glass.borderSubtle.withMultipliedAlpha(alpha));
            g.fillRect(bounds.reduced(8, (ITEM_HEIGHT / 2) - 1).withHeight(1));
            return;
        }

        float const opacity = item.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;
        paintItemBackground(g, bounds, isHovered, isSelected, isFocused, item.enabled, alpha, glass);

        auto contentBounds = bounds.reduced(8, 0);

        if (item.icon.isValid())
        {
            auto iconBounds = contentBounds.removeFromLeft(20).toFloat();
            g.setOpacity(alpha * opacity);
            g.drawImage(item.icon, iconBounds.withSizeKeepingCentre(16, 16), juce::RectanglePlacement::centred);
            contentBounds.removeFromLeft(4);
        }

        if (owner_.multiSelect_)
            paintCheckbox(g, contentBounds, isSelected, alpha, opacity, glass);

        // Selected: accent text; default: textPrimary
        g.setColour((isSelected ? glass.accent : theme.textPrimary).withAlpha(alpha * opacity));

        static const juce::Font itemFont(Typography::body());
        g.setFont(itemFont);

        if (item.description.isNotEmpty())
        {
            auto labelBounds = contentBounds.removeFromTop((ITEM_HEIGHT / 2) + 2);
            g.drawText(item.label, labelBounds, juce::Justification::centredLeft);

            g.setColour(theme.textSecondary.withAlpha(alpha * opacity * 0.8f));
            static const juce::Font descFont(Typography::caption());
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
        int const index = e.y / ITEM_HEIGHT;
        if (index >= 0 && std::cmp_less(index, owner_.filteredIndices_.size()))
        {
            int const itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
            if (owner_.hoveredIndex_ != itemIndex)
            {
                owner_.hoveredIndex_ = itemIndex;
                repaint();
            }
        }
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override
    {
        if (owner_.hoveredIndex_ != -1)
        {
            owner_.hoveredIndex_ = -1;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int const index = e.y / ITEM_HEIGHT;
        if (index >= 0 && std::cmp_less(index, owner_.filteredIndices_.size()))
        {
            int const itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
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
    MultiScoperDropdownPopup& owner_;
};

//==============================================================================
// SearchField (key delegation to popup)
//==============================================================================

class SearchField : public juce::TextEditor
{
public:
    SearchField(MultiScoperDropdownPopup& owner) : owner_(owner) {}

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey || key == juce::KeyPress::escapeKey)
        {
            return owner_.keyPressed(key);
        }

        if (key == juce::KeyPress::returnKey)
        {
            if (owner_.keyPressed(key))
                return true;
        }

        return juce::TextEditor::keyPressed(key);
    }

private:
    MultiScoperDropdownPopup& owner_;
};

//==============================================================================
// MultiScoperDropdownPopup Constructor / Setup
//==============================================================================

MultiScoperDropdownPopup::MultiScoperDropdownPopup(IThemeService& themeService)
    : ThemedComponent(themeService)
    , showSpring_(SpringPresets::springPopup())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);

    getProperties().set("isMultiScoperPopup", true);

    showSpring_.position = 0.0f;
    showSpring_.target = 0.0f;

    listComponent_ = std::make_unique<ItemList>(*this);
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setViewedComponent(listComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    viewport_->setScrollBarThickness(6);
    addAndMakeVisible(*viewport_);
}

MultiScoperDropdownPopup::~MultiScoperDropdownPopup()
{
    if (searchField_)
    {
        searchField_->onTextChange = nullptr;
    }

    stopTimer();
}

void MultiScoperDropdownPopup::setItems(const std::vector<DropdownItem>& items)
{
    items_ = items;
    updateFilteredItems();
}

void MultiScoperDropdownPopup::setSelectedIndices(const std::set<int>& indices)
{
    selectedIndices_ = indices;
    repaint();
}

void MultiScoperDropdownPopup::setSearchable(bool searchable)
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

void MultiScoperDropdownPopup::show(juce::Component* parent, juce::Rectangle<int> buttonBounds)
{
    if (!parent)
        return;
    updateFilteredItems();

    int const contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
    int const viewportHeight = std::min(contentHeight, MAX_VISIBLE_ITEMS * ITEM_HEIGHT);
    int const searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int const totalHeight = viewportHeight + searchHeight + (POPUP_PADDING * 2);
    int const width = buttonBounds.getWidth();

    auto screenBounds = parent->getScreenBounds();
    const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(screenBounds.getCentre());
    if (!display)
        display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (!display)
        return;

    int const x = screenBounds.getX() + buttonBounds.getX();
    int y = screenBounds.getY() + buttonBounds.getBottom();
    if (y + totalHeight > display->userArea.getBottom())
        y = screenBounds.getY() + buttonBounds.getY() - totalHeight;

    setBounds(x, y, width, totalHeight);
    addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
    setVisible(true);

    if (listComponent_)
        listComponent_->setSize(juce::jmax(1, width - (POPUP_PADDING * 2)), contentHeight);

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

    if (searchField_)
        searchField_->grabKeyboardFocus();
}

void MultiScoperDropdownPopup::focusOnFirstSelected()
{
    for (size_t i = 0; i < filteredIndices_.size(); ++i)
    {
        if (selectedIndices_.contains(filteredIndices_[i]))
        {
            focusedIndex_ = static_cast<int>(i);
            ensureItemVisible(focusedIndex_);
            break;
        }
    }
}

void MultiScoperDropdownPopup::dismiss()
{
    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        setVisible(false);
        if (onDismiss)
            onDismiss();
    }
}

void MultiScoperDropdownPopup::updateFilteredItems()
{
    filteredIndices_.clear();

    const bool searchActive = searchText_.isNotEmpty();

    for (size_t i = 0; i < items_.size(); ++i)
    {
        const auto& item = items_[i];
        if (item.isSeparator)
        {
            // Drop separators entirely while a search is active — they
            // become orphan dividers between unrelated matched items and
            // look like bugs. Also collapse consecutive separators to one,
            // and never lead with a separator.
            if (searchActive)
                continue;
            if (filteredIndices_.empty())
                continue;
            const int lastItemIndex = filteredIndices_.back();
            if (items_[static_cast<size_t>(lastItemIndex)].isSeparator)
                continue;
            filteredIndices_.push_back(static_cast<int>(i));
            continue;
        }

        if (!searchActive || item.label.containsIgnoreCase(searchText_) ||
            item.description.containsIgnoreCase(searchText_))
        {
            filteredIndices_.push_back(static_cast<int>(i));
        }
    }

    // Trailing-separator trim for the no-search case.
    while (!filteredIndices_.empty() && items_[static_cast<size_t>(filteredIndices_.back())].isSeparator)
        filteredIndices_.pop_back();

    if (listComponent_)
    {
        int const contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
        listComponent_->setSize(listComponent_->getWidth(), contentHeight);
        // setSize only triggers repaint on dimension change. When the filter
        // changes the item set but not the count — e.g. typing narrows from
        // one group of matches to another of equal size — the list must be
        // repainted explicitly or stale item labels remain on screen.
        listComponent_->repaint();
    }

    focusedIndex_ = juce::jlimit(-1, static_cast<int>(filteredIndices_.size()) - 1, focusedIndex_);
    if (focusedIndex_ < 0 && !filteredIndices_.empty())
        focusedIndex_ = 0;
}

void MultiScoperDropdownPopup::paint(juce::Graphics& g)
{
    float const alpha = showSpring_.getNormalized();
    if (alpha < 0.01f)
        return;

    auto bounds = getLocalBounds().toFloat();

    // Apply scale animation: scale from 0.95 to 1.0 as spring opens
    float const scale = 0.95f + 0.05f * alpha;
    auto scaledBounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale, bounds.getHeight() * scale);

    g.setOpacity(alpha);

    // Use SurfacePainter::paintPanel for popup background
    SurfacePainter::paintPanel(g, scaledBounds, getSurface(), ComponentLayout::RADIUS_XL, BorderLevel::Default);

    g.setOpacity(1.0f);
}

void MultiScoperDropdownPopup::resized()
{
    auto bounds = getLocalBounds().reduced(POPUP_PADDING);
    if (searchable_ && searchField_)
    {
        searchField_->setBounds(bounds.removeFromTop(SEARCH_HEIGHT - 4));
        bounds.removeFromTop(4);
    }
    if (viewport_)
        viewport_->setBounds(bounds);
}

// MultiScoperDropdownPopup::keyPressed lives in MultiScoperDropdownPopupEvents.cpp

void MultiScoperDropdownPopup::timerCallback()
{
    showSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);

    if (showSpring_.isSettled())
    {
        stopTimer();
        if (showSpring_.position < 0.1f)
        {
            setVisible(false);
            if (onDismiss)
                onDismiss();
        }
    }

    if (listComponent_)
        listComponent_->repaint();
    repaint();
}

} // namespace multiscoper
