/*
    MultiScoper - Radio Button and Radio Group Implementation
*/

#include "ui/components/MultiScoperRadioButton.h"

#include <utility>

namespace multiscoper
{

//==============================================================================
// MultiScoperRadioButton Implementation
//==============================================================================

MultiScoperRadioButton::MultiScoperRadioButton(IThemeService& themeService)
    : ThemedComponent(themeService)
    , selectionSpring_(SpringPresets::medium())
    , hoverSpring_(SpringPresets::fast())
    , scaleSpring_(SpringPresets::springGentle())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    selectionSpring_.position = 0.0f;
    selectionSpring_.target = 0.0f;
    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
    scaleSpring_.position = 1.0f;
    scaleSpring_.target = 1.0f;
}

MultiScoperRadioButton::MultiScoperRadioButton(IThemeService& themeService, const juce::String& label)
    : MultiScoperRadioButton(themeService)
{
    label_ = label;
}

MultiScoperRadioButton::MultiScoperRadioButton(IThemeService& themeService, const juce::String& label,
                                               const juce::String& testId)
    : MultiScoperRadioButton(themeService)
{
    label_ = label;
    setTestId(testId);
}

void MultiScoperRadioButton::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperRadioButton::~MultiScoperRadioButton() { stopTimer(); }

void MultiScoperRadioButton::setSelected(bool selected, bool notify)
{
    if (selected_ == selected)
        return;

    selected_ = selected;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        selectionSpring_.setTarget(selected ? 1.0f : 0.0f);

        // Brief scale pulse on selection change
        if (selected)
            scaleSpring_.setTarget(1.0f, 1.15f);

        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        selectionSpring_.position = selected ? 1.0f : 0.0f;
        repaint();
    }

    if (notify && selected && onSelected)
        onSelected();
}

void MultiScoperRadioButton::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void MultiScoperRadioButton::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void MultiScoperRadioButton::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

int MultiScoperRadioButton::getPreferredWidth() const
{
    int const radioWidth = RADIO_SIZE;

    if (label_.isNotEmpty())
    {
        auto font = ComponentLayout::defaultFont();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return radioWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return radioWidth;
}

int MultiScoperRadioButton::getPreferredHeight() const
{
    return std::max(RADIO_SIZE, static_cast<int>(ComponentLayout::defaultFont().getHeight()));
}

// paint, paintCircle, paintDot, paintFocusRing are in MultiScoperRadioButtonPainting.cpp

void MultiScoperRadioButton::resized()
{
    // No child components
}

// mouseDown, mouseUp, mouseEnter, mouseExit, keyPressed, focusGained, focusLost,
// timerCallback, updateAnimations, createAccessibilityHandler are in MultiScoperRadioButtonInteraction.cpp

//==============================================================================
// MultiScoperRadioGroup Implementation
//==============================================================================

MultiScoperRadioGroup::MultiScoperRadioGroup(IThemeService& themeService) : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
}

MultiScoperRadioGroup::MultiScoperRadioGroup(IThemeService& themeService, Orientation orientation)
    : MultiScoperRadioGroup(themeService)
{
    orientation_ = orientation;
}

MultiScoperRadioGroup::~MultiScoperRadioGroup() {}

void MultiScoperRadioGroup::addOption(const juce::String& label)
{
    auto button = std::make_unique<MultiScoperRadioButton>(getThemeService(), label);
    button->parentGroup_ = this;

    int const index = static_cast<int>(buttons_.size());
    button->onSelected = [this, index]() { handleButtonSelected(index); };

    addAndMakeVisible(*button);
    buttons_.push_back(std::move(button));

    // Select first option by default
    if (buttons_.size() == 1)
        setSelectedIndex(0, false);

    resized();
}

void MultiScoperRadioGroup::addOptions(const std::initializer_list<juce::String>& labels)
{
    for (const auto& label : labels)
        addOption(label);
}

void MultiScoperRadioGroup::clearOptions()
{
    buttons_.clear();
    selectedIndex_ = -1;
    resized();
}

