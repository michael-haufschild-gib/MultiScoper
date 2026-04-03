/*
    Oscil - Dropdown Component Implementation
    (OscilDropdownPopup is in OscilDropdownPopup.cpp)
*/

#include "ui/components/OscilDropdown.h"

#include <utility>

namespace oscil
{

//==============================================================================
// OscilDropdown Implementation
//==============================================================================

OscilDropdown::OscilDropdown(IThemeService& themeService)
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

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder) : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
}

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder, const juce::String& testId)
    : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
    setTestId(testId);
}

void OscilDropdown::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilDropdown::~OscilDropdown() { stopTimer(); }

void OscilDropdown::addItem(const juce::String& label, const juce::String& id)
{
    DropdownItem item;
    item.id = id.isEmpty() ? label : id;
    item.label = label;
    items_.push_back(item);
}

void OscilDropdown::addItem(const DropdownItem& item) { items_.push_back(item); }

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
    if (index >= 0 && std::cmp_less(index, items_.size()))
        newSelection.insert(index);

    setSelectedIndices(newSelection, notify);
}

int OscilDropdown::getSelectedIndex() const { return selectedIndices_.empty() ? -1 : *selectedIndices_.begin(); }

juce::String OscilDropdown::getSelectedId() const
{
    int const index = getSelectedIndex();
    return (index >= 0 && std::cmp_less(index, items_.size())) ? items_[static_cast<size_t>(index)].id : juce::String();
}

juce::String OscilDropdown::getSelectedLabel() const
{
    int const index = getSelectedIndex();
    return (index >= 0 && std::cmp_less(index, items_.size())) ? items_[static_cast<size_t>(index)].label
                                                               : juce::String();
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
    for (int const index : selectedIndices_)
        if (index >= 0 && std::cmp_less(index, items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].id);
    return result;
}

std::vector<juce::String> OscilDropdown::getSelectedLabels() const
{
    std::vector<juce::String> result;
    for (int const index : selectedIndices_)
        if (index >= 0 && std::cmp_less(index, items_.size()))
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
            int const first = *selectedIndices_.begin();
            selectedIndices_.clear();
            selectedIndices_.insert(first);
            updateDisplayText();
        }
        repaint();
    }
}

void OscilDropdown::setSearchable(bool searchable) { searchable_ = searchable; }

void OscilDropdown::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilDropdown::showPopup()
{
    if (!enabled_ || items_.empty())
        return;

    popupVisible_ = true;

    popup_ = std::make_unique<OscilDropdownPopup>(getThemeService());
    popup_->setItems(items_);
    popup_->setSelectedIndices(selectedIndices_);
    popup_->setMultiSelect(multiSelect_);
    popup_->setSearchable(searchable_);

    popup_->onItemClicked = [this](int index) { handleItemClicked(index); };

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

void OscilDropdown::updateDisplayText()
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

int OscilDropdown::getPreferredWidth() const
{
    int maxWidth = 100;

    for (const auto& item : items_)
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, item.label, 0, 0);
        int const width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        maxWidth = std::max(maxWidth, width);
    }

    return maxWidth + (PADDING_H * 2) + CHEVRON_SIZE + 8;
}

int OscilDropdown::getPreferredHeight() const { return ComponentLayout::INPUT_HEIGHT; }

void OscilDropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const hoverAmount = hoverSpring_.position;

    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.05f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_SM);

    auto borderColour = popupVisible_ ? getTheme().controlActive
                        : hasFocus_   ? getTheme().controlActive
                                      : getTheme().controlBorder;

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);

    if (hasFocus_ && enabled_)
    {
        g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
                               ComponentLayout::RADIUS_SM + ComponentLayout::FOCUS_RING_OFFSET,
                               ComponentLayout::FOCUS_RING_WIDTH);
    }

    auto textBounds = bounds.reduced(PADDING_H, 0);
    textBounds.removeFromRight(CHEVRON_SIZE + 8);

    bool const isPlaceholder = selectedIndices_.empty();
    g.setColour((isPlaceholder ? getTheme().textSecondary : getTheme().textPrimary).withAlpha(opacity));
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
    g.drawText(displayText_, textBounds, juce::Justification::centredLeft);

    auto chevronBounds =
        bounds.removeFromRight(CHEVRON_SIZE + PADDING_H).withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);

    paintChevron(g, chevronBounds);
}

void OscilDropdown::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const rotation = chevronSpring_.position * juce::MathConstants<float>::pi;

    g.setColour(getTheme().textSecondary.withAlpha(opacity));

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

void OscilDropdown::resized() {}

// mouseDown, mouseEnter, mouseExit, keyPressed, focusGained, focusLost,
// timerCallback, createAccessibilityHandler are in OscilDropdownInteraction.cpp

} // namespace oscil
