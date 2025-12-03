/*
    Oscil - Layout Coordinator Implementation
*/

#include "ui/layout/LayoutCoordinator.h"

namespace oscil
{

LayoutCoordinator::LayoutCoordinator(WindowLayout& layout, LayoutChangedCallback onLayoutChanged)
    : layout_(layout)
    , onLayoutChanged_(std::move(onLayoutChanged))
{
    // Register as listener
    layout_.addListener(this);
}

LayoutCoordinator::~LayoutCoordinator()
{
    // Unregister from layout
    layout_.removeListener(this);
}

void LayoutCoordinator::windowSizeChanged(int /*width*/, int /*height*/)
{
    // Window size is managed by JUCE, this is primarily for state persistence
    // No action needed here
}

void LayoutCoordinator::sidebarWidthChanged(int /*width*/)
{
    if (onLayoutChanged_)
    {
        onLayoutChanged_();
    }
}

void LayoutCoordinator::sidebarCollapseStateChanged(bool /*collapsed*/)
{
    if (onLayoutChanged_)
    {
        onLayoutChanged_();
    }
}

} // namespace oscil
