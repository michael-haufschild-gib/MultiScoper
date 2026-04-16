/*
    Oscil - Schema Migration Framework Implementation

    Migration table (current):
      v0 -> v1: no-op. v0 denotes a pre-versioned OscilState XML (no "version"
                attribute). Structurally identical to v1; no field drift.
      v1 -> v2: no-op at the OscilState level. Oscillator handles its own
                v1 -> v2 migration internally (OscillatorState field synthesis).

    Both migrations stamp the "version" property to their target version so that
    chained calls correctly observe the intermediate state and downstream code
    (e.g. getSchemaVersion()) reports the post-migration version.
*/

#include "core/SchemaMigration.h"

#include "core/OscilLog.h"
#include "core/OscilState.h"

#include <array>
#include <functional>
#include <optional>

namespace oscil::migration
{

namespace
{

using MigrationStep = std::function<MigrationResult(juce::ValueTree&)>;

struct MigrationEntry
{
    int fromVersion;
    int toVersion;
    MigrationStep step;
};

/// Stamp the version property on the state node.
void stampVersion(juce::ValueTree& state, int version) { state.setProperty(StateIds::Version, version, nullptr); }

/// v0 -> v1: pre-versioned XML had no "version" attribute. Structurally
/// identical to v1. Stamp version property; no field changes.
MigrationResult migrateV0toV1(juce::ValueTree& state)
{
    stampVersion(state, 1);
    return MigrationResult::Success;
}

/// v1 -> v2: no OscilState-level schema change. The v2 addition
/// (OscillatorState enum on child Oscillator nodes) is handled inside
/// Oscillator::fromValueTree via its own migrateOscillatorState helper.
MigrationResult migrateV1toV2(juce::ValueTree& state)
{
    stampVersion(state, 2);
    return MigrationResult::Success;
}

const std::array<MigrationEntry, 2>& migrationTable()
{
    static const std::array<MigrationEntry, 2> table{{
        {.fromVersion = 0, .toVersion = 1, .step = &migrateV0toV1},
        {.fromVersion = 1, .toVersion = 2, .step = &migrateV1toV2},
    }};
    return table;
}

/// Find a single-step migration from `from` that advances toward `to`.
/// Returns nullptr if no applicable step registered.
const MigrationEntry* findStep(int from, int to)
{
    for (const auto& entry : migrationTable())
    {
        if (entry.fromVersion == from && entry.toVersion <= to)
            return &entry;
    }
    return nullptr;
}

/// Validate requested range. Returns an error code if unsupported, nullopt if
/// the caller should proceed with stepwise migration.
std::optional<MigrationResult> validateRange(int fromVersion, int toVersion, int lowestFrom, int highestTo)
{
    if (fromVersion > toVersion)
    {
        OSCIL_LOG(STATE, "SchemaMigration: reject downgrade from v" << fromVersion << " to v" << toVersion);
        return MigrationResult::DowngradeUnsupported;
    }
    if (toVersion > highestTo)
    {
        OSCIL_LOG(STATE, "SchemaMigration: toVersion v" << toVersion << " exceeds highest known v" << highestTo);
        return MigrationResult::UnsupportedToVersion;
    }
    if (fromVersion < lowestFrom)
    {
        OSCIL_LOG(STATE, "SchemaMigration: fromVersion v" << fromVersion << " below lowest known v" << lowestFrom);
        return MigrationResult::UnsupportedFromVersion;
    }
    return std::nullopt;
}

} // namespace

const char* migrationResultToString(MigrationResult result) noexcept
{
    switch (result)
    {
        case MigrationResult::Success:
            return "Success";
        case MigrationResult::UnsupportedFromVersion:
            return "UnsupportedFromVersion";
        case MigrationResult::UnsupportedToVersion:
            return "UnsupportedToVersion";
        case MigrationResult::DowngradeUnsupported:
            return "DowngradeUnsupported";
    }
    return "Unknown";
}

int lowestSupportedFromVersion() noexcept
{
    int lowest = migrationTable().front().fromVersion;
    for (const auto& entry : migrationTable())
        lowest = std::min(lowest, entry.fromVersion);
    return lowest;
}

int highestSupportedToVersion() noexcept
{
    int highest = migrationTable().front().toVersion;
    for (const auto& entry : migrationTable())
        highest = std::max(highest, entry.toVersion);
    return highest;
}

MigrationResult migrateOscilState(juce::ValueTree& state, int fromVersion, int toVersion)
{
    if (fromVersion == toVersion)
    {
        stampVersion(state, toVersion);
        return MigrationResult::Success;
    }

    if (const auto err =
            validateRange(fromVersion, toVersion, lowestSupportedFromVersion(), highestSupportedToVersion()))
        return *err;

    int current = fromVersion;
    while (current < toVersion)
    {
        const MigrationEntry* entry = findStep(current, toVersion);
        if (entry == nullptr)
        {
            OSCIL_LOG(STATE, "SchemaMigration: no step from v" << current << " toward v" << toVersion);
            return MigrationResult::UnsupportedFromVersion;
        }

        const MigrationResult result = entry->step(state);
        if (result != MigrationResult::Success)
            return result;

        current = entry->toVersion;
    }

    return MigrationResult::Success;
}

} // namespace oscil::migration
