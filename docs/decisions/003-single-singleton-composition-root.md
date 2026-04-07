# ADR-003: Single Singleton (PluginFactory) as Composition Root

## Status
Accepted

## Context
JUCE's plugin hosting model calls a global C function `createPluginFilter()` to instantiate the processor. This function has no parameters -- there is no way to inject dependencies from the host. Meanwhile, all core services (InstanceRegistry, ThemeManager, ShaderRegistry, MemoryBudgetManager, GlobalPreferences, PresetManager) must be shared across plugin instances running in the same host process.

## Options Considered
1. **Multiple singletons** -- Each service has its own `getInstance()`. Simple but creates hidden coupling, makes testing harder, and order of construction/destruction is fragile.
2. **Single composition root singleton** -- One `PluginFactory` owns all services and provides them via dependency injection to `PluginProcessor`. Only `createPluginFilter()` uses the singleton; all other code receives dependencies through constructors.
3. **Pure DI with no singletons** -- Not possible due to `createPluginFilter()` having no parameters.

## Decision
Single composition root (`PluginFactory`). It is the minimum necessary singleton to bridge the JUCE hosting API. All services are created in `PluginFactory`'s constructor and injected into processors via `PluginProcessorConfig`.

## Consequences
- Only `createPluginFilter()` and test fixtures call `PluginFactory::getInstance()`.
- All other code receives services via constructor parameters -- fully testable without singletons.
- `PluginFactory::setInstance()` allows tests to substitute a mock factory (via atomic pointer swap).
- Previously existing singletons (ThemeManager, InstanceRegistry, ShaderRegistry, etc.) were all removed and migrated to DI. See `docs/archive/plan-singleton-removal.md` for the migration history.

## Verification
- `tests/test_plugin_processor_lifecycle.cpp`: processors receive services via DI.
- `tests/test_plugin_editor.cpp`: editor tests use mock factory.
