/*
    MultiScoper - Dynamic Height Content Interface
    Used by MultiScoperAccordionSection to respond to content size changes
*/

#pragma once

#include <functional>

namespace multiscoper
{

/**
 * Interface for components that change their height dynamically based on state
 * Components implementing this interface can notify their parent MultiScoperAccordionSection
 * to recalculate layout when their size requirements change.
 *
 * Usage:
 * 1. Component inherits from DynamicHeightContent
 * 2. Component implements getPreferredHeight() returning current desired height
 * 3. When component state changes (e.g., mode switch), it calls onPreferredHeightChanged()
 * 4. MultiScoperAccordionSection detects this interface and wires up the callback
 */
class DynamicHeightContent
{
public:
    virtual ~DynamicHeightContent() = default;

    /**
     * Calculate the preferred height based on current state
     */
    virtual int getPreferredHeight() const = 0;

    /**
     * Callback set by parent MultiScoperAccordionSection to receive height change notifications.
     * Single-writer contract: only the owning MultiScoperAccordionSection may set/clear this.
     * Implementations call this when their preferred height changes (e.g., mode switch).
     */
    std::function<void()> onPreferredHeightChanged;
};

} // namespace multiscoper
