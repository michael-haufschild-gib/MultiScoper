/*
    Oscil - Radio Button and Radio Group Implementation
*/

#include "ui/components/OscilRadioButton.h"

namespace oscil
{

//==============================================================================
// OscilRadioButton Implementation
//==============================================================================

OscilRadioButton::OscilRadioButton()
    : selectionSpring_(SpringPresets::bouncy())
    , hoverSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    selectionSpring_.position = 0.0f;
    selectionSpring_.target = 0.0f;
    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
}

OscilRadioButton::OscilRadioButton(const juce::String& label)
    : OscilRadioButton()
{
    label_ = label;
}

OscilRadioButton::~OscilRadioButton()
{
    ThemeManager::getInstance().removeListener(this);
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
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        int labelWidth = font.getStringWidth(label_);
        return radioWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return radioWidth;
}

int OscilRadioButton::getPreferredHeight() const
{
    return std::max(RADIO_SIZE, static_cast<int>(juce::Font(juce::FontOptions().withHeight(13.0f)).getHeight()));
}

void OscilRadioButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Calculate circle bounds
    juce::Rectangle<float> circleBounds;

    if (label_.isEmpty())
    {
        circleBounds = bounds.toFloat().withSizeKeepingCentre(RADIO_SIZE, RADIO_SIZE);
    }
    else if (labelOnRight_)
    {
        circleBounds = juce::Rectangle<float>(
            0, (bounds.getHeight() - RADIO_SIZE) / 2.0f,
            RADIO_SIZE, RADIO_SIZE);

        // Draw label
        auto labelBounds = bounds.toFloat()
            .withLeft(RADIO_SIZE + ComponentLayout::SPACING_SM);

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        int labelWidth = font.getStringWidth(label_);

        // Draw label on left
        auto labelBounds = juce::Rectangle<float>(
            0, 0, static_cast<float>(labelWidth), static_cast<float>(bounds.getHeight()));

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        circleBounds = juce::Rectangle<float>(
            labelWidth + ComponentLayout::SPACING_SM,
            (bounds.getHeight() - RADIO_SIZE) / 2.0f,
            RADIO_SIZE, RADIO_SIZE);
    }

    paintCircle(g, circleBounds);

    if (selected_ || selectionSpring_.position > 0.01f)
        paintDot(g, circleBounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, circleBounds);
}

void OscilRadioButton::paintCircle(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverSpring_.position;

    // Background
    auto bgColour = theme_.backgroundSecondary;
    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.1f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillEllipse(bounds);

    // Border
    auto borderColour = selected_ ? theme_.controlActive : theme_.controlBorder;
    g.setColour(borderColour.withAlpha(opacity));
    g.drawEllipse(bounds.reduced(0.5f), 1.0f);
}

void OscilRadioButton::paintDot(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float progress = selectionSpring_.position;

    if (progress < 0.01f) return;

    // Animated dot with scale
    float dotScale = progress;
    float dotRadius = (DOT_SIZE / 2.0f) * dotScale;

    g.setColour(theme_.controlActive.withAlpha(opacity * progress));
    g.fillEllipse(
        bounds.getCentreX() - dotRadius,
        bounds.getCentreY() - dotRadius,
        dotRadius * 2.0f,
        dotRadius * 2.0f
    );
}

void OscilRadioButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawEllipse(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

void OscilRadioButton::resized()
{
    // No child components
}

void OscilRadioButton::mouseDown(const juce::MouseEvent&)
{
    if (enabled_)
        isPressed_ = true;
}

void OscilRadioButton::mouseUp(const juce::MouseEvent& e)
{
    if (isPressed_ && enabled_ && contains(e.getPosition()))
    {
        if (!selected_)
        {
            if (parentGroup_)
            {
                // Let the group handle selection
                int myIndex = -1;
                for (int i = 0; i < parentGroup_->getNumOptions(); ++i)
                {
                    if (parentGroup_->getButton(i) == this)
                    {
                        myIndex = i;
                        break;
                    }
                }
                if (myIndex >= 0)
                    parentGroup_->setSelectedIndex(myIndex);
            }
            else
            {
                setSelected(true);
            }
        }
    }
    isPressed_ = false;
}

void OscilRadioButton::mouseEnter(const juce::MouseEvent&)
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

void OscilRadioButton::mouseExit(const juce::MouseEvent&)
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

bool OscilRadioButton::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        if (!selected_)
        {
            if (parentGroup_)
            {
                int myIndex = -1;
                for (int i = 0; i < parentGroup_->getNumOptions(); ++i)
                {
                    if (parentGroup_->getButton(i) == this)
                    {
                        myIndex = i;
                        break;
                    }
                }
                if (myIndex >= 0)
                    parentGroup_->setSelectedIndex(myIndex);
            }
            else
            {
                setSelected(true);
            }
        }
        return true;
    }
    return false;
}

void OscilRadioButton::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilRadioButton::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilRadioButton::timerCallback()
{
    updateAnimations();

    if (selectionSpring_.isSettled() && hoverSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilRadioButton::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    selectionSpring_.update(dt);
    hoverSpring_.update(dt);
}

void OscilRadioButton::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilRadioButton::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::radioButton,
        juce::AccessibilityActions()
            .addAction(juce::AccessibilityActionType::press,
                [this] { if (enabled_ && !selected_) setSelected(true); })
    );
}

//==============================================================================
// OscilRadioGroup Implementation
//==============================================================================

OscilRadioGroup::OscilRadioGroup()
{
    setWantsKeyboardFocus(true);
    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);
}

OscilRadioGroup::OscilRadioGroup(Orientation orientation)
    : OscilRadioGroup()
{
    orientation_ = orientation;
}

OscilRadioGroup::~OscilRadioGroup()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscilRadioGroup::addOption(const juce::String& label)
{
    auto button = std::make_unique<OscilRadioButton>(label);
    button->parentGroup_ = this;

    int index = static_cast<int>(buttons_.size());
    button->onSelected = [this, index]() {
        handleButtonSelected(index);
    };

    addAndMakeVisible(*button);
    buttons_.add(std::move(button));

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
    if (index < 0 || index >= buttons_.size())
        return;

    if (selectedIndex_ == index)
        return;

    // Deselect previous
    if (selectedIndex_ >= 0 && selectedIndex_ < buttons_.size())
        buttons_[selectedIndex_]->setSelected(false, false);

    selectedIndex_ = index;

    // Select new
    buttons_[selectedIndex_]->setSelected(true, false);

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
    if (selectedIndex_ >= 0 && selectedIndex_ < buttons_.size())
        return buttons_[selectedIndex_]->getLabel();

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
    if (buttons_.isEmpty())
        return 0;

    if (orientation_ == Orientation::Horizontal)
    {
        int totalWidth = 0;
        for (int i = 0; i < buttons_.size(); ++i)
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
    if (buttons_.isEmpty())
        return 0;

    if (orientation_ == Orientation::Vertical)
    {
        int totalHeight = 0;
        for (int i = 0; i < buttons_.size(); ++i)
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
    if (buttons_.isEmpty())
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
    if (!enabled_ || buttons_.isEmpty())
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
        buttons_[newIndex]->grabKeyboardFocus();
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
    for (int i = 0; i < buttons_.size(); ++i)
        buttons_[i]->setSelected(i == selectedIndex_, false);
}

void OscilRadioGroup::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilRadioGroup::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::group
    );
}

} // namespace oscil
