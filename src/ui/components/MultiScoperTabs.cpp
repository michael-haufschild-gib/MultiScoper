/*
    MultiScoper - Tabs Component Implementation
    (Painting, layout cache, and indicator bounds are in MultiScoperTabsPainting.cpp)
*/

#include "ui/components/MultiScoperTabs.h"

#include "ui/theme/Typography.h"

#include <utility>

namespace multiscoper
{

MultiScoperTabs::MultiScoperTabs(IThemeService& themeService)
    : ThemedComponent(themeService)
    , indicatorXSpring_(SpringPresets::springIndicator())
    , indicatorWidthSpring_(SpringPresets::springIndicator())
    , hoverSpring_(SpringPresets::fast())
{
    setWantsKeyboardFocus(true);

    indicatorXSpring_.position = 0.0f;
    indicatorWidthSpring_.position = 0.0f;
    hoverSpring_.position = 0.0f;
}

MultiScoperTabs::MultiScoperTabs(IThemeService& themeService, Orientation orientation) : MultiScoperTabs(themeService)
{
    orientation_ = orientation;
}

MultiScoperTabs::MultiScoperTabs(IThemeService& themeService, Orientation orientation, const juce::String& testId)
    : MultiScoperTabs(themeService)
{
    orientation_ = orientation;
    setTestId(testId);
}

void MultiScoperTabs::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperTabs::~MultiScoperTabs() { stopTimer(); }

void MultiScoperTabs::addTab(const juce::String& label, const juce::String& id)
{
    TabItem tab;
    tab.id = id.isEmpty() ? label : id;
    tab.label = label;
    addTab(tab);
}

void MultiScoperTabs::addTab(const TabItem& tab)
{
    tabs_.push_back(tab);
    tabAccentColours_.emplace_back();

    if (tabs_.size() == 1)
    {
        selectedIndex_ = 0;
        updateLayoutCache();
        updateIndicatorTarget(getTabBounds(0), true);
    }
    else
    {
        updateLayoutCache();
    }

    resized();
}

void MultiScoperTabs::addTabs(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
    {
        TabItem tab;
        tab.id = label;
        tab.label = label;
        tabs_.push_back(tab);
        tabAccentColours_.emplace_back();
    }

    updateLayoutCache();

    if (!tabs_.empty() && selectedIndex_ == 0)
        updateIndicatorTarget(getTabBounds(0), true);

    resized();
}

void MultiScoperTabs::addTabs(const std::vector<TabItem>& tabs)
{
    for (const auto& tab : tabs)
    {
        tabs_.push_back(tab);
        tabAccentColours_.emplace_back();
    }

    updateLayoutCache();

    if (!tabs_.empty() && selectedIndex_ == 0)
        updateIndicatorTarget(getTabBounds(0), true);

    resized();
}

void MultiScoperTabs::clearTabs()
{
    stopTimer();
    tabs_.clear();
    tabAccentColours_.clear();
    selectedIndex_ = 0;
    hoveredIndex_ = -1;
    targetIndicatorX_ = 0;
    targetIndicatorWidth_ = 0;
    indicatorXSpring_.position = 0;
    indicatorWidthSpring_.position = 0;
    updateLayoutCache();
    resized();
}

void MultiScoperTabs::setTabBadge(int index, int count)
{
    if (index >= 0 && std::cmp_less(index, tabs_.size()))
    {
        tabs_[static_cast<size_t>(index)].badgeCount = count;
        repaint();
    }
}

void MultiScoperTabs::setTabEnabled(int index, bool enabled)
{
    if (index >= 0 && std::cmp_less(index, tabs_.size()))
    {
        tabs_[static_cast<size_t>(index)].enabled = enabled;
        repaint();
    }
}

void MultiScoperTabs::setTabAccentColour(int index, juce::Colour colour)
{
    if (index >= 0 && std::cmp_less(index, tabAccentColours_.size()))
    {
        tabAccentColours_[static_cast<size_t>(index)] = colour;
        repaint();
    }
}

void MultiScoperTabs::clearTabAccentColour(int index)
{
    if (index >= 0 && std::cmp_less(index, tabAccentColours_.size()))
    {
        tabAccentColours_[static_cast<size_t>(index)].reset();
        repaint();
    }
}

juce::Colour MultiScoperTabs::getEffectiveTabAccent(int index) const
{
    jassert(tabs_.size() == tabAccentColours_.size());
    if (index >= 0 && std::cmp_less(index, tabAccentColours_.size()))
    {
        if (auto const& c = tabAccentColours_[static_cast<size_t>(index)])
            return *c;
    }
    return getSurface().accent;
}

void MultiScoperTabs::setSelectedIndex(int index, bool notify)
{
    if (index < 0 || std::cmp_greater_equal(index, tabs_.size()))
        return;

    if (!tabs_[static_cast<size_t>(index)].enabled)
        return;

    if (selectedIndex_ == index)
        return;

    selectedIndex_ = index;

    updateIndicatorTarget(getTabBounds(index), false);

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

juce::String MultiScoperTabs::getSelectedId() const
{
    if (selectedIndex_ >= 0 && std::cmp_less(selectedIndex_, tabs_.size()))
        return tabs_[static_cast<size_t>(selectedIndex_)].id;
    return {};
}

void MultiScoperTabs::setSelectedById(const juce::String& id, bool notify)
{
    for (size_t i = 0; i < tabs_.size(); ++i)
    {
        if (tabs_[i].id == id)
        {
            setSelectedIndex(static_cast<int>(i), notify);
            return;
        }
    }
}

void MultiScoperTabs::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        updateLayoutCache();
        resized();
    }
}

