/*
    Oscil - Dynamic Height Content Interface
    Interface for content components that can dynamically change their preferred height
    Used by CollapsibleSection to respond to content size changes
*/

#pragma once

#include <functional>

namespace oscil
{

/**
 * Interface for content that has dynamic height based on state
 *
 * Components implementing this interface can notify their parent CollapsibleSection
 * when their preferred height changes (e.g., when switching timing modes).
 *
 * Usage:
 * 1. Content component inherits from DynamicHeightContent
 * 2. Content implements getPreferredHeight()
 * 3. Content calls notifyHeightChanged() when its height needs to change
 * 4. CollapsibleSection detects this interface and wires up the callback
 */
class DynamicHeightContent
{
public:
    virtual ~DynamicHeightContent() = default;

    /**
     * Get the preferred height for this content based on current state
     * This should return the ideal height needed to display all content
     */
    virtual int getPreferredHeight() const = 0;

    /**
     * Callback set by parent CollapsibleSection to receive height change notifications
     * The parent uses this to know when to re-layout
     */
    std::function<void()> onPreferredHeightChanged;

protected:
    /**
     * Call this when the content's preferred height changes
     * E.g., when switching from TIME to MELODIC mode
     */
    void notifyHeightChanged()
    {
        if (onPreferredHeightChanged)
        {
            onPreferredHeightChanged();
        }
    }
};

} // namespace oscil
