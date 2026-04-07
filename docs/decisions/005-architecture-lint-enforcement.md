# ADR-005: Architecture Boundaries Enforced by Lint

## Status
Accepted

## Context
The layered architecture (core → rendering → ui → plugin) is the project's primary defense against spaghetti dependencies. Without enforcement, new code naturally gravitates toward convenient cross-layer includes, eroding the architecture over time.

## Decision
Architecture boundaries are enforced by `scripts/architecture_lint.py`, which scans `#include` directives in all source and header files. The rules are:
- `core/` must NOT include `ui/`, `rendering/`, `plugin/`, `tools/`
- `rendering/` must NOT include `ui/`, `plugin/`, `tools/`
- `ui/` must NOT include `plugin/`, `tools/`

This runs in three places:
1. **Pre-commit hook** -- blocks commits with violations.
2. **CMake lint target** (`cmake --build --preset dev --target lint`) -- runs during development.
3. **CI workflow** -- blocks PRs with violations.

## Consequences
- Developers cannot accidentally add upward dependencies.
- The lint is fast (scans all 300+ files in <1 second).
- Exceptions require modifying the lint rules, which is deliberate and reviewable.
- Communication between layers uses interfaces (`IInstanceRegistry`, `IThemeService`, `IAudioDataProvider`) and dependency injection.

## Alternatives Rejected
- **Manual code review** -- unreliable as sole enforcement mechanism.
- **C++ modules** -- not yet supported by JUCE or cross-platform build toolchains.
- **CMake INTERFACE libraries per layer** -- provides compile-time enforcement but is harder to configure and doesn't catch include-path violations in headers.
