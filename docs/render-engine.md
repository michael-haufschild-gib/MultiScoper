# Post-Processing Effects Requirements for Oscilloscope Plugin

This document defines **post-processing effects** to add on top of the scene render (waveform + UI) in your JUCE + OpenGL oscilloscope plugin.
It’s written as a **requirements spec** – focused on behavior, parameters, and performance tiers – so you can implement effects incrementally.

The assumption is that you already have:

- A `RenderEngine` with a multi-pass pipeline (scene → post-FX → screen).
- A `FramebufferManager` with `scene`, `ping`, `pong`, and optional `history` buffers.
- Shader metadata + visual presets (Eco / Normal / Ultra).

---

## 1. Global Design Constraints

All post-processing effects **must**:

1. Operate on a **fullscreen quad** using a source texture and writing to a target FBO.
2. Be **individually switchable** based on:
   - Global visual preset (Eco / Normal / Ultra).
   - Per-shader metadata (`allowPostProcessing`, `allowBloom`, etc.).
3. Support a clear **performance profile**:
   - *Cheap*: safe for Eco.
   - *Medium*: Normal/Ultra only.
   - *Expensive*: Ultra only, optional.
4. Be **order-aware** – certain effects should always happen before/after others.
5. Use a **parameter block** per effect that can be exposed to the plugin’s UI.

Each effect is described below with:

- **Purpose / Visual Goals**
- **Inputs / Outputs**
- **Parameters**
- **Audio Reactivity Hooks**
- **Performance Notes**
- **Default Usage per Quality Level**

---

## 2. Effect #1 – Bloom / Glow

### Purpose / Visual Goals

Make bright parts of the waveform and UI **glow** and feel “energetic” and “AAA”.
Should emphasize transients and high-amplitude regions without washing out the image.

### Inputs / Outputs

- **Input texture**: Previous pass (usually `sceneFbo.color` or ping/pong).
- **Output texture**: Other ping/pong FBO.
- **Internal buffers**:
  - One or more downsampled textures for blur passes (1–3 levels).

### Parameters

- `threshold` (float, 0.0–5.0, default ~1.0)
  Luminance or intensity threshold above which pixels contribute to bloom.

- `softKnee` (float, 0.0–1.0, default ~0.5)
  Smooth transition around the threshold.

- `intensity` (float, 0.0–3.0, default ~1.0)
  Overall strength of bloom added back to the scene.

- `radius` (float, 0.5–5.0, default ~2.0)
  Controls blur radius; mapped to number of blur taps / downsample levels.

- `maxIterations` (int, 1–4, default 2)
  Number of blur iterations (affects quality vs performance).

- `qualityLevelOverride` (optional enum)
  Allow lowering quality at Eco (e.g. fewer iterations).

### Audio Reactivity Hooks

- **Amplitude / RMS** → modulate `intensity` subtly (±10–20%).
- **Transient detector** → short spikes in `intensity` on strong hits.
- Optionally **band-limited energy** (high frequencies) → add more bloom for “sparkly” content.

### Performance Notes

- Use **separable blur** (horizontal + vertical) to reduce cost.
- Downsample to 0.25–0.5× resolution for blur.
- Bloom should be **Medium** cost; disabled or simplified at Eco.

### Default Usage

- **Eco**: OFF (or single-level, low-intensity).
- **Normal**: ON, moderate radius and intensity.
- **Ultra**: ON, full multi-level blur, more subtle but higher quality.

---

## 3. Effect #2 – Color Grading & Tonemapping

### Purpose / Visual Goals

Give a strong, cohesive **color mood** to the oscilloscope (e.g. cold sci-fi, warm analog, cyberpunk), and apply gentle tonemapping so highlights roll off nicely.

### Inputs / Outputs

- **Input**: Previous post-FX texture.
- **Output**: Ping/pong or final screen, depending on pipeline.
- Could be implemented via:
  - Simple curves (contrast, saturation, tint, temperature), or
  - 3D LUT (more advanced, optional).

