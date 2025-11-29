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
    state.setProperty(StateIds::VerticalScale, verticalScale_, nullptr);
    state.setProperty(StateIds::VerticalOffset, verticalOffset_, nullptr);

    if (timeWindow_.has_value())
    {
        state.setProperty(StateIds::TimeWindow, timeWindow_.value(), nullptr);
    }

    state.setProperty(StateIds::ShaderId, shaderId_, nullptr);

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
    colour_ = juce::Colour(static_cast<juce::uint32>(static_cast<int>(state.getProperty(StateIds::Colour, static_cast<int>(0xFF00FF00)))));
    opacity_ = state.getProperty(StateIds::Opacity, 1.0f);
    paneId_.id = state.getProperty(StateIds::PaneId, "");
    orderIndex_ = state.getProperty(StateIds::Order, 0);
    visible_ = state.getProperty(StateIds::Visible, true);
    name_ = state.getProperty(StateIds::Name, "");

    // Extended display properties (with defaults for migration)
    lineWidth_ = state.getProperty(StateIds::LineWidth, DEFAULT_LINE_WIDTH);
    verticalScale_ = state.getProperty(StateIds::VerticalScale, DEFAULT_VERTICAL_SCALE);
    verticalOffset_ = state.getProperty(StateIds::VerticalOffset, DEFAULT_VERTICAL_OFFSET);

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
    verticalScale_ = juce::jlimit(MIN_VERTICAL_SCALE, MAX_VERTICAL_SCALE, verticalScale_);
    verticalOffset_ = juce::jlimit(MIN_VERTICAL_OFFSET, MAX_VERTICAL_OFFSET, verticalOffset_);
    opacity_ = juce::jlimit(0.0f, 1.0f, opacity_);

    // Shader ID (default to neon_glow for migration)
    shaderId_ = state.getProperty(StateIds::ShaderId, "neon_glow");

    schemaVersion_ = CURRENT_SCHEMA_VERSION;
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
