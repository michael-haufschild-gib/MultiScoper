/*
    Oscil - Tabs Component Implementation
*/

#include "ui/components/OscilTabs.h"

namespace oscil
{

OscilTabs::OscilTabs()
    : indicatorXSpring_(SpringPresets::snappy())
    , indicatorWidthSpring_(SpringPresets::snappy())
    , hoverSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    indicatorXSpring_.position = 0.0f;
    indicatorWidthSpring_.position = 0.0f;
    hoverSpring_.position = 0.0f;
}

OscilTabs::OscilTabs(Orientation orientation)
    : OscilTabs()
{
    orientation_ = orientation;
}

OscilTabs::~OscilTabs()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilTabs::addTab(const juce::String& label, const juce::String& id)
{
    TabItem tab;
    tab.id = id.isEmpty() ? label : id;
    tab.label = label;
    tabs_.push_back(tab);

    if (tabs_.size() == 1)
    {
        selectedIndex_ = 0;
        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }

    resized();
}

void OscilTabs::addTab(const TabItem& tab)
{
    tabs_.push_back(tab);

    if (tabs_.size() == 1)
    {
        selectedIndex_ = 0;
        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }

    resized();
}

void OscilTabs::addTabs(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
        addTab(label);
}

void OscilTabs::addTabs(const std::vector<TabItem>& tabs)
{
    for (const auto& tab : tabs)
        addTab(tab);
}

void OscilTabs::clearTabs()
{
    tabs_.clear();
    selectedIndex_ = 0;
    hoveredIndex_ = -1;
    resized();
}

void OscilTabs::setTabBadge(int index, int count)
{
    if (index >= 0 && index < static_cast<int>(tabs_.size()))
    {
        tabs_[index].badgeCount = count;
        repaint();
    }
}

void OscilTabs::setTabEnabled(int index, bool enabled)
{
    if (index >= 0 && index < static_cast<int>(tabs_.size()))
    {
        tabs_[index].enabled = enabled;
        repaint();
    }
}

void OscilTabs::setSelectedIndex(int index, bool notify)
{
    if (index < 0 || index >= static_cast<int>(tabs_.size()))
        return;

    if (!tabs_[index].enabled)
        return;

    if (selectedIndex_ == index)
        return;

    selectedIndex_ = index;

    // Update indicator target position
    auto bounds = getTabBounds(index);
    targetIndicatorX_ = static_cast<float>(bounds.getX());
    targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        indicatorXSpring_.setTarget(targetIndicatorX_);
        indicatorWidthSpring_.setTarget(targetIndicatorWidth_);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
        repaint();
    }

    if (notify)
    {
        if (onTabChanged)
            onTabChanged(selectedIndex_);

        if (onTabChangedId)
            onTabChangedId(getSelectedId());
    }
}

juce::String OscilTabs::getSelectedId() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(tabs_.size()))
        return tabs_[selectedIndex_].id;
    return {};
}

void OscilTabs::setSelectedById(const juce::String& id, bool notify)
{
    for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
    {
        if (tabs_[i].id == id)
        {
            setSelectedIndex(i, notify);
            return;
        }
    }
}

void OscilTabs::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        resized();
    }
}

void OscilTabs::setVariant(Variant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void OscilTabs::setTabWidth(int width)
{
    if (tabWidth_ != width)
    {
        tabWidth_ = width;
        resized();
    }
}

void OscilTabs::setTabHeight(int height)
{
    if (tabHeight_ != height)
    {
        tabHeight_ = height;
        resized();
    }
}

void OscilTabs::setStretchTabs(bool stretch)
{
    if (stretchTabs_ != stretch)
    {
        stretchTabs_ = stretch;
        resized();
    }
}

int OscilTabs::getPreferredWidth() const
{
    if (tabs_.empty())
        return 100;

    if (orientation_ == Orientation::Horizontal)
    {
        int totalWidth = 0;
        for (const auto& tab : tabs_)
        {
            if (tabWidth_ > 0)
            {
                totalWidth += tabWidth_;
            }
            else
            {
                auto font = juce::Font(13.0f);
                int labelWidth = font.getStringWidth(tab.label);
                int iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
                int badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
                totalWidth += labelWidth + iconWidth + badgeWidth + TAB_PADDING_H * 2;
            }
        }
        return totalWidth;
    }
    else
    {
        int maxWidth = 0;
        for (const auto& tab : tabs_)
        {
            auto font = juce::Font(13.0f);
            int labelWidth = font.getStringWidth(tab.label);
            int iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
            int badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
            maxWidth = std::max(maxWidth, labelWidth + iconWidth + badgeWidth + TAB_PADDING_H * 2);
        }
        return tabWidth_ > 0 ? tabWidth_ : maxWidth;
    }
}

int OscilTabs::getPreferredHeight() const
{
    if (tabs_.empty())
        return DEFAULT_TAB_HEIGHT;

    if (orientation_ == Orientation::Vertical)
    {
        int height = tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;
        return height * static_cast<int>(tabs_.size());
    }
    else
    {
        return tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;
    }
}

void OscilTabs::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background line for default variant
    if (variant_ == Variant::Default && orientation_ == Orientation::Horizontal)
    {
        g.setColour(theme_.controlBorder.withAlpha(0.3f));
        g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }

    // Draw tabs
    for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
    {
        auto tabBounds = getTabBounds(i);
        paintTab(g, i, tabBounds);
    }

    // Draw indicator
    paintIndicator(g);

    // Focus ring
    if (hasFocus_)
    {
        auto selectedBounds = getTabBounds(selectedIndex_).toFloat();
        g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(
            selectedBounds.reduced(-ComponentLayout::FOCUS_RING_OFFSET),
            ComponentLayout::RADIUS_SM,
            ComponentLayout::FOCUS_RING_WIDTH
        );
    }
}

