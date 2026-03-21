/*
    Oscil - Radio Button and Radio Group Implementation
*/

#include "ui/components/OscilRadioButton.h"

namespace oscil
{

//==============================================================================
// OscilRadioButton Implementation
//==============================================================================

OscilRadioButton::OscilRadioButton(IThemeService& themeService)
    : ThemedComponent(themeService)
    , selectionSpring_(SpringPresets::bouncy())
    , hoverSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    selectionSpring_.position = 0.0f;
    selectionSpring_.target = 0.0f;
    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
}

OscilRadioButton::OscilRadioButton(IThemeService& themeService, const juce::String& label)
    : OscilRadioButton(themeService)
{
    label_ = label;
}

OscilRadioButton::OscilRadioButton(IThemeService& themeService, const juce::String& label, const juce::String& testId)
    : OscilRadioButton(themeService)
{
    label_ = label;
    setTestId(testId);
}

void OscilRadioButton::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilRadioButton::~OscilRadioButton()
{
    stopTimer();
}

void OscilRadioButton::setSelected(bool selected, bool notify)
{
    if (selected_ == selected) return;

    selected_ = selected;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        selectionSpring_.setTarget(selected ? 1.0f : 0.0f);
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

void OscilRadioButton::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void OscilRadioButton::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void OscilRadioButton::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

int OscilRadioButton::getPreferredWidth() const
{
    int radioWidth = RADIO_SIZE;

    if (label_.isNotEmpty())
    {
        auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return radioWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return radioWidth;
}

int OscilRadioButton::getPreferredHeight() const
{
    return std::max(RADIO_SIZE, static_cast<int>(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)).getHeight()));
}

// paint, paintCircle, paintDot, paintFocusRing are in OscilRadioButtonPainting.cpp

void OscilRadioButton::resized()
{
    // No child components
}

// mouseDown, mouseUp, mouseEnter, mouseExit, keyPressed, focusGained, focusLost,
// timerCallback, updateAnimations, createAccessibilityHandler are in OscilRadioButtonInteraction.cpp

//==============================================================================
// OscilRadioGroup Implementation
//==============================================================================

OscilRadioGroup::OscilRadioGroup(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
}

OscilRadioGroup::OscilRadioGroup(IThemeService& themeService, Orientation orientation)
    : OscilRadioGroup(themeService)
{
    orientation_ = orientation;
}

OscilRadioGroup::~OscilRadioGroup()
{
}

void OscilRadioGroup::addOption(const juce::String& label)
{
    auto button = std::make_unique<OscilRadioButton>(getThemeService(), label);
    button->parentGroup_ = this;

    int index = static_cast<int>(buttons_.size());
    button->onSelected = [this, index]() {
        handleButtonSelected(index);
    };

    addAndMakeVisible(*button);
    buttons_.push_back(std::move(button));

    // Select first option by default
    if (buttons_.size() == 1)
        setSelectedIndex(0, false);

    resized();
}

void OscilRadioGroup::addOptions(const std::initializer_list<juce::String>& labels)
{
    for (const auto& label : labels)
        addOption(label);
}

void OscilRadioGroup::clearOptions()
{
    buttons_.clear();
    selectedIndex_ = -1;
    resized();
}

void OscilRadioGroup::setSelectedIndex(int index, bool notify)
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

juce::String OscilRadioGroup::getSelectedLabel() const
{
    if (selectedIndex_ >= 0 && static_cast<size_t>(selectedIndex_) < buttons_.size())
        return buttons_[static_cast<size_t>(selectedIndex_)]->getLabel();

    return {};
}

void OscilRadioGroup::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        resized();
    }
}

void OscilRadioGroup::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        resized();
    }
}

void OscilRadioGroup::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        for (auto& button : buttons_)
            button->setEnabled(enabled);
    }
}

int OscilRadioGroup::getPreferredWidth() const
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
    else
    {
        int maxWidth = 0;
        for (const auto& button : buttons_)
            maxWidth = std::max(maxWidth, button->getPreferredWidth());
        return maxWidth;
    }
}

int OscilRadioGroup::getPreferredHeight() const
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
    else
    {
        int maxHeight = 0;
        for (const auto& button : buttons_)
            maxHeight = std::max(maxHeight, button->getPreferredHeight());
        return maxHeight;
    }
}

void OscilRadioGroup::resized()
{
    layoutButtons();
}

void OscilRadioGroup::layoutButtons()
{
    if (buttons_.empty())
        return;

    auto bounds = getLocalBounds();

    if (orientation_ == Orientation::Horizontal)
    {
        int x = 0;
        for (auto& button : buttons_)
        {
            int width = button->getPreferredWidth();
            button->setBounds(x, 0, width, bounds.getHeight());
            x += width + spacing_;
        }
    }
    else
    {
        int y = 0;
        for (auto& button : buttons_)
        {
            int height = button->getPreferredHeight();
            button->setBounds(0, y, bounds.getWidth(), height);
            y += height + spacing_;
        }
    }
}

bool OscilRadioGroup::keyPressed(const juce::KeyPress& key)
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

void OscilRadioGroup::handleButtonSelected(int index)
{
    setSelectedIndex(index);
}

void OscilRadioGroup::updateButtonStates()
{
    for (size_t i = 0; i < buttons_.size(); ++i)
        buttons_[i]->setSelected(static_cast<int>(i) == selectedIndex_, false);
}


std::unique_ptr<juce::AccessibilityHandler> OscilRadioGroup::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::group
    );
}

} // namespace oscil
