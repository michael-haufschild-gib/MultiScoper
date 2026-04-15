# ADR-006: Dependency Pinning Policy

## Status

Accepted

## Context

Oscil uses CPM.cmake (`cmake/CPM.cmake`) to fetch source dependencies at configure time. Each `CPMAddPackage` call specifies a `GIT_TAG`, which CPM resolves to a commit during the first configure. If the reference is a branch (e.g., `main`, `origin/main`), the resolved commit drifts — different developers and different CI runs can produce different build outputs from identical source trees. This is a reproducibility defect.

Prior to this ADR, one dependency was pinned to `origin/main`, violating reproducibility. All other dependencies were pinned to tagged releases.

## Decision

Every `CPMAddPackage` in `CMakeLists.txt` must use one of the following reference types:

1. **Semantic version tag** (e.g., `8.0.12`, `v3.12.0`) — the preferred form for upstreams that publish tagged releases. Tags on major OSS projects (JUCE, cpp-httplib, nlohmann/json) are immutable in practice; treating them as stable references is acceptable.
2. **Commit SHA** (40-char hex) — required when the upstream does not publish stable tags, or when we need to pin to a state between tags for a specific fix. Must be accompanied by a code comment stating the pin date and rationale.

Branch references (`main`, `origin/main`, `develop`) are prohibited.

## Current pins

| Dependency             | Reference                                      | Rationale                                     |
|------------------------|------------------------------------------------|-----------------------------------------------|
| JUCE                   | `8.0.12` (tag)                                 | Upstream publishes stable tags                |
| cpp-httplib            | `v0.38.0` (tag)                                | Upstream publishes stable tags                |
| nlohmann/json          | `v3.12.0` (tag)                                | Upstream publishes stable tags                |
| clap-juce-extensions   | `e1f67893cc409a40c1154fa2e78c97046da24ce0` (SHA) | Upstream tags 0.24–0.26 are incompatible with JUCE 8.0.12; main tracks JUCE 8. |

## Review cadence

Pins are reviewed quarterly. A review consists of:

1. Checking each upstream for new releases, security advisories, or critical fixes.
2. Producing a pinning-review PR that either bumps a reference or records "no change" with a note.
3. Running the full CI matrix against the bumped reference before merging.

The review is tracked as a recurring task in the project task list.

## Consequences

- Different checkouts of the repository always resolve to the same dependency commits.
- Upgrades are deliberate and reviewable, not silent.
- The `CMakeLists.txt` block around each pin documents the rationale for the chosen reference type.
- A CI-side check (future work, see task: _CPM reference linter_) would prevent regressions by parsing `CPMAddPackage` blocks and asserting no branch references exist.

## Alternatives rejected

- **Pin everything to SHAs, including tagged releases.** Advised against by the advisor consultation — tags on major OSS projects are stable and SHA pins harm readability for no reproducibility gain. Reserved for cases without stable tags.
- **Use CPM's `VERSION` field with semver ranges.** Introduces version drift by design.
- **Vendor dependencies into the tree.** Maintainership burden exceeds reproducibility benefit at this project scale.