void OscilTabs::paintTab(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    const auto& tab = tabs_[index];
    bool isSelected = (index == selectedIndex_);
    bool isHovered = (index == hoveredIndex_);
    float opacity = tab.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Hover background for pills variant
    if (variant_ == Variant::Pills && isHovered && !isSelected && tab.enabled)
    {
        g.setColour(theme_.backgroundSecondary.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.reduced(2).toFloat(), ComponentLayout::RADIUS_SM);
    }

    // Content bounds
    auto contentBounds = bounds.reduced(TAB_PADDING_H, 0);
    int contentWidth = 0;

    // Calculate total content width for centering
    auto font = juce::Font(13.0f);
    int labelWidth = font.getStringWidth(tab.label);
    contentWidth += labelWidth;

    if (tab.icon.isValid())
        contentWidth += ICON_SIZE + 8;

    if (tab.badgeCount > 0)
        contentWidth += BADGE_SIZE + 4;

    int startX = contentBounds.getX() + (contentBounds.getWidth() - contentWidth) / 2;
    int centerY = bounds.getCentreY();

    // Icon
    if (tab.icon.isValid())
    {
        g.setOpacity(opacity);
        g.drawImage(tab.icon,
            juce::Rectangle<float>(static_cast<float>(startX),
                                   static_cast<float>(centerY - ICON_SIZE / 2),
                                   ICON_SIZE, ICON_SIZE),
            juce::RectanglePlacement::centred);
        startX += ICON_SIZE + 8;
    }

    // Label
    auto textColour = isSelected ? theme_.controlActive
                    : isHovered && tab.enabled ? theme_.textPrimary
                    : theme_.textSecondary;

    g.setColour(textColour.withAlpha(opacity));
    g.setFont(font);

    auto labelBounds = juce::Rectangle<int>(startX, 0, labelWidth, bounds.getHeight());
    g.drawText(tab.label, labelBounds, juce::Justification::centred);
    startX += labelWidth;

    // Badge
    if (tab.badgeCount > 0)
    {
        auto badgeBounds = juce::Rectangle<int>(
            startX + 4, centerY - BADGE_SIZE / 2, BADGE_SIZE, BADGE_SIZE);
        paintBadge(g, badgeBounds, tab.badgeCount);
    }
}

void OscilTabs::paintIndicator(juce::Graphics& g)
{
    if (tabs_.empty())
        return;

    auto indicatorBounds = getIndicatorBounds();

    switch (variant_)
    {
        case Variant::Default:
            // Underline indicator
            g.setColour(theme_.controlActive);
            if (orientation_ == Orientation::Horizontal)
            {
                g.fillRoundedRectangle(
                    indicatorBounds.getX(),
                    static_cast<float>(getHeight() - INDICATOR_HEIGHT),
                    indicatorBounds.getWidth(),
                    INDICATOR_HEIGHT,
                    INDICATOR_HEIGHT / 2.0f
                );
            }
            else
            {
                g.fillRoundedRectangle(
                    0,
                    indicatorBounds.getY(),
                    INDICATOR_HEIGHT,
                    indicatorBounds.getHeight(),
                    INDICATOR_HEIGHT / 2.0f
                );
            }
            break;

        case Variant::Pills:
            // Filled background
            g.setColour(theme_.controlActive.withAlpha(0.15f));
            g.fillRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM);
            g.setColour(theme_.controlActive);
            g.drawRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM, 1.0f);
            break;

        case Variant::Bordered:
            // Border around tab
            g.setColour(theme_.controlBorder);
            g.drawRoundedRectangle(indicatorBounds.reduced(1), ComponentLayout::RADIUS_SM, 1.0f);
            break;
    }
}

void OscilTabs::paintBadge(juce::Graphics& g, juce::Rectangle<int> bounds, int count)
{
    g.setColour(theme_.statusError);
    g.fillEllipse(bounds.toFloat());

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(10.0f).boldened());

    juce::String text = count > 99 ? "99+" : juce::String(count);
    g.drawText(text, bounds, juce::Justification::centred);
}

