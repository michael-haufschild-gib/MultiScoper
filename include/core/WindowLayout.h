/*
    Oscil - Window Layout
    Manages plugin window dimensions and sidebar configuration
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace oscil
{

/**
 * Window layout configuration for plugin instance.
 * Manages window dimensions and sidebar state per-instance.
 */
class WindowLayout
{
public:
    // Dimension constraints from PRD
    static constexpr int MIN_WINDOW_WIDTH = 800;
    static constexpr int MAX_WINDOW_WIDTH = 3840;
    static constexpr int MIN_WINDOW_HEIGHT = 600;
    static constexpr int MAX_WINDOW_HEIGHT = 2160;
    static constexpr int MIN_SIDEBAR_WIDTH = 200;
    static constexpr int MAX_SIDEBAR_WIDTH = 800;
    static constexpr int MIN_OSCILLOSCOPE_WIDTH = 400;
    static constexpr int COLLAPSED_SIDEBAR_WIDTH = 50;

    // Default values
    static constexpr int DEFAULT_WINDOW_WIDTH = 1200;
    static constexpr int DEFAULT_WINDOW_HEIGHT = 800;
    static constexpr int DEFAULT_SIDEBAR_WIDTH = 300;

    WindowLayout();

    /**
     * Create from ValueTree state
     */
    explicit WindowLayout(const juce::ValueTree& state);

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    // Window dimensions
    int getWindowWidth() const { return windowWidth_; }
    int getWindowHeight() const { return windowHeight_; }

    void setWindowWidth(int width);
    void setWindowHeight(int height);
    void setWindowSize(int width, int height);

    // Sidebar
    int getSidebarWidth() const { return sidebarWidth_; }
    bool isSidebarCollapsed() const { return sidebarCollapsed_; }

    void setSidebarWidth(int width);
    void setSidebarCollapsed(bool collapsed);
    void toggleSidebarCollapsed();

    /**
     * Get effective sidebar width (considering collapsed state)
     */
    int getEffectiveSidebarWidth() const;

    /**
     * Get oscilloscope display area width
     */
    int getOscilloscopeAreaWidth() const;

    /**
     * Validate and clamp sidebar width given current window width
     */
    int clampSidebarWidth(int proposedWidth) const;

    /**
     * Check if proposed window size is valid
     */
    bool isValidWindowSize(int width, int height) const;

    /**
     * Listener interface for layout changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void windowSizeChanged(int /*width*/, int /*height*/) {}
        virtual void sidebarWidthChanged(int /*width*/) {}
        virtual void sidebarCollapseStateChanged(bool /*collapsed*/) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    int windowWidth_ = DEFAULT_WINDOW_WIDTH;
    int windowHeight_ = DEFAULT_WINDOW_HEIGHT;
    int sidebarWidth_ = DEFAULT_SIDEBAR_WIDTH;
    bool sidebarCollapsed_ = false;

    juce::ListenerList<Listener> listeners_;

    void notifyWindowSizeChanged();
    void notifySidebarWidthChanged();
    void notifySidebarCollapseStateChanged();
};

/**
 * State for sidebar resize drag operation
 */
struct SidebarResizeState
{
    bool isResizing = false;
    int dragStartX = 0;
    int originalSidebarWidth = 0;
    int currentSidebarWidth = 0;
    int previewWidth = 0;

    void startResize(int mouseX, int currentWidth)
    {
        isResizing = true;
        dragStartX = mouseX;
        originalSidebarWidth = currentWidth;
        currentSidebarWidth = currentWidth;
        previewWidth = currentWidth;
    }

    void updateResize(int mouseX, int windowWidth)
    {
        if (!isResizing) return;

        // Sidebar is on the right, so moving left increases width
        int delta = dragStartX - mouseX;
        previewWidth = originalSidebarWidth + delta;

        // Clamp to valid range
        previewWidth = juce::jlimit(
            WindowLayout::MIN_SIDEBAR_WIDTH,
            juce::jmin(WindowLayout::MAX_SIDEBAR_WIDTH,
                       windowWidth - WindowLayout::MIN_OSCILLOSCOPE_WIDTH),
            previewWidth
        );
    }

    void endResize()
    {
        if (isResizing)
        {
            currentSidebarWidth = previewWidth;
        }
        isResizing = false;
    }

    void cancelResize()
    {
        isResizing = false;
        previewWidth = originalSidebarWidth;
    }
};

// ValueTree identifiers for WindowLayout
namespace WindowLayoutIds
{
    static const juce::Identifier WindowLayout{ "WindowLayout" };
    static const juce::Identifier WindowWidth{ "windowWidth" };
    static const juce::Identifier WindowHeight{ "windowHeight" };
    static const juce::Identifier SidebarWidth{ "sidebarWidth" };
    static const juce::Identifier SidebarCollapsed{ "sidebarCollapsed" };
}

} // namespace oscil
