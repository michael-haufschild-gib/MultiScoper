/*
    Oscil - Oscillator Builder
    Fluent builder for creating Oscillator objects in tests
*/

#pragma once

#include "core/Oscillator.h"
#include <memory>

namespace oscil::test
{

/**
 * Fluent builder for creating test Oscillator objects
 *
 * Example usage:
 *   auto oscillator = OscillatorBuilder()
 *       .withName("Test Oscillator")
 *       .withSourceId(sourceId)
 *       .withProcessingMode(ProcessingMode::Mid)
 *       .withColour(juce::Colours::cyan)
 *       .withVisibility(true)
 *       .build();
 */
class OscillatorBuilder
{
public:
    OscillatorBuilder()
        : id_(OscillatorId::generate())
        , processingMode_(ProcessingMode::FullStereo)
        , colour_(juce::Colours::white)
        , lineWidth_(Oscillator::DEFAULT_LINE_WIDTH)
        , isVisible_(true)
        , orderIndex_(0)
    {
    }

    /**
     * Set a specific oscillator ID (otherwise auto-generated)
     */
    OscillatorBuilder& withId(const juce::String& id)
    {
        id_ = OscillatorId{ id };
        return *this;
    }

    /**
     * Set a specific oscillator ID
     */
    OscillatorBuilder& withId(const OscillatorId& id)
    {
        id_ = id;
        return *this;
    }

    /**
     * Set the oscillator name
     */
    OscillatorBuilder& withName(const juce::String& name)
    {
        name_ = name;
        return *this;
    }

    /**
     * Set the source ID
     */
    OscillatorBuilder& withSourceId(const SourceId& sourceId)
    {
        sourceId_ = sourceId;
        return *this;
    }

    /**
     * Set no source (NO_SOURCE)
     */
    OscillatorBuilder& withNoSource()
    {
        sourceId_ = SourceId::noSource();
        return *this;
    }

    /**
     * Set the processing mode
     */
    OscillatorBuilder& withProcessingMode(ProcessingMode mode)
    {
        processingMode_ = mode;
        return *this;
    }

    /**
     * Set as Full Stereo mode
     */
    OscillatorBuilder& asFullStereo()
    {
        processingMode_ = ProcessingMode::FullStereo;
        return *this;
    }

    /**
     * Set as Mono mode
     */
    OscillatorBuilder& asMono()
    {
        processingMode_ = ProcessingMode::Mono;
        return *this;
    }

    /**
     * Set as Mid mode
     */
    OscillatorBuilder& asMid()
    {
        processingMode_ = ProcessingMode::Mid;
        return *this;
    }

    /**
     * Set as Side mode
     */
    OscillatorBuilder& asSide()
    {
        processingMode_ = ProcessingMode::Side;
        return *this;
    }

    /**
     * Set as Left channel only
     */
    OscillatorBuilder& asLeft()
    {
        processingMode_ = ProcessingMode::Left;
        return *this;
    }

    /**
     * Set as Right channel only
     */
    OscillatorBuilder& asRight()
    {
        processingMode_ = ProcessingMode::Right;
        return *this;
    }

    /**
     * Set the oscillator color
     */
    OscillatorBuilder& withColour(juce::Colour colour)
    {
        colour_ = colour;
        return *this;
    }

    /**
     * Set the line width
     */
    OscillatorBuilder& withLineWidth(float width)
    {
        lineWidth_ = juce::jlimit(Oscillator::MIN_LINE_WIDTH, Oscillator::MAX_LINE_WIDTH, width);
        return *this;
    }

    /**
     * Set visibility
     */
    OscillatorBuilder& withVisibility(bool visible)
    {
        isVisible_ = visible;
        return *this;
    }

    /**
     * Set as visible
     */
    OscillatorBuilder& asVisible()
    {
        isVisible_ = true;
        return *this;
    }

    /**
     * Set as hidden
     */
    OscillatorBuilder& asHidden()
    {
        isVisible_ = false;
        return *this;
    }

    /**
     * Set the order index
     */
    OscillatorBuilder& withOrderIndex(int index)
    {
        orderIndex_ = index;
        return *this;
    }

    /**
     * Set the pane ID
     */
    OscillatorBuilder& withPaneId(const PaneId& paneId)
    {
        paneId_ = paneId;
        return *this;
    }

    /**
     * Build and return a unique_ptr to the Oscillator
     */
    std::unique_ptr<Oscillator> buildUnique()
    {
        auto osc = std::make_unique<Oscillator>();

        // Note: We can't directly set the ID after construction in the actual Oscillator class
        // So we'll use the auto-generated ID. If specific ID is needed, tests should construct differently.

        if (name_.has_value())
        {
            osc->setName(name_.value());
        }

        if (sourceId_.has_value())
        {
            osc->setSourceId(sourceId_.value());
        }

        osc->setProcessingMode(processingMode_);
        osc->setColour(colour_);
        osc->setLineWidth(lineWidth_);
        osc->setVisible(isVisible_);
        osc->setOrderIndex(orderIndex_);

        if (paneId_.has_value())
        {
            osc->setPaneId(paneId_.value());
        }

        return osc;
    }

    /**
     * Build and return an Oscillator by value
     */
    Oscillator build()
    {
        auto ptr = buildUnique();
        return std::move(*ptr);
    }

    /**
     * Build and return a shared_ptr to the Oscillator
     */
    std::shared_ptr<Oscillator> buildShared()
    {
        return std::shared_ptr<Oscillator>(buildUnique().release());
    }

private:
    OscillatorId id_;
    std::optional<juce::String> name_;
    std::optional<SourceId> sourceId_;
    ProcessingMode processingMode_;
    juce::Colour colour_;
    float lineWidth_;
    bool isVisible_;
    int orderIndex_;
    std::optional<PaneId> paneId_;
};

/**
 * Helper function to quickly create an OscillatorId
 */
inline OscillatorId makeOscillatorId(const juce::String& id)
{
    return OscillatorId{ id };
}

/**
 * Helper function to quickly create a PaneId
 */
inline PaneId makePaneId(const juce::String& id)
{
    return PaneId{ id };
}

} // namespace oscil::test