void MultiScoperRadioGroup::setSelectedIndex(int index, bool notify)
{
    if (index < 0 || static_cast<size_t>(index) >= buttons_.size())
        return;

    if (selectedIndex_ == index)
        return;

    // Deselect previous
    if (selectedIndex_ >= 0 && static_cast<size_t>(selectedIndex_) < buttons_.size())
        buttons_[static_cast<size_t>(selectedIndex_)]->setSelected(false, false);

    selectedIndex_ = index;

    // Select new
    buttons_[static_cast<size_t>(selectedIndex_)]->setSelected(true, false);

    if (notify)
    {
        if (onSelectionChanged)
            onSelectionChanged(selectedIndex_);

        if (onSelectionChangedLabel)
            onSelectionChangedLabel(getSelectedLabel());
    }
}

juce::String MultiScoperRadioGroup::getSelectedLabel() const
{
    if (selectedIndex_ >= 0 && static_cast<size_t>(selectedIndex_) < buttons_.size())
        return buttons_[static_cast<size_t>(selectedIndex_)]->getLabel();

    return {};
}

void MultiScoperRadioGroup::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        resized();
    }
}

void MultiScoperRadioGroup::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        resized();
    }
}

void MultiScoperRadioGroup::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        for (auto& button : buttons_)
            button->setEnabled(enabled);
    }
}

int MultiScoperRadioGroup::getPreferredWidth() const
{
    if (buttons_.empty())
        return 0;

    if (orientation_ == Orientation::Horizontal)
    {
        int totalWidth = 0;
        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            totalWidth += buttons_[i]->getPreferredWidth();
            if (i > 0)
                totalWidth += spacing_;
        }
        return totalWidth;
    }

    int maxWidth = 0;
    for (const auto& button : buttons_)
        maxWidth = std::max(maxWidth, button->getPreferredWidth());
    return maxWidth;
}

int MultiScoperRadioGroup::getPreferredHeight() const
{
    if (buttons_.empty())
        return 0;

    if (orientation_ == Orientation::Vertical)
    {
        int totalHeight = 0;
        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            totalHeight += buttons_[i]->getPreferredHeight();
            if (i > 0)
                totalHeight += spacing_;
        }
        return totalHeight;
    }

    int maxHeight = 0;
    for (const auto& button : buttons_)
        maxHeight = std::max(maxHeight, button->getPreferredHeight());
    return maxHeight;
}

void MultiScoperRadioGroup::resized() { layoutButtons(); }

void MultiScoperRadioGroup::layoutButtons()
{
    if (buttons_.empty())
        return;

    auto bounds = getLocalBounds();

    if (orientation_ == Orientation::Horizontal)
    {
        int x = 0;
        for (auto& button : buttons_)
        {
            int const width = button->getPreferredWidth();
            button->setBounds(x, 0, width, bounds.getHeight());
            x += width + spacing_;
        }
    }
    else
    {
        int y = 0;
        for (auto& button : buttons_)
        {
            int const height = button->getPreferredHeight();
            button->setBounds(0, y, bounds.getWidth(), height);
            y += height + spacing_;
        }
    }
}

bool MultiScoperRadioGroup::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_ || buttons_.empty())
        return false;

    int newIndex = selectedIndex_;

    if (orientation_ == Orientation::Vertical)
    {
        if (key == juce::KeyPress::upKey)
            newIndex = std::max(0, selectedIndex_ - 1);
        else if (key == juce::KeyPress::downKey)
            newIndex = std::min(static_cast<int>(buttons_.size()) - 1, selectedIndex_ + 1);
    }
    else
    {
        if (key == juce::KeyPress::leftKey)
            newIndex = std::max(0, selectedIndex_ - 1);
        else if (key == juce::KeyPress::rightKey)
            newIndex = std::min(static_cast<int>(buttons_.size()) - 1, selectedIndex_ + 1);
    }

    if (newIndex != selectedIndex_)
    {
        setSelectedIndex(newIndex);
        buttons_[static_cast<size_t>(newIndex)]->grabKeyboardFocus();
        return true;
    }

    return false;
}

void MultiScoperRadioGroup::handleButtonSelected(int index) { setSelectedIndex(index); }

void MultiScoperRadioGroup::updateButtonStates()
{
    for (size_t i = 0; i < buttons_.size(); ++i)
        buttons_[i]->setSelected(std::cmp_equal(i, selectedIndex_), false);
}

std::unique_ptr<juce::AccessibilityHandler> MultiScoperRadioGroup::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

} // namespace multiscoper
