# ADR-009: Schema Migration Fail-Closed

## Status

Accepted

## Context

`OscilState::fromXmlString` is the single ingress for persisted plugin state coming back from a DAW session. Prior to this decision, on schema version mismatch it logged a warning and then assigned `state_ = loadedState` anyway. Three failure modes resulted:

1. **Silent downgrade** -- a session saved by a newer build loaded into an older build would overwrite in-memory state with XML whose fields this build does not understand. The unknown fields were ignored; the known-but-semantically-different fields were accepted at face value.
2. **Silent forward-load of unknown-future data** -- a session with `version="999"` parsed fine and was committed to `state_`, even though no migration logic exists for it.
3. **No central migration surface** -- per-entity migration (`Oscillator::migrateOscillatorState`) was hidden inside `fromValueTree`. There was no place to register a new step without touching multiple call sites.

The audit of `CURRENT_SCHEMA_VERSION` across `OscilState`, `Oscillator`, and `Source` showed the constants have all been static since the initial commit -- the drift that did happen (the `OscillatorState` enum added at Oscillator v2) is handled inside `Oscillator::migrateOscillatorState` using the per-child `schemaVersion` property, so the top-level OscilState schema has had no field drift. Nevertheless, the framework is needed now because future schema changes are expected and the silent-accept behavior is unsafe to keep.

## Decision

A dedicated `oscil::migration` module (`include/core/SchemaMigration.h`, `src/core/SchemaMigration.cpp`) owns all OscilState-level schema migrations. It exposes one entry point:

```cpp
[[nodiscard]] MigrationResult migrateOscilState(juce::ValueTree& state,
                                                int fromVersion,
                                                int toVersion);
```

`MigrationResult` is `Success`, `UnsupportedFromVersion`, `UnsupportedToVersion`, or `DowngradeUnsupported`.

`OscilState::fromXmlString` calls this entry point. On any result other than `Success`, it logs the reason and returns `false` **without** assigning `state_`. The in-memory state is preserved. This is the fail-closed behavior that replaces log-and-continue.

The migration table is:

| From | To | Step       | Notes                                                                                           |
|------|----|-----------|-------------------------------------------------------------------------------------------------|
| 0    | 1  | no-op     | v0 denotes pre-versioned XML (no `version` attribute). Structurally identical to v1.            |
| 1    | 2  | no-op     | No OscilState-level field drift between v1 and v2. Oscillator v1->v2 is handled internally.     |

Both steps stamp the `version` property to their target version so chained calls and `getSchemaVersion()` observe the post-migration version.

Downgrades (`fromVersion > toVersion`) are always rejected. This includes the case where a persisted state claims a version higher than this build's `CURRENT_SCHEMA_VERSION`.

Chain migration is attempted automatically. `migrateOscilState(tree, 0, 2)` walks `v0 -> v1` then `v1 -> v2`.

## Consequences

- A plugin session saved by a newer build loading into an older build now refuses to load and preserves whatever defaults the older build already had. The user sees a fresh session instead of a silently corrupted one.
- Future schema changes register a new `{from, to, step}` entry in `migrationTable()` in `SchemaMigration.cpp`. No changes to `fromXmlString` are required.
- The existing `test_state_persistence_migration.cpp` tests were updated: `MissingSchemaVersion` and `OldSchemaVersion` now expect forward migration to `CURRENT_SCHEMA_VERSION`; `FutureSchemaVersion` now expects `fromXmlString` to return `false` and the state to be preserved.
- Direct `juce::ValueTree` writes that bypass `fromXmlString` (none exist today) would bypass migration. This is an accepted limitation; the single-ingress invariant is enforced by having `fromXmlString` be the only code that parses external XML.

## Alternatives Rejected

- **Keep log-and-continue** -- Rejected as unsafe. Silent state corruption on version mismatch is worse than a refused load.
- **Ad-hoc per-field defaulting** -- The current `getProperty(key, default)` style already does this for missing fields, but it cannot detect semantic changes (e.g., an enum whose meaning changed between versions) and offers no seam for multi-step upgrade logic.
- **Reject anything that is not exactly `CURRENT_SCHEMA_VERSION`** -- Too strict. Users must be able to load v1 sessions in v2 builds. The framework allows this via registered migrations.
- **Use `juce::ValueTree::sync` or similar JUCE built-ins** -- None provide schema-aware migration with rejection semantics; JUCE's model is strictly property-level merge.
