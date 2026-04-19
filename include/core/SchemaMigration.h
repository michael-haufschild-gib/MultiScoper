/*
    MultiScoper - Schema Migration Framework
    Fail-closed migration of persisted MultiScoperState ValueTree across schema versions.

    Contract:
    - migrateMultiScoperState(state, fromVersion, toVersion) returns MigrationResult::Success
      on successful in-place migration. Any other result indicates the caller MUST
      reject the state and not commit it to live memory.
    - Chain migration is attempted when no direct (from, to) pair is registered:
      e.g. v0 -> v2 is satisfied by running v0 -> v1 then v1 -> v2.
    - Downgrade (from > to) is always rejected (DowngradeUnsupported).
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace multiscoper::migration
{

/**
 * Outcome of a schema migration attempt.
 *
 * Success: The ValueTree was mutated in place and is safe to commit.
 * UnsupportedFromVersion: fromVersion is below the lowest known migration start,
 *                         or no chain of registered migrations can reach toVersion.
 * UnsupportedToVersion:   toVersion is above the highest known migration end.
 * DowngradeUnsupported:   fromVersion > toVersion; we refuse to lossy-drop data.
 */
enum class MigrationResult : int
{
    Success = 0,
    UnsupportedFromVersion = 1,
    UnsupportedToVersion = 2,
    DowngradeUnsupported = 3
};

/// Convert MigrationResult to a short diagnostic string.
[[nodiscard]] const char* migrationResultToString(MigrationResult result) noexcept;

/**
 * Migrate an MultiScoperState ValueTree from fromVersion to toVersion.
 *
 * On Success, state is mutated in place and the caller may commit it.
 * On any other result, state may be partially mutated and MUST be discarded.
 */
[[nodiscard]] MigrationResult migrateMultiScoperState(juce::ValueTree& state, int fromVersion, int toVersion);

/// Lowest schema version the migrator knows how to read.
[[nodiscard]] int lowestSupportedFromVersion() noexcept;

/// Highest schema version the migrator knows how to produce.
[[nodiscard]] int highestSupportedToVersion() noexcept;

} // namespace multiscoper::migration
