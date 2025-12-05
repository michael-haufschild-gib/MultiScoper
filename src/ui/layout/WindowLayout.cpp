/*
    Oscil - Window Layout
    Implementation of window dimension and sidebar management
*/

#include "ui/layout/WindowLayout.h"
#include <juce_events/juce_events.h>

namespace oscil
{

WindowLayout::WindowLayout()
    : windowWidth_(DEFAULT_WINDOW_WIDTH)
    , windowHeight_(DEFAULT_WINDOW_HEIGHT)
    , sidebarWidth_(DEFAULT_SIDEBAR_WIDTH)
    , sidebarCollapsed_(false)
{
}

WindowLayout::WindowLayout(const juce::ValueTree& state)
    : WindowLayout()
{
    fromValueTree(state);
}

juce::ValueTree WindowLayout::toValueTree() const
{
    juce::ValueTree tree(WindowLayoutIds::WindowLayout);
    tree.setProperty(WindowLayoutIds::WindowWidth, windowWidth_, nullptr);
    tree.setProperty(WindowLayoutIds::WindowHeight, windowHeight_, nullptr);
    tree.setProperty(WindowLayoutIds::SidebarWidth, sidebarWidth_, nullptr);
    tree.setProperty(WindowLayoutIds::SidebarCollapsed, sidebarCollapsed_, nullptr);
    return tree;
}

void WindowLayout::fromValueTree(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != WindowLayoutIds::WindowLayout)
        return;

    windowWidth_ = state.getProperty(WindowLayoutIds::WindowWidth, DEFAULT_WINDOW_WIDTH);
    windowHeight_ = state.getProperty(WindowLayoutIds::WindowHeight, DEFAULT_WINDOW_HEIGHT);
    sidebarWidth_ = state.getProperty(WindowLayoutIds::SidebarWidth, DEFAULT_SIDEBAR_WIDTH);
    sidebarCollapsed_ = state.getProperty(WindowLayoutIds::SidebarCollapsed, false);

    // Validate loaded values
    windowWidth_ = juce::jlimit(MIN_WINDOW_WIDTH, MAX_WINDOW_WIDTH, windowWidth_);
    windowHeight_ = juce::jlimit(MIN_WINDOW_HEIGHT, MAX_WINDOW_HEIGHT, windowHeight_);
    sidebarWidth_ = juce::jlimit(MIN_SIDEBAR_WIDTH, MAX_SIDEBAR_WIDTH, sidebarWidth_);
}

void WindowLayout::setWindowWidth(int width)
{
    width = juce::jlimit(MIN_WINDOW_WIDTH, MAX_WINDOW_WIDTH, width);
    if (windowWidth_ != width)
    {
        windowWidth_ = width;
        // Ensure sidebar width is still valid for new window width
        sidebarWidth_ = clampSidebarWidth(sidebarWidth_);
        notifyWindowSizeChanged();
    }
}

void WindowLayout::setWindowHeight(int height)
{
    height = juce::jlimit(MIN_WINDOW_HEIGHT, MAX_WINDOW_HEIGHT, height);
    if (windowHeight_ != height)
    {
        windowHeight_ = height;
        notifyWindowSizeChanged();
    }
}

void WindowLayout::setWindowSize(int width, int height)
{
    width = juce::jlimit(MIN_WINDOW_WIDTH, MAX_WINDOW_WIDTH, width);
    height = juce::jlimit(MIN_WINDOW_HEIGHT, MAX_WINDOW_HEIGHT, height);

    bool changed = (windowWidth_ != width || windowHeight_ != height);
    windowWidth_ = width;
    windowHeight_ = height;

    // Ensure sidebar width is still valid for new window width
    int clampedSidebar = clampSidebarWidth(sidebarWidth_);
    if (clampedSidebar != sidebarWidth_)
    {
        sidebarWidth_ = clampedSidebar;
        notifySidebarWidthChanged();
    }

    if (changed)
    {
        notifyWindowSizeChanged();
    }
}

void WindowLayout::setSidebarWidth(int width)
{
    width = clampSidebarWidth(width);
    if (sidebarWidth_ != width)
    {
        sidebarWidth_ = width;
        notifySidebarWidthChanged();
    }
}

void WindowLayout::setSidebarCollapsed(bool collapsed)
{
    if (sidebarCollapsed_ != collapsed)
    {
        sidebarCollapsed_ = collapsed;
        notifySidebarCollapseStateChanged();
    }
}

void WindowLayout::toggleSidebarCollapsed()
{
    setSidebarCollapsed(!sidebarCollapsed_);
}

int WindowLayout::getEffectiveSidebarWidth() const
{
    return sidebarCollapsed_ ? COLLAPSED_SIDEBAR_WIDTH : sidebarWidth_;
}

int WindowLayout::getOscilloscopeAreaWidth() const
{
    return windowWidth_ - getEffectiveSidebarWidth();
}

int WindowLayout::clampSidebarWidth(int proposedWidth) const
{
    // Ensure minimum sidebar width
    int minWidth = MIN_SIDEBAR_WIDTH;

    // Ensure maximum sidebar width doesn't make oscilloscope too small
    int maxWidth = juce::jmin(MAX_SIDEBAR_WIDTH,
                               windowWidth_ - MIN_OSCILLOSCOPE_WIDTH);

    return juce::jlimit(minWidth, maxWidth, proposedWidth);
}

bool WindowLayout::isValidWindowSize(int width, int height) const
{
    return width >= MIN_WINDOW_WIDTH && width <= MAX_WINDOW_WIDTH &&
           height >= MIN_WINDOW_HEIGHT && height <= MAX_WINDOW_HEIGHT;
}

void WindowLayout::addListener(Listener* listener)
{
    // ListenerList is not thread-safe - must be called from message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.add(listener);
}

void WindowLayout::removeListener(Listener* listener)
{
    // ListenerList is not thread-safe - must be called from message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.remove(listener);
}

void WindowLayout::notifyWindowSizeChanged()
{
    listeners_.call([this](Listener& l) { l.windowSizeChanged(windowWidth_, windowHeight_); });
}

void WindowLayout::notifySidebarWidthChanged()
{
    listeners_.call([this](Listener& l) { l.sidebarWidthChanged(sidebarWidth_); });
}

void WindowLayout::notifySidebarCollapseStateChanged()
{
    listeners_.call([this](Listener& l) { l.sidebarCollapseStateChanged(sidebarCollapsed_); });
}

} // namespace oscil