### Parameters

- `exposure` (float, -3.0–+3.0, default 0.0)
  Pre-tonemap multiplier.

- `contrast` (float, 0.5–2.0, default 1.0)

- `saturation` (float, 0.0–2.0, default 1.0)

- `temperature` (float, -1.0–1.0, default 0.0)
  Negative = cooler, positive = warmer.

- `tint` (float, -1.0–1.0, default 0.0)

- `mode` (enum: `Simple`, `Filmic`, `LUT`)
  - `Simple`: linear + curve.
  - `Filmic`: ACES-like tonemapping curve.
  - `LUT`: uses a 3D LUT texture (Ultra only).

- `presetId` (string / int)
  References a saved color grade preset (e.g., “Neon Lab”, “Analog Amber”).

### Audio Reactivity Hooks

- Light modulation only; avoid nausea:
  - **Low-frequency energy** → tiny shifts in `temperature` (e.g. warmer on strong bass).
  - **Overall RMS** → slight contrast boost at high volume.

### Performance Notes

- **Simple & Filmic modes** are **Cheap** (1 pass, few ops).
- LUT-based grading is **Medium** and should be **Ultra only**.

### Default Usage

- **Eco**: ON, `Simple` mode only.
- **Normal**: ON, `Filmic` or `Simple`.
- **Ultra**: ON, optionally `LUT` mode for premium presets.

---

## 4. Effect #3 – Vignette

### Purpose / Visual Goals

Subtle darkening towards the edges to **draw focus to the center** and reduce visual noise from the borders.

### Inputs / Outputs

- **Input**: Previous pass.
- **Output**: Next ping/pong or final buffer.
- No additional textures required.

### Parameters

- `strength` (float, 0.0–1.0, default ~0.3)
  Overall darkening at corners.

- `radius` (float, 0.5–1.5, default ~1.0)
  How far towards center the vignette reaches.

- `softness` (float, 0.1–1.0, default ~0.5)

### Audio Reactivity Hooks

- Optional and very subtle; e.g. modulate `strength` slightly with RMS.

### Performance Notes

- Extremely **Cheap**; basically a handful of math ops per pixel.
- Safe for all quality levels.

### Default Usage

- **Eco**: ON (very subtle).
- **Normal**: ON.
- **Ultra**: ON, tuned to match preset’s mood.

---

## 5. Effect #4 – Film Grain / Noise

### Purpose / Visual Goals

Add very subtle **film grain** or digital noise to break up banding, avoid “too clean / too CG” look, and reduce AI-looking uniformity.

### Inputs / Outputs

- **Input**: Previous pass.
- **Output**: Next buffer.
- Optional **blue noise** or pseudo-random texture, or purely procedural noise using UV + time.

### Parameters

- `intensity` (float, 0.0–1.0, default ~0.15)
  Grain contrast.

- `size` (float, 0.5–3.0, default ~1.0)
  Scales grain spatial frequency.

- `animated` (bool, default true)
  Whether noise is temporally changing (`time`-based).

- `colorMode` (enum: `Monochrome`, `RGB`)
  Monochrome grain is less distracting.

### Audio Reactivity Hooks

- Optional: on high transients, grain intensity bumps slightly to feel more energetic.

### Performance Notes

- **Cheap** if purely procedural.
- Slightly more cost if sampling a noise texture.

### Default Usage

- **Eco**: OFF or very low.
- **Normal**: ON, subtle.
- **Ultra**: ON, slightly richer.

---

## 6. Effect #5 – Chromatic Aberration

### Purpose / Visual Goals

Subtle lens-style color separation towards the screen edges to add **camera/lens character** and game-like polish.

### Inputs / Outputs

- **Input**: Previous pass.
- **Output**: Next buffer.
- No extra textures; implemented via offset UV sampling per color channel.

### Parameters

- `intensity` (float, 0.0–1.0, default ~0.15)
  Magnitude of per-channel offset at screen edges.

