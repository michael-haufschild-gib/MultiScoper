# Folder Structure Proposal

## Snapshot of the Current Layout
- `src/core`, `src/dsp`, `src/rendering`, and `src/ui` mirror the public headers in `include/`, but the boundaries regularly blur (e.g., `src/ui/PluginTestServer.cpp` mixes UI concerns with HTTP test harness logic).
- Pure UI widgets live beside entry points (`src/ui/components/OscilButton.cpp` vs. `src/ui/PluginEditor.cpp`) which makes it hard to isolate reusable UI code or bundle-only surfaces.
- Rendering code mixes foundational objects (`src/rendering/RenderEngine.cpp`) with highly specific effects under the same directory, while some specializations are already tucked under `rendering/effects`, `rendering/materials`, etc., leading to two competing patterns.
- Tooling-specific code (`src/ui/test_server/*` and `PluginTestServer_OLD.cpp`) sits inside the production UI tree, so test harness changes risk leaking into the plugin build.

## Re-organization Goals
1. **Traceability** – Every source file should advertise its purpose and runtime layer via its path.
2. **Feature-locality** – A developer touching "oscillator editing" or "render pipeline" work should find UI + logic neighbors quickly.
3. **Isolation of tooling** – Test servers, adapters, and legacy scaffolding should move out of production targets.
4. **Header/source symmetry** – `include/` and `src/` should expose the same module boundaries so IDE navigation stays predictable.

## Candidate Layouts
### 1. Pure Layered (close to today)
```
src/
  core/
  dsp/
  rendering/
  ui/
```
- **Pros**: Simple mental model, matches many JUCE examples.
- **Cons**: Fails at feature locality; UI folders keep growing monolithically (already 20+ widgets). Cross-cutting workflows (e.g., capture → render → UI) require hopping between folders.

### 2. Full Feature Slices
```
src/
  capture/
  oscillator/
  visualization/
  theming/
```
- **Pros**: Keeps logic + UI + rendering for a feature together.
- **Cons**: Duplicates shared infrastructure (DSP helpers, JUCE adaptors) or forces many `#include` hops back to shared code; harder to share rendering primitives between slices.

### 3. Runtime-Pipeline (Audio → Render → UI)
```
src/
  audio/
  presentation/
  ui/
  tooling/
```
- **Pros**: Makes data flow explicit.
- **Cons**: Still separates tightly coupled UI controllers from their views and does not address feature teams needing vertical slices.

## Recommended Hybrid Layout
Blend feature slices for user-facing modules with shared layers for primitives. Keep rendering, DSP, and UI-kit reusable, but group orchestration logic per feature.

```
src/
  app/
    PluginProcessor.cpp
    PluginEditor.cpp
  platform/
    juce/                # entry-specific glue (adapters, mac/mm files)
  shared/
    core/                # OscilState, InstanceRegistry, SharedCaptureBuffer
    dsp/                 # SignalProcessor, TimingEngine
    rendering/           # Engine, Framebuffer, ShaderRegistry, Effect base classes
    ui_kit/              # Generic widgets (OscilButton, OscilSlider, ColorPicker)
  modules/
    oscillator/          # Oscillator list, dialogs, presenters, color themes
    capture/             # Capture buffer lifecycle + UI status bar
    visualization/       # Waveform components, camera controls, effect chain UI
    layout/              # Pane components, sidebar sections, window layout coordinators
    theme/               # ThemeManager, ThemeEditorComponent, palette state
  tooling/
    test_server/         # HTTP handlers + TestRunner glue
    adapters/            # PluginTestServer.cpp (move out of UI target)
```

### Notes
- Each `modules/<feature>` folder contains `core/`, `ui/`, and `coordinators/` subfolders when needed, so tests can target a single module.
- `shared/rendering` keeps effect implementations under `effects/`, `materials/`, `particles/`, and `shaders/`, but wraps them with an index header (`RenderingPrimitives.h`) for consumers.
- `platform/juce` becomes the home for `AnimationSettings.mm` and any Objective-C++ glue so platform tooling does not leak into cross-platform code.

## Migration Plan
1. **Phase 1 – Introduce directories**: Create `src/app`, `src/shared`, `src/modules`, `src/platform`, `src/tooling` (no file moves yet) and update `CMakeLists.txt` to recognize them.
2. **Phase 2 – Extract tooling**: Move `src/ui/test_server/` and `src/ui/PluginTestServer*.cpp` into `src/tooling/test_server/` and gate the target behind a dedicated library or the existing `test_harness` build.
3. **Phase 3 – Stabilize shared layers**: Relocate `InstanceRegistry.cpp`, `OscilState.cpp`, `SharedCaptureBuffer.cpp`, `SignalProcessor.cpp`, `TimingEngine.cpp`, render primitives, and UI atoms into `src/shared/*`. Ensure headers under `include/` match the new paths.
4. **Phase 4 – Carve feature modules**: For example, move `OscillatorListComponent.cpp`, `OscillatorPresenter.cpp`, dialogs, and toolbar files into `src/modules/oscillator/`. Mirror this for capture, visualization, layout, and theme features as described above.
5. **Phase 5 – Update tests**: Point `tests/test_*` files to the new include paths; co-locate module-specific tests under `tests/modules/<feature>` if desired.
6. **Phase 6 – Clean up legacy**: Delete `PluginTestServer_OLD.cpp`, ensure `test_server/*.bak` files leave the tree, and document each module entry point inside `docs/architecture.md`.

Following this hybrid plan keeps shared logic deduplicated while giving teams vertical ownership of user-facing features. The phased approach lets you move a module at a time without breaking CI.
