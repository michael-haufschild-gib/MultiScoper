/*
    Oscil - Layout Coordinator
    Handles WindowLayout::Listener callbacks for layout changes
*/

#pragma once

#include "ui/layout/WindowLayout.h"

#include <functional>

namespace oscil
{

/**
 * Coordinates layout change events for the plugin editor.
 * Encapsulates WindowLayout::Listener implementation to reduce
 * editor responsibilities.
 */
class LayoutCoordinator : public WindowLayout::Listener
{
public:
    using LayoutChangedCallback = std::function<void()>;

    /**
     * Create coordinator with window layout reference and change callback.
     * Automatically registers as listener on construction.
     */
    LayoutCoordinator(WindowLayout& layout, LayoutChangedCallback onLayoutChanged);

    /**
     * Destructor automatically unregisters from layout.
     */
    ~LayoutCoordinator() override;

    /// Respond to window resize by re-laying out panes (WindowLayout::Listener).
    void windowSizeChanged(int width, int height) override;
    /// Respond to sidebar width drag (WindowLayout::Listener).
    void sidebarWidthChanged(int width) override;
    /// Respond to sidebar collapse toggle (WindowLayout::Listener).
    void sidebarCollapseStateChanged(bool collapsed) override;

    /**
     * Get the window layout reference.
     */
    WindowLayout& getLayout() { return layout_; }
    const WindowLayout& getLayout() const { return layout_; }

    // Prevent copying
    LayoutCoordinator(const LayoutCoordinator&) = delete;
    LayoutCoordinator& operator=(const LayoutCoordinator&) = delete;

private:
    WindowLayout& layout_;
    LayoutChangedCallback onLayoutChanged_;
};

} // namespace oscil
