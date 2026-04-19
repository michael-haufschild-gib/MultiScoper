/*
    MultiScoper - Dropdown Component Implementation
    (MultiScoperDropdownPopup is in MultiScoperDropdownPopup.cpp)
*/

#include "ui/components/MultiScoperDropdown.h"

#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

#include <utility>

namespace multiscoper
{

//==============================================================================
// MultiScoperDropdown Implementation
//==============================================================================

MultiScoperDropdown::MultiScoperDropdown(IThemeService& themeService)
    : ThemedComponent(themeService)
    , hoverSpring_(SpringPresets::fast())
    , chevronSpring_(SpringPresets::medium())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
    chevronSpring_.position = 0.0f;
    chevronSpring_.target = 0.0f;
}

MultiScoperDropdown::MultiScoperDropdown(IThemeService& themeService, const juce::String& placeholder)
    : MultiScoperDropdown(themeService)
{
    placeholder_ = placeholder;
}

MultiScoperDropdown::MultiScoperDropdown(IThemeService& themeService, const juce::String& placeholder,
                                         const juce::String& testId)
    : MultiScoperDropdown(themeService)
{
    placeholder_ = placeholder;
    setTestId(testId);
}

void MultiScoperDropdown::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperDropdown::~MultiScoperDropdown() { stopTimer(); }

void MultiScoperDropdown::addItem(const juce::String& label, const juce::String& id)
{
    DropdownItem item;
    item.id = id.isEmpty() ? label : id;
    item.label = label;
    items_.push_back(item);
}

void MultiScoperDropdown::addItem(const DropdownItem& item) { items_.push_back(item); }

void MultiScoperDropdown::addItems(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
        addItem(label);
}

void MultiScoperDropdown::addItems(const std::vector<DropdownItem>& items)
{
    for (const auto& item : items)
        addItem(item);
}

void MultiScoperDropdown::clearItems()
{
    items_.clear();
    selectedIndices_.clear();
    updateDisplayText();
    repaint();
}

void MultiScoperDropdown::setSelectedIndex(int index, bool notify)
{
    std::set<int> newSelection;
    if (index >= 0 && std::cmp_less(index, items_.size()))
        newSelection.insert(index);

    setSelectedIndices(newSelection, notify);
}

int MultiScoperDropdown::getSelectedIndex() const { return selectedIndices_.empty() ? -1 : *selectedIndices_.begin(); }

juce::String MultiScoperDropdown::getSelectedId() const
{
    int const index = getSelectedIndex();
    return (index >= 0 && std::cmp_less(index, items_.size())) ? items_[static_cast<size_t>(index)].id : juce::String();
}

juce::String MultiScoperDropdown::getSelectedLabel() const
{
    int const index = getSelectedIndex();
    return (index >= 0 && std::cmp_less(index, items_.size())) ? items_[static_cast<size_t>(index)].label
                                                               : juce::String();
}

void MultiScoperDropdown::setSelectedIndices(const std::set<int>& indices, bool notify)
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

std::vector<juce::String> MultiScoperDropdown::getSelectedIds() const
{
    std::vector<juce::String> result;
    for (int const index : selectedIndices_)
        if (index >= 0 && std::cmp_less(index, items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].id);
    return result;
}

std::vector<juce::String> MultiScoperDropdown::getSelectedLabels() const
{
    std::vector<juce::String> result;
    for (int const index : selectedIndices_)
        if (index >= 0 && std::cmp_less(index, items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].label);
    return result;
}

void MultiScoperDropdown::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    updateDisplayText();
    repaint();
}

void MultiScoperDropdown::setMultiSelect(bool multiSelect)
{
    if (multiSelect_ == multiSelect)
        return;

    multiSelect_ = multiSelect;

    const bool collapsed = !multiSelect && selectedIndices_.size() > 1;
    if (collapsed)
    {
        int const first = *selectedIndices_.begin();
        selectedIndices_.clear();
        selectedIndices_.insert(first);
        updateDisplayText();

        // Selection cardinality changed from N → 1 — notify listeners so
        // they don't carry a stale view of the multi-selection.
        if (onMultiSelectionChanged)
            onMultiSelectionChanged(selectedIndices_);
        if (onSelectionChanged)
            onSelectionChanged(getSelectedIndex());
        if (onSelectionChangedId)
            onSelectionChangedId(getSelectedId());
    }

    repaint();
}

void MultiScoperDropdown::setSearchable(bool searchable) { searchable_ = searchable; }

void MultiScoperDropdown::setTooltip(const juce::String& tooltip)
{
    tooltipText_ = tooltip;
    setHelpText(tooltip);
}

