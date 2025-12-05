/*
    Oscil - Oscillator Model
    Represents a single visualization unit with source, processing, and appearance settings
*/

#pragma once

#include "core/Source.h"
#include "core/WaveformColorPalette.h"

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <optional>

namespace oscil
{

/**
 * Processing modes available per oscillator
 */
enum class ProcessingMode
{
    FullStereo, // Pass-through L/R visualization (two traces)
    Mono,       // (L+R)/2 summed mono representation
    Mid,        // Mid component M=(L+R)/2 (single trace)
    Side,       // Side component S=(L-R)/2 (single trace)
    Left,       // Left channel only
    Right       // Right channel only
};

/**
 * Convert ProcessingMode to string for serialization
 */
inline juce::String processingModeToString(ProcessingMode mode)
{
    switch (mode)
    {
        case ProcessingMode::FullStereo:
            return "FullStereo";
        case ProcessingMode::Mono:
            return "Mono";
        case ProcessingMode::Mid:
            return "Mid";
        case ProcessingMode::Side:
            return "Side";
        case ProcessingMode::Left:
            return "Left";
        case ProcessingMode::Right:
            return "Right";
        default:
            return "FullStereo";
    }
}

/**
 * Convert string to ProcessingMode for deserialization
 */
inline ProcessingMode stringToProcessingMode(const juce::String& str)
{
    if (str == "FullStereo")
        return ProcessingMode::FullStereo;
    if (str == "Mono")
        return ProcessingMode::Mono;
    if (str == "Mid")
        return ProcessingMode::Mid;
    if (str == "Side")
        return ProcessingMode::Side;
    if (str == "Left")
        return ProcessingMode::Left;
    if (str == "Right")
        return ProcessingMode::Right;
    return ProcessingMode::FullStereo;
}

/**
 * Unique identifier for an oscillator
 */
struct OscillatorId
{
    juce::String id;

    bool operator==(const OscillatorId& other) const { return id == other.id; }
    bool operator!=(const OscillatorId& other) const { return !(*this == other); }

    [[nodiscard]] static OscillatorId generate();
    static OscillatorId invalid() { return OscillatorId{""}; }
    bool isValid() const { return id.isNotEmpty(); }
};

/**
 * Unique identifier for a pane
 */
struct PaneId
{
    juce::String id;

    bool operator==(const PaneId& other) const { return id == other.id; }
    bool operator!=(const PaneId& other) const { return !(*this == other); }

    [[nodiscard]] static PaneId generate();
    static PaneId invalid() { return PaneId{""}; }
    bool isValid() const { return id.isNotEmpty(); }
};

/**
 * Hash functions for ID types
 */
struct OscillatorIdHash
{
    std::size_t operator()(const OscillatorId& oid) const
    {
        return std::hash<std::string>{}(oid.id.toStdString());
    }
};

struct PaneIdHash
{
    std::size_t operator()(const PaneId& pid) const
    {
        return std::hash<std::string>{}(pid.id.toStdString());
    }
};

/**
 * Oscillator state for source assignment tracking
 */
enum class OscillatorState
{
    ACTIVE,   // Source is assigned and providing data
    NO_SOURCE // Source disconnected, preserving config for reconnection
};

/**
 * Oscillator configuration and state.
 * An oscillator is a user-created visualization unit referencing exactly one Source
 * and applying a processing mode + color + visibility.
 * PRD aligned with extended display properties.
 */
class Oscillator
{
public:
    // Display constraints from PRD
    static constexpr float MIN_LINE_WIDTH = 0.5f;
    static constexpr float MAX_LINE_WIDTH = 5.0f;
    static constexpr float DEFAULT_LINE_WIDTH = 1.5f;

    static constexpr int MIN_NAME_LENGTH = 1;
    static constexpr int MAX_NAME_LENGTH = 32;

    /**
     * Create a new oscillator with generated ID
     */
    Oscillator();

