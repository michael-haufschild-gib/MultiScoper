# ADR-014: Signal-Gated OpenGL Repainting

## Status

Accepted

## Context

MultiScoper is designed to run as many simultaneous plugin instances as a DAW
project contains tracks. Real sessions routinely carry 8–16 open editors.
Each editor owns a `juce::OpenGLContext`; left in the JUCE default of
`setContinuousRepainting(true)`, every context wakes on each VSync tick
(typically 60 Hz) and redraws the waveform — even when the input is silent
and pixel output is identical to the previous frame.

Measured cost: with 16 editors open and silent, continuous repainting
saturates roughly one full CPU core on an M1-class machine and keeps the
GPU in its active state, preventing it from entering the low-power sleep
states that dominate battery life. This is a quality-of-life regression
users notice when they leave MultiScoper on a bus track and walk away.

## Decision

Disable `context_.setContinuousRepainting(true)`. Drive OpenGL redraws
explicitly in two situations:

1. **Signal activity** — `OpenGLLifecycleManager::updateWaveformData()`
   returns a `bool anySignal` flag derived from
   `WaveformComponent::getPeakLevel() > kSilenceEpsilon` across every
   visible waveform. `GpuRenderCoordinator::updateRendering()` triggers a
   repaint on each signal-bearing tick, plus a post-silence tail of
   `kPostSilenceFrames` (30 frames ≈ 0.5 s at 60 Hz) so fades/decays still
   render out smoothly after the source goes silent.

2. **Non-signal scene-state changes** — `GpuRenderCoordinator::forceRepaint()`
   explicitly triggers one repaint. Callers are responsible for invoking
   it whenever a change would be invisible to the signal-activity heuristic:

   | Change | Caller |
   |---|---|
   | Theme palette change | `MultiScoperPluginEditor::onThemeChanged` |
   | Oscillator property change (color, mode, visibility) | `OscillatorPanelController::applyOscillatorPropertyChange` |
   | Panel refresh (add/remove oscillator, reorder, pane layout) | `OscillatorPanelController::runRefreshIteration` |
   | Grid visibility toggle | `MultiScoperPluginEditor::showGridChanged` |
   | Auto-scale toggle | `MultiScoperPluginEditor::autoScaleChanged` |
   | Time interval / sync mode / timing grid change | `MultiScoperPluginEditor::setDisplaySamplesForAllPanes` and `setGridConfigForAllPanes` (called from `TimingEngineListenerAdapter::updateDisplayAndGrid`) |
   | Editor resize | `MultiScoperPluginEditor::resized` |
   | GPU rendering mode toggled on | `GpuRenderCoordinator::setGpuRenderingEnabled` (disabled→enabled transition) |

## Rules

1. **Never re-enable `setContinuousRepainting(true)`.** The only site that
   touches this API is `OpenGLLifecycleManager::setGpuRenderingEnabled`;
   size-lint + architecture-lint keep calls off the rest of the codebase.

2. **Every new UI action that alters scene state must either:**
   (a) go through `OscillatorPanelController::refreshPanels()` (which
       terminates in a `forceRepaint()`), or
   (b) call `GpuRenderCoordinator::forceRepaint()` explicitly.

   The review contract: if you add a handler that mutates what the GL
   waveform draws — colors, geometry, visibility, timing, layout — and the
   change does not produce audio, you owe a `forceRepaint()`.

3. **Never call `forceRepaint()` from the audio thread.** It is a
   message-thread operation (routes through `juce::OpenGLContext::triggerRepaint`,
   which posts to the message queue). Audio-thread callers must hop via
   `juce::MessageManager::callAsync` first.

4. **Treat the post-silence tail (`kPostSilenceFrames`) as private.** It
   is deliberately small (half a second) so idle editors fall to zero GPU
   cost quickly; lengthening it would regress the original motivation.
   Any fade/decay longer than 0.5 s must drive its own repaints via
   forceRepaint() or a dedicated timer.

## Failure modes and their symptoms

| Symptom | Likely cause |
|---|---|
| Waveform keeps old theme colors until audio resumes | New UI path mutated state without calling `forceRepaint()` — add the call at the mutation site. |
| Editor blank on open with silent audio | Initial paint not seeded. `GpuRenderCoordinator::setGpuRenderingEnabled(true)` already calls `forceRepaint()` on transition; verify that path is hit, and that the editor's `attachTo` succeeded (check `context_.isAttached()`). |
| CPU regresses with many editors open | Check that `setContinuousRepainting(false)` is still in `setGpuRenderingEnabled`; check no rogue `forceRepaint()` is being called per-frame in a hot path. |
| Fades/envelope decays freeze before finishing | `kPostSilenceFrames` tail is too short for the effect. Either extend the effect's own redraw trigger, or accept the limit. |

## Verification

- `tests/test_multiscoper_look_and_feel.cpp` — theme-token propagation into
  JUCE widget colour IDs, exercised on theme change.
- `tests/test_oscillator_panel_controller.cpp` — property-change dispatch
  path (uses a real `GpuRenderCoordinator` wired to a `TestEditor`).
- Manual/integration: open 16 instances with no audio, observe CPU; a
  regression of this decision would reappear as ~1 core of idle CPU.

## Alternatives considered

1. **Keep continuous repainting on.** Rejected: the original motivation.
2. **Drive repaints from `juce::OpenGLContext::setContinuousRepainting(true)` only when signal is present.** Rejected: no JUCE API toggles this cleanly per-frame, and the on/off churn itself schedules message-thread work.
3. **Push all redraw decisions into `updateRendering()` and have every state mutation dirty an atomic flag that updateRendering polls.** Rejected in favor of explicit `forceRepaint()`: fewer moving parts, clearer contract in code review, no shared state between mutation site and render tick.
4. **Use `juce::Component::repaint()` on the editor root to propagate to the GL context.** `Component::repaint()` invalidates JUCE's software paint layer but does not schedule a GL redraw when `setContinuousRepainting` is off; the GL context is a distinct surface.
