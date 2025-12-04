/*
    Oscil - Oscillator Implementation
    PRD aligned with extended display properties and NO_SOURCE state
*/

#include "core/Oscillator.h"
#include "core/OscilState.h"

namespace oscil
{

OscillatorId OscillatorId::generate()
{
    return OscillatorId{ juce::Uuid().toString() };
}

PaneId PaneId::generate()
{
    return PaneId{ juce::Uuid().toString() };
}

Oscillator::Oscillator()
    : id_(OscillatorId::generate())
    , state_(OscillatorState::NO_SOURCE)
    , shaderId_("basic")
    , visualPresetId_("default")
{
}

Oscillator::Oscillator(const juce::ValueTree& state)
    : id_(OscillatorId::generate())
    , state_(OscillatorState::NO_SOURCE)
{
    fromValueTree(state);
}

juce::ValueTree Oscillator::toValueTree() const
{
    juce::ValueTree state(StateIds::Oscillator);

    state.setProperty(StateIds::Id, id_.id, nullptr);
    state.setProperty(StateIds::SourceId, sourceId_.id, nullptr);
    state.setProperty(StateIds::OscillatorState, static_cast<int>(state_), nullptr);
    state.setProperty(StateIds::ProcessingMode, processingModeToString(processingMode_), nullptr);
    state.setProperty(StateIds::Colour, static_cast<int>(colour_.getARGB()), nullptr);
    state.setProperty(StateIds::Opacity, opacity_, nullptr);
    state.setProperty(StateIds::PaneId, paneId_.id, nullptr);
    state.setProperty(StateIds::Order, orderIndex_, nullptr);
    state.setProperty(StateIds::Visible, visible_, nullptr);
    state.setProperty(StateIds::Name, name_, nullptr);
    state.setProperty(StateIds::SchemaVersion, schemaVersion_, nullptr);

    // Extended display properties
    state.setProperty(StateIds::LineWidth, lineWidth_, nullptr);

    if (timeWindow_.has_value())
    {
        state.setProperty(StateIds::TimeWindow, timeWindow_.value(), nullptr);
    }

    state.setProperty(StateIds::ShaderId, shaderId_, nullptr);
    state.setProperty(StateIds::VisualPresetId, visualPresetId_, nullptr);

    // Serialize overrides
    if (visualOverrides_.getNumProperties() > 0)
    {
        state.addChild(visualOverrides_.createCopy(), -1, nullptr);
    }

    return state;
}

void Oscillator::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType(StateIds::Oscillator))
        return;

    int loadedSchemaVersion = state.getProperty(StateIds::SchemaVersion, 1);

    id_.id = state.getProperty(StateIds::Id, id_.id);
    sourceId_.id = state.getProperty(StateIds::SourceId, "");

    // Handle oscillator state (migrate from old schema if needed)
    if (loadedSchemaVersion >= 2)
    {
        state_ = static_cast<OscillatorState>(
            static_cast<int>(state.getProperty(StateIds::OscillatorState, 0)));
    }
    else
    {
        // Migration: if sourceId is valid, assume ACTIVE
        state_ = sourceId_.isValid() ? OscillatorState::ACTIVE : OscillatorState::NO_SOURCE;
    }

    processingMode_ = stringToProcessingMode(state.getProperty(StateIds::ProcessingMode, "FullStereo"));
    colour_ = juce::Colour(static_cast<juce::uint32>(static_cast<int>(state.getProperty(StateIds::Colour, static_cast<int>(WaveformColorPalette::getRandomColor().getARGB())))));
    opacity_ = state.getProperty(StateIds::Opacity, 1.0f);
    paneId_.id = state.getProperty(StateIds::PaneId, "");
    orderIndex_ = state.getProperty(StateIds::Order, 0);
    visible_ = state.getProperty(StateIds::Visible, true);
    name_ = state.getProperty(StateIds::Name, "");

    // Extended display properties (with defaults for migration)
    lineWidth_ = state.getProperty(StateIds::LineWidth, DEFAULT_LINE_WIDTH);

    if (state.hasProperty(StateIds::TimeWindow))
    {
        timeWindow_ = static_cast<float>(state.getProperty(StateIds::TimeWindow));
    }
    else
    {
        timeWindow_ = std::nullopt;
    }

    // Validate and clamp values
    lineWidth_ = juce::jlimit(MIN_LINE_WIDTH, MAX_LINE_WIDTH, lineWidth_);
    opacity_ = juce::jlimit(0.0f, 1.0f, opacity_);

    // Shader ID (default to basic for migration)
    shaderId_ = state.getProperty(StateIds::ShaderId, "basic");

    // Visual preset ID (default to "default" for migration)
    visualPresetId_ = state.getProperty(StateIds::VisualPresetId, "default").toString();

    // Load overrides
    auto overrides = state.getChildWithName("VisualOverrides");
    if (overrides.isValid())
    {
        visualOverrides_ = overrides.createCopy();
    }
    else
    {
        visualOverrides_ = juce::ValueTree("VisualOverrides");
    }

    schemaVersion_ = CURRENT_SCHEMA_VERSION;
}

void Oscillator::setVisualOverride(const juce::Identifier& property, const juce::var& value)
{
    visualOverrides_.setProperty(property, value, nullptr);
}

juce::var Oscillator::getVisualOverride(const juce::Identifier& property) const
{
    return visualOverrides_.getProperty(property);
}

void Oscillator::clearVisualOverrides()
{
    visualOverrides_.removeAllProperties(nullptr);
}

void Oscillator::setSourceId(const SourceId& sourceId)
{
    sourceId_ = sourceId;

    // Automatically transition state based on source validity
    if (sourceId.isValid())
    {
        state_ = OscillatorState::ACTIVE;
    }
    else
    {
        state_ = OscillatorState::NO_SOURCE;
    }
}

void Oscillator::clearSource()
{
    // Preserve configuration but transition to NO_SOURCE
    sourceId_ = SourceId::invalid();
    state_ = OscillatorState::NO_SOURCE;
    // All other settings (colour, processingMode, etc.) are preserved
}

void Oscillator::setName(const juce::String& name)
{
    // Truncate to MAX_NAME_LENGTH if needed
    if (name.length() > MAX_NAME_LENGTH)
    {
        name_ = name.substring(0, MAX_NAME_LENGTH);
    }
    else
    {
        name_ = name;
    }
}

bool Oscillator::isValidName(const juce::String& name)
{
    return name.length() >= MIN_NAME_LENGTH && name.length() <= MAX_NAME_LENGTH;
}

} // namespace oscil