- `centerRadius` (float, 0.0–1.0, default ~0.3)
  Radius around center with no CA; effect grows beyond this.

### Audio Reactivity Hooks

- On strong transients, CA may *very slightly* increase for 1–2 frames for a punchy feel.
  Keep subtle to avoid motion sickness.

### Performance Notes

- **Medium** cost (extra samples per pixel).
- Only suitable for **Normal / Ultra** quality.

### Default Usage

- **Eco**: OFF.
- **Normal**: ON, low intensity.
- **Ultra**: ON, moderate intensity.

---

## 7. Effect #6 – Trails / Afterglow (Feedback Buffer)

### Purpose / Visual Goals

Create **temporal persistence** (trails/afterglow) so the waveform leaves a soft trace, enhancing visual richness and convey motion over time.

### Inputs / Outputs

- **Inputs**:
  - Current frame (e.g. `sceneFbo.color`).
  - Previous accumulated frame (`historyFbo.color`).

- **Output**:
  - Updated history buffer (written to `historyFbo`).
  - Optionally a composite output to ping/pong for further passes.

### Parameters

- `decay` (float, 0.7–0.99, default ~0.9)
  How quickly trails fade; lower = fast fade, higher = long trails.

- `intensity` (float, 0.0–2.0, default 1.0)
  Blend strength between current and history.

- `mode` (enum: `Additive`, `Max`, `Lerp`)
  Different composition styles.

### Audio Reactivity Hooks

- **RMS / loudness** → lower `decay` at high volumes (sharper, less smeary).
- **Quiet segments** → higher `decay` for dreamy long trails.
- **Transient detector** → momentarily reduce trail strength to keep transients crisp.

### Performance Notes

- **Medium** cost: typically 2 texture reads and simple math per pixel.
- Requires maintaining a persistent `historyFbo` across frames.

### Default Usage

- **Eco**: OFF.
- **Normal**: ON (conservative decay).
- **Ultra**: ON (stylish long trails by default).

---

## 8. Effect #7 – Scanlines / CRT Mask (Subtle)

### Purpose / Visual Goals

Optional stylistic effect to evoke **retro display / CRT** aesthetics, especially for certain presets (e.g. “Vintage Scope”, “CRT Lab”).

### Inputs / Outputs

- **Input**: Previous pass.
- **Output**: Next buffer or final.
- Could be done purely procedurally (sinusoidal modulation, row/column masks).

### Parameters

- `strength` (float, 0.0–1.0, default ~0.2)
  How visible the scanlines/mask are.

- `lineDensity` (float, 0.5–4.0, default ~1.0)
  Number of lines per unit distance (adjusted for resolution).

- `mode` (enum: `Scanlines`, `ApertureGrille`, `ShadowMask`)
  Different CRT-inspired looks.

### Audio Reactivity Hooks

- Typically none; this should be stable and not flicker.

### Performance Notes

- **Cheap** (basic math per pixel).
- Optional from a design perspective; can be preset-specific.

### Default Usage

- **Eco**: OFF.
- **Normal**: OFF by default, ON for CRT-themed presets.
- **Ultra**: Same as Normal, but with higher fidelity masks.

---

## 9. Effect #8 – Distortion / Heat Haze (Optional)

### Purpose / Visual Goals

Introduce subtle **warping/distortion** to the image, like heat haze or refractive energy fields around the waveform, for certain “experimental” presets.

### Inputs / Outputs

- **Input**: Previous buffer.
- **Output**: Next buffer.
- May require:
  - A distortion map (noise texture), or
  - Procedural noise based on UV + time.

### Parameters

- `intensity` (float, 0.0–1.0, default ~0.0–0.2)
  Overall distortion scale.

- `frequency` (float, 0.5–5.0, default 1.0)
  Spatial frequency of distortion.

- `speed` (float, 0.0–3.0, default 1.0)
  Temporal speed / evolution.

- `maskByWaveform` (bool)
  Distortion stronger near waveform region; requires auxiliary mask or analytic function.