void MultiScoperDropdown::enablementChanged()
{
    setMouseCursor(isEnabled() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void MultiScoperDropdown::showPopup()
{
    if (!isEnabled() || items_.empty())
        return;

    popupVisible_ = true;

    popup_ = std::make_unique<MultiScoperDropdownPopup>(getThemeService());
    popup_->setItems(items_);
    popup_->setSelectedIndices(selectedIndices_);
    popup_->setMultiSelect(multiSelect_);
    popup_->setSearchable(searchable_);

    popup_->onItemClicked = [this](int index) { handleItemClicked(index); };

    // The popup invokes onDismiss synchronously from its own methods
    // (timerCallback under spring-animated dismiss, or dismiss() directly
    // when AnimationSettings::shouldUseSpringAnimations() is false —
    // i.e., under accessibility reduced-motion). Destroying the popup
    // from within a method of the popup itself produces a UAF when the
    // surrounding frame (JUCE Timer dispatcher, mouseDown dispatcher,
    // focusLost dispatcher) accesses `this` after the callback returns.
    //
    // Defer popup destruction to the message loop via callAsync. Guard
    // against (a) the dropdown being destroyed before the async fires
    // (SafePointer) and (b) the popup being replaced by a subsequent
    // showPopup() call (pointer-identity check).
    auto* popupPtr = popup_.get();
    popup_->onDismiss = [safeThis = juce::Component::SafePointer<MultiScoperDropdown>(this), popupPtr]() {
        if (!safeThis)
            return;
        safeThis->popupVisible_ = false;
        safeThis->repaint();
        juce::MessageManager::callAsync([safeThis, popupPtr]() {
            if (!safeThis)
                return;
            if (safeThis->popup_.get() != popupPtr)
                return; // popup was already replaced by a newer showPopup()
            safeThis->popup_.reset();
            safeThis->chevronSpring_.setTarget(0.0f);
            safeThis->startTimerHz(ComponentLayout::ANIMATION_FPS);
        });
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

void MultiScoperDropdown::hidePopup()
{
    if (popup_)
        popup_->dismiss();
}

void MultiScoperDropdown::handleItemClicked(int index)
{
    if (multiSelect_)
    {
        if (selectedIndices_.contains(index))
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

void MultiScoperDropdown::updateDisplayText()
{
    if (selectedIndices_.empty())
    {
        displayText_ = placeholder_;
    }
    else if (selectedIndices_.size() == 1)
    {
        int const index = *selectedIndices_.begin();
        if (index >= 0 && std::cmp_less(index, items_.size()))
            displayText_ = items_[static_cast<size_t>(index)].label;
        else
            displayText_ = placeholder_;
    }
    else
    {
        displayText_ = juce::String(selectedIndices_.size()) + " selected";
    }
}

int MultiScoperDropdown::getPreferredWidth() const
{
    int maxWidth = 100;

    for (const auto& item : items_)
    {
        auto font = Typography::body();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, item.label, 0, 0);
        int const width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        maxWidth = std::max(maxWidth, width);
    }

    return maxWidth + (PADDING_H * 2) + CHEVRON_SIZE + 8;
}

int MultiScoperDropdown::getPreferredHeight() const { return ComponentLayout::INPUT_HEIGHT; }

void MultiScoperDropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = isEnabled() ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    if (!isEnabled())
        g.setOpacity(opacity);

    // Use SurfacePainter::paintInput for the trigger area
    bool const isFocused = hasFocus_ || popupVisible_;
    SurfacePainter::paintInput(g, bounds, getSurface(), 0.0f, isFocused, isHovered_);

    // Focus ring when focused
    if (hasFocus_ && isEnabled())
        SurfacePainter::paintFocusRing(g, bounds, 0.0f, getSurface().accent);

    // Text
    auto textBounds = bounds.reduced(PADDING_H, 0);
    textBounds.removeFromRight(CHEVRON_SIZE + 8);

    bool const isPlaceholder = selectedIndices_.empty();
    g.setColour((isPlaceholder ? getTheme().textSecondary : getTheme().textPrimary).withAlpha(opacity));
    g.setFont(Typography::body());
    g.drawText(displayText_, textBounds, juce::Justification::centredLeft);

    // Chevron
    auto chevronBounds =
        bounds.removeFromRight(CHEVRON_SIZE + PADDING_H).withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);

    paintChevron(g, chevronBounds);

    if (!isEnabled())
        g.setOpacity(1.0f);
}

void MultiScoperDropdown::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float const opacity = isEnabled() ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const rotation = chevronSpring_.position * juce::MathConstants<float>::pi;

    // Chevron: textSecondary default (tertiary feel), textPrimary on hover
    auto chevronColour = isHovered_ ? getTheme().textPrimary : getTheme().textSecondary;
    g.setColour(chevronColour.withAlpha(opacity * 0.7f));

    juce::Path chevron;
    float const size = bounds.getWidth() * 0.4f;
    float const cx = bounds.getCentreX();
    float const cy = bounds.getCentreY();

    chevron.startNewSubPath(cx - size, cy - (size * 0.3f));
    chevron.lineTo(cx, cy + (size * 0.3f));
    chevron.lineTo(cx + size, cy - (size * 0.3f));

    chevron.applyTransform(juce::AffineTransform::rotation(rotation, cx, cy));

    g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MultiScoperDropdown::resized() {}

// mouseDown, mouseEnter, mouseExit, keyPressed, focusGained, focusLost,
// timerCallback, createAccessibilityHandler are in MultiScoperDropdownInteraction.cpp

} // namespace multiscoper
