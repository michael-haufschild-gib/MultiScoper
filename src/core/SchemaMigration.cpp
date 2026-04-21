/*
    MultiScoper - Schema Migration Framework Implementation

    The framework exists so we can register stepwise migrations as the
    persisted format evolves. No migration steps are registered today —
    only the current schema version (v3) is accepted. Older saves are
    rejected at load time.
*/

#include "core/SchemaMigration.h"

#include "core/MultiScoperLog.h"
#include "core/MultiScoperState.h"

#include <array>
#include <functional>
#include <optional>

namespace multiscoper::migration
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

/// Canonical current schema version. Must match MultiScoperState::CURRENT_SCHEMA_VERSION —
/// the static_assert below prevents drift at compile time.
constexpr int kCurrentVersion = 3;
static_assert(kCurrentVersion == MultiScoperState::CURRENT_SCHEMA_VERSION,
              "SchemaMigration::kCurrentVersion must match MultiScoperState::CURRENT_SCHEMA_VERSION");

const std::array<MigrationEntry, 0>& migrationTable()
{
    static const std::array<MigrationEntry, 0> table{};
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
        MULTISCOPER_LOG(STATE, "SchemaMigration: reject downgrade from v" << fromVersion << " to v" << toVersion);
        return MigrationResult::DowngradeUnsupported;
    }
    if (toVersion > highestTo)
    {
        MULTISCOPER_LOG(STATE, "SchemaMigration: toVersion v" << toVersion << " exceeds highest known v" << highestTo);
        return MigrationResult::UnsupportedToVersion;
    }
    if (fromVersion < lowestFrom)
    {
        MULTISCOPER_LOG(STATE,
                        "SchemaMigration: fromVersion v" << fromVersion << " below lowest known v" << lowestFrom);
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

int lowestSupportedFromVersion() noexcept { return kCurrentVersion; }

int highestSupportedToVersion() noexcept { return kCurrentVersion; }

MigrationResult migrateMultiScoperState(juce::ValueTree& state, int fromVersion, int toVersion)
{
    if (fromVersion == toVersion && fromVersion == kCurrentVersion)
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
            MULTISCOPER_LOG(STATE, "SchemaMigration: no step from v" << current << " toward v" << toVersion);
            return MigrationResult::UnsupportedFromVersion;
        }

        const MigrationResult result = entry->step(state);
        if (result != MigrationResult::Success)
            return result;

        current = entry->toVersion;
    }

    return MigrationResult::Success;
}

} // namespace multiscoper::migration