### Audio Reactivity Hooks

- **Transient detector** → spikes of distortion.
- **Frequency content** → higher highs = “shimmer” distortion.

### Performance Notes

- **Medium to Expensive** depending on sampling strategy.
- Only for **Experimental/Ultra** presets; avoid in utility modes.

### Default Usage

- **Eco**: OFF.
- **Normal**: OFF (only for specific stylistic presets).
- **Ultra**: Optional, preset-driven.

---

## 10. Effect #9 – Sharpen (Optional, Very Subtle)

### Purpose / Visual Goals

Restore crispness if earlier post-FX (bloom, trails, downsampling) made the image too soft. Should be used very lightly.

### Inputs / Outputs

- **Input**: Previous buffer.
- **Output**: Next buffer.
- Implemented via simple unsharp mask or edge-enhancement filter.

### Parameters

- `amount` (float, 0.0–1.0, default ~0.1)
  Sharpen strength.

- `radius` (float, 0.5–2.0, default ~1.0)
  Blur radius for unsharp mask.

- `threshold` (float, 0.0–1.0, default 0.0)
  Edge threshold to avoid sharpening noise/grain.

### Audio Reactivity Hooks

- None. This should remain stable and not pump with music.

### Performance Notes

- **Medium** cost (extra samples per pixel, small blur).
- Should be optional and off by default unless a preset really needs it.

### Default Usage

- **Eco**: OFF.
- **Normal**: OFF or very low (if needed).
- **Ultra**: Optional, subtle.

---

## 11. Suggested Default Pipelines per Quality Level

### Eco

Focus: **performance & clarity**. No heavy effects.

- `ScenePass` → `ColorGrade` (Simple) → `Vignette` → `FilmGrain (low)` → Screen

Enabled effects:
- Color Grading (Simple)
- Vignette
- Optional very subtle Film Grain

Disabled:
- Bloom
- Trails
- Chromatic Aberration
- CRT/Scanlines
- Distortion
- Sharpen

### Normal

Focus: **good visuals** with reasonable cost.

- `ScenePass` → `Trails` → `Bloom` → `ColorGrade (Filmic)` → `FilmGrain` → `ChromaticAberration (subtle)` → `Vignette` → Screen

Enabled effects:
- Bloom (medium quality)
- Color Grading (Filmic/Simple)
- Vignette
- Film Grain
- Trails (conservative decay)
- Optional CA on compatible shaders

### Ultra

Focus: **maximum wow factor** for high-end GPUs.

- `ScenePass` → `Trails` → `Bloom (multi-level)` → `Distortion (optional)` → `ColorGrade (LUT/Filmic)` → `FilmGrain (rich)` → `ChromaticAberration` → `CRT/Scanlines (preset-specific)` → `Vignette` → `Sharpen (subtle)` → Screen

Enabled effects:
- All of the above, with higher quality settings.
- Optional extras (Distortion, CRT, Sharpen) controlled by **preset** and **shader metadata**.

---

## 12. Integration with Shader Metadata

Each **visual shader profile** should define which effects are allowed or recommended. Example metadata extension:

```cpp
struct ShaderMeta {
    ShaderType type = ShaderType::Waveform2D;

    bool allowPostProcessing = true;
    bool allowBloom = true;
    bool allowTrails = true;
    bool allowChromaticAberration = true;
    bool allowCRT = false;
    bool allowDistortion = false;

    QualityLevel minimumQuality = QualityLevel::Eco;
};
```

The **preset + shader metadata** together determine which effects are actually instantiated when building the pass graph. This ensures:

- Utility/debug shaders can run with **no post-FX**.
- Minimal shaders for weak GPUs can skip heavy passes.
- Feature-rich “AAA” shaders can enable full pipelines in Ultra mode.

---

This requirements file should give you a clear roadmap for implementing post-processing passes in your oscilloscope plugin while keeping everything **configurable, performant, and visually coherent**.