void MultiScoperTabs::setVariant(Variant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void MultiScoperTabs::setTabWidth(int width)
{
    if (tabWidth_ != width)
    {
        tabWidth_ = width;
        updateLayoutCache();
        resized();
    }
}

void MultiScoperTabs::setTabHeight(int height)
{
    if (tabHeight_ != height)
    {
        tabHeight_ = height;
        updateLayoutCache();
        resized();
    }
}

void MultiScoperTabs::setStretchTabs(bool stretch)
{
    if (stretchTabs_ != stretch)
    {
        stretchTabs_ = stretch;
        updateLayoutCache();
        resized();
    }
}

int MultiScoperTabs::getPreferredWidth() const
{
    if (tabs_.empty())
        return 100;

    auto computeTabContentWidth = [](const TabItem& tab) -> int {
        auto font = Typography::body().withHeight(TAB_FONT_SIZE);
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, tab.label, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        int const iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
        int const badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
        return labelWidth + iconWidth + badgeWidth + (TAB_PADDING_H * 2);
    };

    if (orientation_ == Orientation::Horizontal)
    {
        int totalWidth = 0;
        for (const auto& tab : tabs_)
            totalWidth += tabWidth_ > 0 ? tabWidth_ : computeTabContentWidth(tab);
        return totalWidth;
    }

    if (tabWidth_ > 0)
        return tabWidth_;

    int maxWidth = 0;
    for (const auto& tab : tabs_)
        maxWidth = std::max(maxWidth, computeTabContentWidth(tab));
    return maxWidth;
}

int MultiScoperTabs::getPreferredHeight() const
{
    if (tabs_.empty())
        return DEFAULT_TAB_HEIGHT;

    if (orientation_ == Orientation::Vertical)
    {
        int const height = tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;
        return height * static_cast<int>(tabs_.size());
    }

    return tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;
}

void MultiScoperTabs::resized()
{
    updateLayoutCache();

    if (!tabs_.empty() && selectedIndex_ >= 0)
    {
        updateIndicatorTarget(getTabBounds(selectedIndex_), !isTimerRunning());
    }
}

int MultiScoperTabs::getTabAtPosition(juce::Point<int> pos) const
{
    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        if (getTabBounds(i).contains(pos))
            return i;
    }
    return -1;
}

void MultiScoperTabs::mouseDown(const juce::MouseEvent& e)
{
    int const index = getTabAtPosition(e.getPosition());
    if (index >= 0 && tabs_[static_cast<size_t>(index)].enabled)
        setSelectedIndex(index);
}

void MultiScoperTabs::mouseMove(const juce::MouseEvent& e)
{
    int const newHovered = getTabAtPosition(e.getPosition());
    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();
    }
}

void MultiScoperTabs::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();
    }
}

bool MultiScoperTabs::keyPressed(const juce::KeyPress& key)
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

    int const direction = (newIndex > selectedIndex_) ? 1 : -1;
    while (newIndex >= 0 && std::cmp_less(newIndex, tabs_.size()) && !tabs_[static_cast<size_t>(newIndex)].enabled)
    {
        newIndex += direction;
    }

    if (newIndex >= 0 && std::cmp_less(newIndex, tabs_.size()) && newIndex != selectedIndex_)
    {
        setSelectedIndex(newIndex);
        return true;
    }

    return false;
}

void MultiScoperTabs::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperTabs::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperTabs::timerCallback()
{
    updateAnimations();

    if (indicatorXSpring_.isSettled() && indicatorWidthSpring_.isSettled() && hoverSpring_.isSettled())
    {
        stopTimer();
    }

    repaint();
}

void MultiScoperTabs::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    indicatorXSpring_.update(dt);
    indicatorWidthSpring_.update(dt);
    hoverSpring_.update(dt);
}

void MultiScoperTabs::updateIndicatorTarget(juce::Rectangle<int> bounds, bool snap)
{
    if (orientation_ == Orientation::Vertical)
    {
        targetIndicatorX_ = static_cast<float>(bounds.getY());
        targetIndicatorWidth_ = static_cast<float>(bounds.getHeight());
    }
    else
    {
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
    }

    if (snap)
    {
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }
}

std::unique_ptr<juce::AccessibilityHandler> MultiScoperTabs::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

} // namespace multiscoper
