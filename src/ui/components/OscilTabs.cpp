/*
    Oscil - Tabs Component Implementation
    (Painting, layout cache, and indicator bounds are in OscilTabsPainting.cpp)
*/

#include "ui/components/OscilTabs.h"

#include <utility>

namespace oscil
{

static constexpr float kTabFontSize = 13.0f;

OscilTabs::OscilTabs(IThemeService& themeService)
    : ThemedComponent(themeService)
    , indicatorXSpring_(SpringPresets::medium())
    , indicatorWidthSpring_(SpringPresets::medium())
    , hoverSpring_(SpringPresets::fast())
{
    setWantsKeyboardFocus(true);

    indicatorXSpring_.position = 0.0f;
    indicatorWidthSpring_.position = 0.0f;
    hoverSpring_.position = 0.0f;
}

OscilTabs::OscilTabs(IThemeService& themeService, Orientation orientation) : OscilTabs(themeService)
{
    orientation_ = orientation;
}

OscilTabs::OscilTabs(IThemeService& themeService, Orientation orientation, const juce::String& testId)
    : OscilTabs(themeService)
{
    orientation_ = orientation;
    setTestId(testId);
}

void OscilTabs::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilTabs::~OscilTabs() { stopTimer(); }

void OscilTabs::addTab(const juce::String& label, const juce::String& id)
{
    TabItem tab;
    tab.id = id.isEmpty() ? label : id;
    tab.label = label;
    tabs_.push_back(tab);

    if (tabs_.size() == 1)
    {
        selectedIndex_ = 0;
        updateLayoutCache();

        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }
    else
    {
        updateLayoutCache();
    }

    resized();
}

void OscilTabs::addTab(const TabItem& tab)
{
    tabs_.push_back(tab);

    if (tabs_.size() == 1)
    {
        selectedIndex_ = 0;
        updateLayoutCache();

        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }
    else
    {
        updateLayoutCache();
    }

    resized();
}

void OscilTabs::addTabs(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
    {
        TabItem tab;
        tab.id = label;
        tab.label = label;
        tabs_.push_back(tab);
    }

    updateLayoutCache();

    if (!tabs_.empty() && selectedIndex_ == 0)
    {
        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }

    resized();
}

void OscilTabs::addTabs(const std::vector<TabItem>& tabs)
{
    for (const auto& tab : tabs)
        tabs_.push_back(tab);

    updateLayoutCache();

    if (!tabs_.empty() && selectedIndex_ == 0)
    {
        auto bounds = getTabBounds(0);
        targetIndicatorX_ = static_cast<float>(bounds.getX());
        targetIndicatorWidth_ = static_cast<float>(bounds.getWidth());
        indicatorXSpring_.position = targetIndicatorX_;
        indicatorWidthSpring_.position = targetIndicatorWidth_;
    }

    resized();
}

void OscilTabs::clearTabs()
{
    tabs_.clear();
    selectedIndex_ = 0;
    hoveredIndex_ = -1;
    updateLayoutCache();
    resized();
}

void OscilTabs::setTabBadge(int index, int count)
{
    if (index >= 0 && std::cmp_less(index, tabs_.size()))
    {
        tabs_[static_cast<size_t>(index)].badgeCount = count;
        repaint();
    }
}

void OscilTabs::setTabEnabled(int index, bool enabled)
{
    if (index >= 0 && std::cmp_less(index, tabs_.size()))
    {
        tabs_[static_cast<size_t>(index)].enabled = enabled;
        repaint();
    }
}

void OscilTabs::setSelectedIndex(int index, bool notify)
{
    if (index < 0 || std::cmp_greater_equal(index, tabs_.size()))
        return;

    if (!tabs_[static_cast<size_t>(index)].enabled)
        return;

    if (selectedIndex_ == index)
        return;

    selectedIndex_ = index;

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
    if (selectedIndex_ >= 0 && std::cmp_less(selectedIndex_, tabs_.size()))
        return tabs_[static_cast<size_t>(selectedIndex_)].id;
    return {};
}

void OscilTabs::setSelectedById(const juce::String& id, bool notify)
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

void OscilTabs::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        updateLayoutCache();
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
        updateLayoutCache();
        resized();
    }
}

void OscilTabs::setTabHeight(int height)
{
    if (tabHeight_ != height)
    {
        tabHeight_ = height;
        updateLayoutCache();
        resized();
    }
}

void OscilTabs::setStretchTabs(bool stretch)
{
    if (stretchTabs_ != stretch)
    {
        stretchTabs_ = stretch;
        updateLayoutCache();
        resized();
    }
}

int OscilTabs::getPreferredWidth() const
{
    if (tabs_.empty())
        return 100;

    auto computeTabContentWidth = [](const TabItem& tab) -> int {
        auto font = juce::Font(juce::FontOptions().withHeight(kTabFontSize));
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

int OscilTabs::getPreferredHeight() const
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

void OscilTabs::resized()
{
    updateLayoutCache();

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
    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        if (getTabBounds(i).contains(pos))
            return i;
    }
    return -1;
}

void OscilTabs::mouseDown(const juce::MouseEvent& e)
{
    int const index = getTabAtPosition(e.getPosition());
    if (index >= 0 && tabs_[static_cast<size_t>(index)].enabled)
        setSelectedIndex(index);
}

void OscilTabs::mouseMove(const juce::MouseEvent& e)
{
    int const newHovered = getTabAtPosition(e.getPosition());
    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();
    }
}

void OscilTabs::mouseExit(const juce::MouseEvent& /*event*/)
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

void OscilTabs::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void OscilTabs::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void OscilTabs::timerCallback()
{
    updateAnimations();

    if (indicatorXSpring_.isSettled() && indicatorWidthSpring_.isSettled() && hoverSpring_.isSettled())
    {
        stopTimer();
    }

    repaint();
}

void OscilTabs::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    indicatorXSpring_.update(dt);
    indicatorWidthSpring_.update(dt);
    hoverSpring_.update(dt);
}

std::unique_ptr<juce::AccessibilityHandler> OscilTabs::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

} // namespace oscil