void OscilTabs::resized()
{
    // Update indicator position after resize
    if (!tabs_.empty() && selectedIndex_ >= 0)
    {
        auto bounds = getTabBounds(selectedIndex_);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());

        if (!isTimerRunning())
        {
            indicatorXSpring_.position = targetIndicatorX_;
            indicatorWidthSpring_.position = targetIndicatorWidth_;
        }
    }
}

int OscilTabs::getTabAtPosition(juce::Point<int> pos) const
{
    for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
    {
        if (getTabBounds(i).contains(pos))
            return i;
    }
    return -1;
}

juce::Rectangle<int> OscilTabs::getTabBounds(int index) const
{
    if (index < 0 || index >= static_cast<int>(tabs_.size()))
        return {};

    auto bounds = getLocalBounds();

    if (orientation_ == Orientation::Horizontal)
    {
        int height = tabHeight_ > 0 ? tabHeight_ : bounds.getHeight();

        if (stretchTabs_)
        {
            int tabWidth = bounds.getWidth() / static_cast<int>(tabs_.size());
            return juce::Rectangle<int>(index * tabWidth, 0, tabWidth, height);
        }
        else if (tabWidth_ > 0)
        {
            return juce::Rectangle<int>(index * tabWidth_, 0, tabWidth_, height);
        }
        else
        {
            // Calculate position based on preceding tab widths
            int x = 0;
            auto font = juce::Font(13.0f);

            for (int i = 0; i < index; ++i)
            {
                const auto& tab = tabs_[i];
                int labelWidth = font.getStringWidth(tab.label);
                int iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
                int badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
                x += labelWidth + iconWidth + badgeWidth + TAB_PADDING_H * 2;
            }

            const auto& tab = tabs_[index];
            int labelWidth = font.getStringWidth(tab.label);
            int iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
            int badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
            int width = labelWidth + iconWidth + badgeWidth + TAB_PADDING_H * 2;

            return juce::Rectangle<int>(x, 0, width, height);
        }
    }
    else
    {
        // Vertical
        int width = tabWidth_ > 0 ? tabWidth_ : bounds.getWidth();
        int height = tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;

        return juce::Rectangle<int>(0, index * height, width, height);
    }
}

juce::Rectangle<float> OscilTabs::getIndicatorBounds() const
{
    if (orientation_ == Orientation::Horizontal)
    {
        int height = tabHeight_ > 0 ? tabHeight_ : getHeight();
        return juce::Rectangle<float>(
            indicatorXSpring_.position, 0,
            indicatorWidthSpring_.position, static_cast<float>(height)
        );
    }
    else
    {
        int width = tabWidth_ > 0 ? tabWidth_ : getWidth();
        return juce::Rectangle<float>(
            0, indicatorXSpring_.position,
            static_cast<float>(width), indicatorWidthSpring_.position
        );
    }
}

void OscilTabs::mouseDown(const juce::MouseEvent& e)
{
    int index = getTabAtPosition(e.getPosition());
    if (index >= 0 && tabs_[index].enabled)
        setSelectedIndex(index);
}

void OscilTabs::mouseMove(const juce::MouseEvent& e)
{
    int newHovered = getTabAtPosition(e.getPosition());
    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();
    }
}

void OscilTabs::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();
    }
}

bool OscilTabs::keyPressed(const juce::KeyPress& key)
{
    if (tabs_.empty())
        return false;

    int newIndex = selectedIndex_;

    if (orientation_ == Orientation::Horizontal)
    {
        if (key == juce::KeyPress::leftKey)
            newIndex = std::max(0, selectedIndex_ - 1);
        else if (key == juce::KeyPress::rightKey)
            newIndex = std::min(static_cast<int>(tabs_.size()) - 1, selectedIndex_ + 1);
    }
    else
    {
        if (key == juce::KeyPress::upKey)
            newIndex = std::max(0, selectedIndex_ - 1);
        else if (key == juce::KeyPress::downKey)
            newIndex = std::min(static_cast<int>(tabs_.size()) - 1, selectedIndex_ + 1);
    }

    // Skip disabled tabs
    int direction = (newIndex > selectedIndex_) ? 1 : -1;
    while (newIndex >= 0 && newIndex < static_cast<int>(tabs_.size()) &&
           !tabs_[newIndex].enabled)
    {
        newIndex += direction;
    }

    if (newIndex >= 0 && newIndex < static_cast<int>(tabs_.size()) &&
        newIndex != selectedIndex_)
    {
        setSelectedIndex(newIndex);
        return true;
    }

    return false;
}

void OscilTabs::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilTabs::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilTabs::timerCallback()
{
    updateAnimations();

    if (indicatorXSpring_.isSettled() && indicatorWidthSpring_.isSettled() &&
        hoverSpring_.isSettled())
    {
        stopTimer();
    }

    repaint();
}

void OscilTabs::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    indicatorXSpring_.update(dt);
    indicatorWidthSpring_.update(dt);
    hoverSpring_.update(dt);
}

void OscilTabs::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilTabs::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::group
    );
}

} // namespace oscil
