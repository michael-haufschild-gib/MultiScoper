/*
    Oscil - Dynamic Height Content Interface
    Used by OscilAccordionSection to respond to content size changes
*/

#pragma once

#include <functional>

namespace oscil
{

/**
 * Interface for components that change their height dynamically based on state
 * Components implementing this interface can notify their parent OscilAccordionSection
 * to recalculate layout when their size requirements change.
 *
 * Usage:
 * 1. Component inherits from DynamicHeightContent
 * 2. Component implements getPreferredHeight() returning current desired height
 * 3. When component state changes (e.g., mode switch), it calls onPreferredHeightChanged()
 * 4. OscilAccordionSection detects this interface and wires up the callback
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
     * Callback set by parent OscilAccordionSection to receive height change notifications
     */
    std::function<void()> onPreferredHeightChanged;
};

} // namespace oscil