    /**
     * Create an oscillator from serialized ValueTree
     */
    explicit Oscillator(const juce::ValueTree& state);

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load state from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    // Getters
    [[nodiscard]] OscillatorId getId() const noexcept { return id_; }
    [[nodiscard]] SourceId getSourceId() const noexcept { return sourceId_; }
    [[nodiscard]] ProcessingMode getProcessingMode() const noexcept { return processingMode_; }
    [[nodiscard]] juce::Colour getColour() const noexcept { return colour_; }
    [[nodiscard]] float getOpacity() const noexcept { return opacity_; }
    [[nodiscard]] PaneId getPaneId() const noexcept { return paneId_; }
    [[nodiscard]] int getOrderIndex() const noexcept { return orderIndex_; }
    [[nodiscard]] bool isVisible() const noexcept { return visible_; }
    [[nodiscard]] juce::String getName() const noexcept { return name_; }
    [[nodiscard]] OscillatorState getState() const noexcept { return state_; }
    [[nodiscard]] float getLineWidth() const noexcept { return lineWidth_; }
    [[nodiscard]] std::optional<float> getTimeWindow() const noexcept { return timeWindow_; }
    [[nodiscard]] juce::String getShaderId() const noexcept { return shaderId_; }
    [[nodiscard]] juce::String getVisualPresetId() const noexcept { return visualPresetId_; }

    // Setters
    void setSourceId(const SourceId& sourceId);
    void setProcessingMode(ProcessingMode mode) noexcept { processingMode_ = mode; }
    void setColour(juce::Colour colour) noexcept { colour_ = colour; }
    void setOpacity(float opacity) noexcept { opacity_ = juce::jlimit(0.0f, 1.0f, opacity); }
    void setPaneId(const PaneId& paneId) noexcept { paneId_ = paneId; }
    void setOrderIndex(int index) noexcept { orderIndex_ = index; }
    void setVisible(bool visible) noexcept { visible_ = visible; }
    void setName(const juce::String& name);
    void setLineWidth(float width) noexcept { lineWidth_ = juce::jlimit(MIN_LINE_WIDTH, MAX_LINE_WIDTH, width); }
    void setTimeWindow(std::optional<float> window) noexcept { timeWindow_ = window; }
    void setShaderId(const juce::String& shaderId) noexcept { shaderId_ = shaderId; }
    void setVisualPresetId(const juce::String& presetId) noexcept { visualPresetId_ = presetId; }

    // Visual overrides
    void setVisualOverride(const juce::Identifier& property, const juce::var& value);
    juce::var getVisualOverride(const juce::Identifier& property) const;
    void clearVisualOverrides();
    const juce::ValueTree& getVisualOverrides() const { return visualOverrides_; }

    /**
     * Clear source assignment (transitions to NO_SOURCE state)
     * Configuration is preserved for potential reconnection
     */
    void clearSource();

    /**
     * Check if oscillator has a valid source
     */
    [[nodiscard]] bool hasSource() const noexcept { return state_ == OscillatorState::ACTIVE && sourceId_.isValid(); }

    /**
     * Check if oscillator is in NO_SOURCE state
     */
    [[nodiscard]] bool isNoSource() const noexcept { return state_ == OscillatorState::NO_SOURCE; }

    /**
     * Get the effective colour with opacity applied
     */
    [[nodiscard]] juce::Colour getEffectiveColour() const noexcept
    {
        return colour_.withAlpha(opacity_);
    }

    /**
     * Check if this oscillator produces a single trace (mono-like modes)
     * vs two traces (FullStereo)
     */
    [[nodiscard]] bool isSingleTrace() const noexcept
    {
        return processingMode_ != ProcessingMode::FullStereo;
    }

    /**
     * Validate name according to PRD constraints (1-32 characters)
     */
    static bool isValidName(const juce::String& name);

    // Schema version for migration support
    static constexpr int CURRENT_SCHEMA_VERSION = 2;

private:
    OscillatorId id_;
    SourceId sourceId_;
    OscillatorState state_ = OscillatorState::NO_SOURCE;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    juce::Colour colour_{WaveformColorPalette::getRandomColor()};
    float opacity_ = 1.0f;
    PaneId paneId_;
    int orderIndex_ = 0;
    bool visible_ = true;
    juce::String name_;

    // PRD extended display properties
    float lineWidth_ = DEFAULT_LINE_WIDTH;
    std::optional<float> timeWindow_;                    // Per-oscillator time window override (seconds)
    juce::String shaderId_ = "basic";                    // Shader for GPU rendering
    juce::String visualPresetId_ = "default";            // Visual preset for render effects
    juce::ValueTree visualOverrides_{"VisualOverrides"}; // Custom visual settings

    int schemaVersion_ = CURRENT_SCHEMA_VERSION;
};

} // namespace oscil
