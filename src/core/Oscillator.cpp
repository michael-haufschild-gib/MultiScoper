/*
    Oscil - Oscillator Implementation
    PRD aligned with extended display properties and NO_SOURCE state
*/

#include "core/Oscillator.h"

#include "core/OscilLog.h"
#include "core/OscilState.h"

namespace oscil
{

OscillatorId OscillatorId::generate() { return OscillatorId{juce::Uuid().toString()}; }

PaneId PaneId::generate() { return PaneId{juce::Uuid().toString()}; }

Oscillator::Oscillator()
    : id_(OscillatorId::generate())
    , state_(OscillatorState::NO_SOURCE)
    , shaderId_("basic")
    , visualPresetId_("default")
{
}

Oscillator::Oscillator(const juce::ValueTree& state) : id_(OscillatorId::generate()), state_(OscillatorState::NO_SOURCE)
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

OscillatorState Oscillator::migrateOscillatorState(const juce::ValueTree& state, int schemaVersion,
                                                   const SourceId& sourceId)
{
    if (schemaVersion >= 2)
    {
        const int rawState = static_cast<int>(
            state.getProperty(StateIds::OscillatorState, static_cast<int>(OscillatorState::NO_SOURCE)));

        OscillatorState result;
        if (rawState == static_cast<int>(OscillatorState::ACTIVE) ||
            rawState == static_cast<int>(OscillatorState::NO_SOURCE))
            result = static_cast<OscillatorState>(rawState);
        else
            result = sourceId.isValid() ? OscillatorState::ACTIVE : OscillatorState::NO_SOURCE;

        // Enforce invariant: state and source validity must agree.
        if ((result == OscillatorState::ACTIVE) != sourceId.isValid())
            result = sourceId.isValid() ? OscillatorState::ACTIVE : OscillatorState::NO_SOURCE;

        return result;
    }

    return sourceId.isValid() ? OscillatorState::ACTIVE : OscillatorState::NO_SOURCE;
}

void Oscillator::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType(StateIds::Oscillator))
        return;

    int loadedSchemaVersion = state.getProperty(StateIds::SchemaVersion, 1);
    id_.id = state.getProperty(StateIds::Id, id_.id);
    sourceId_.id = state.getProperty(StateIds::SourceId, "");
    state_ = migrateOscillatorState(state, loadedSchemaVersion, sourceId_);

    processingMode_ = stringToProcessingMode(state.getProperty(StateIds::ProcessingMode, "FullStereo"));
    colour_ = juce::Colour(static_cast<juce::uint32>(
        static_cast<int>(state.getProperty(StateIds::Colour, static_cast<int>(colour_.getARGB())))));
    opacity_ = state.getProperty(StateIds::Opacity, 1.0f);
    paneId_.id = state.getProperty(StateIds::PaneId, "");
    orderIndex_ = state.getProperty(StateIds::Order, 0);
    visible_ = state.getProperty(StateIds::Visible, true);
    name_ = state.getProperty(StateIds::Name, "");

    lineWidth_ = state.getProperty(StateIds::LineWidth, DEFAULT_LINE_WIDTH);
    timeWindow_ = state.hasProperty(StateIds::TimeWindow)
                      ? std::optional<float>(static_cast<float>(state.getProperty(StateIds::TimeWindow)))
                      : std::nullopt;

    auto sanitizeNaN = [](float value, float fallback) { return std::isnan(value) ? fallback : value; };
    lineWidth_ = juce::jlimit(MIN_LINE_WIDTH, MAX_LINE_WIDTH, sanitizeNaN(lineWidth_, DEFAULT_LINE_WIDTH));
    opacity_ = juce::jlimit(0.0f, 1.0f, sanitizeNaN(opacity_, 1.0f));

    shaderId_ = state.getProperty(StateIds::ShaderId, "basic");
    visualPresetId_ = state.getProperty(StateIds::VisualPresetId, "default").toString();

    auto overrides = state.getChildWithName("VisualOverrides");
    visualOverrides_ = overrides.isValid() ? overrides.createCopy() : juce::ValueTree("VisualOverrides");

    schemaVersion_ = CURRENT_SCHEMA_VERSION;

    OSCIL_LOG(STATE, "Oscillator::fromValueTree: id="
                         << id_.id << " name=" << name_ << " sourceId=" << sourceId_.id << " paneId=" << paneId_.id
                         << " state=" << static_cast<int>(state_) << " mode=" << processingModeToString(processingMode_)
                         << " visible=" << (visible_ ? "true" : "false") << " shader=" << shaderId_);
}

void Oscillator::setVisualOverride(const juce::Identifier& property, const juce::var& value)
{
    visualOverrides_.setProperty(property, value, nullptr);
}

juce::var Oscillator::getVisualOverride(const juce::Identifier& property) const
{
    return visualOverrides_.getProperty(property);
}

void Oscillator::clearVisualOverrides() { visualOverrides_.removeAllProperties(nullptr); }

void Oscillator::setSourceId(const SourceId& sourceId)
{
    [[maybe_unused]] auto oldSourceId = sourceId_;
    [[maybe_unused]] auto oldState = state_;
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
    OSCIL_LOG(STATE, "Oscillator::setSourceId: id=" << id_.id << " name=" << name_ << " source=" << oldSourceId.id
                                                    << "->" << sourceId_.id << " state=" << static_cast<int>(oldState)
                                                    << "->" << static_cast<int>(state_));
}

void Oscillator::clearSource()
{
    OSCIL_LOG(STATE,
              "Oscillator::clearSource: id=" << id_.id << " name=" << name_ << " previousSource=" << sourceId_.id);
    // Preserve configuration but transition to NO_SOURCE
    sourceId_ = SourceId::noSource();
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
