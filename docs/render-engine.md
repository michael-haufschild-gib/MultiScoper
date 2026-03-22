# Render Engine Architecture

The render engine provides GPU-accelerated waveform visualization with per-waveform post-processing effects.

## Architecture

```
WaveformGLRenderer (juce::OpenGLRenderer)
  │
  ├── collectWaveformsToRender()  ← gathers data under SpinLock
  │
  └── renderWithEngine()          ← under WriteLock
        │
        └── RenderEngine
              ├── RenderBootstrapper   (composite/blit shaders, fullscreen quad)
              ├── EffectPipeline       (FBO pool, scene FBO, effect chain)
              │     ├── FramebufferPool
              │     ├── EffectChain    (ordered list of post-process steps)
              │     └── PostProcessEffect instances (Bloom, Trails, etc.)
              ├── WaveformPass         (shader registry, grid renderer)
              │     ├── ShaderRegistry (factory for WaveformShader instances)
              │     └── GridRenderer
              └── WaveformRenderState  (per-waveform: visual config, history FBO, timing)
```

## Thread Safety

| Thread | Role | Lock |
|-|-|-|
| Message | Updates waveform data, config | `dataLock_` (SpinLock) for WaveformRenderData; `engineLock_` (ReadLock) for RenderEngine config |
| OpenGL | Rendering | `engineLock_` (WriteLock) for full render pass; no internal SpinLock needed during render |

- `WaveformGLRenderer::updateWaveform()` — message thread writes under `dataLock_`
- `WaveformGLRenderer::renderOpenGL()` — GL thread reads under `dataLock_`, renders under `engineLock_` WriteLock
- `WaveformGLRenderer::performRenderEngineOperation()` — message thread accesses RenderEngine under `engineLock_` ReadLock

## Per-Frame Render Flow

1. `renderOpenGL()` — compute delta time, resize if needed
2. `collectWaveformsToRender()` — snapshot waveform data under SpinLock
3. `renderWithEngine()` — under WriteLock:
   a. `syncWaveforms(activeIds)` — remove stale waveform states
   b. `beginFrame(dt)` — clear scene FBO
   c. For each waveform:
      - `registerWaveform` / `setWaveformConfig` if needed
      - `renderWaveform(data)`:
        - `prepareRender()` — update timing, disable depth test
        - `renderGrid()` — if grid enabled, render grid lines in pane viewport
        - `renderWaveformLayer()`:
          - Render geometry to waveformFBO via WaveformPass
          - Apply per-waveform post-processing via EffectPipeline
          - Composite result onto sceneFBO
   d. `endFrame()` — apply global effects, blit scene to screen

## Shader System

Four built-in shaders, each with GPU (OpenGL) and software (juce::Graphics) paths:

| Shader | ID | Description |
|-|-|-|
| BasicShader | `basic` | Line with glow passes |
| NeonGlowShader | `neon_glow` | Neon glow effect |
| GradientFillShader | `gradient_fill` | Area fill under waveform |
| DualOutlineShader | `dual_outline` | Double concentric outlines |

Shaders are lazy-compiled: only `basic` compiles at startup; others compile on first use.

### Adding a Shader

1. Create `include/rendering/shaders/MyShader.h` inheriting `WaveformShader`
2. Create `src/rendering/shaders/MyShader.cpp` with `compile()`, `render()`, `renderSoftware()`
3. Add GLSL source files to `resources/shaders/` and register in `CMakeLists.txt` binary data
4. Register in `ShaderRegistry::registerBuiltInShaders()`
5. Add to `ShaderType` enum and `shaderTypeToId()`/`idToShaderType()`
6. Add source files to `cmake/Sources.cmake`

## Post-Processing Effects

11 effects, each implementing `PostProcessEffect`:

| Effect | Settings struct | Description |
|-|-|-|
| Bloom | `BloomSettings` | Brightness extraction + multi-pass blur |
| RadialBlur | `RadialBlurSettings` | Zoom/radial blur |
| TiltShift | `TiltShiftSettings` | Fake depth of field |
| ColorGrade | `ColorGradeSettings` | Brightness, contrast, saturation, temperature |
| ChromaticAberration | `ChromaticAberrationSettings` | RGB channel offset |
| Scanlines | `ScanlineSettings` | CRT-style scanlines |
| Distortion | `DistortionSettings` | Wave distortion |
| Vignette | `VignetteSettings` | Edge darkening |
| FilmGrain | `FilmGrainSettings` | Animated noise |
| Glitch | `GlitchSettings` | Digital glitch blocks |
| Trails | `TrailSettings` | Persistence/decay via history FBO |

Effects are lazy-compiled and configured per-frame via `EffectChain`.

## Visual Configuration

`VisualConfiguration` aggregates all per-waveform visual settings:
- Shader type selection
- All 11 post-processing effect settings
- Compositing blend mode and opacity
- Preset identifier

Presets: `default`, `vector_scope`. Serialized via `toValueTree()`/`fromValueTree()`.

## Framebuffer Management

- `FramebufferPool` — owns waveformFBO (per-waveform scratch) and pingPong FBOs (effect chain)
- `sceneFBO` — accumulates all composited waveforms
- Per-waveform `historyFBO` — for trails effect persistence
- All FBOs resize on window resize

## Quality Levels

`QualityLevel` enum controls effect fidelity (bloom iterations, blur samples, etc.).
