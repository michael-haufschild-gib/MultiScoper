# GPU Rendering Review & Architecture Analysis

**Date:** November 29, 2025
**Project:** Oscil Multi-Track Oscilloscope
**Component:** GPU Rendering Subsystem (`WaveformGLRenderer`, `NeonGlowShader`)

## Executive Summary

The current GPU rendering implementation contains a **critical architectural flaw** that will cause the plugin's User Interface (UI) to disappear when GPU rendering is enabled. By attaching a custom `juce::OpenGLRenderer` to the main `OscilPluginEditor` and using `setRenderer()`, the standard JUCE component painting mechanism is bypassed, resulting in a blank screen (except for the custom-rendered waveforms).

Additionally, there are significant risks regarding OpenGL version compatibility (macOS Legacy vs. Core Profile) and potential coordinate system mismatches.

## 1. Critical Issues

### 1.1. UI Disappearance (The "Global Renderer" Problem)

**Severity:** Critical
**Location:** `src/ui/PluginEditor.cpp`

When `openGLContext_.setRenderer(glRenderer_.get())` is called, the provided renderer takes complete control of the OpenGL context's rendering loop (`renderOpenGL()`). The default behavior of `juce::OpenGLContext`—which is to traverse the component hierarchy and paint standard UI elements (sliders, buttons, backgrounds)—is **disabled**.

**Result:**
-   The `WaveformGLRenderer::renderOpenGL` method clears the screen and draws waveforms.
-   The Sidebar, Status Bar, Window Layout, and all other controls are **never drawn**.
-   The plugin will appear as a dark background with floating waveforms and no controls.

**Fix Strategy:**
Do not use `setRenderer()` on the main editor's context if you want to retain standard UI rendering. Instead, use one of the following approaches:
1.  **Hybrid Approach (Recommended):** Do *not* set a custom renderer. Let JUCE handle the main loop. Use `juce::OpenGLGraphicsContextCustomShader` within `WaveformComponent::paint()` to apply the glow effects. This leverages JUCE's batched renderer while allowing custom shader code.
2.  **Underlay Component:** Attach the `OpenGLContext` (with custom renderer) to a `BackgroundComponent` that sits *behind* all other UI elements. Note: This requires careful handling of transparency in the foreground components and can be tricky with windowing.
3.  **Hook Approach:** If raw GL performance is strictly required (batching), register a `OpenGLRenderer` that *also* manually triggers the painting of the JUCE component tree (though this API is not publicly exposed in a standard way).

### 1.2. OpenGL Version Compatibility

**Severity:** High
**Location:** `src/rendering/shaders/NeonGlowShader.cpp`

The shader source specifies `#version 330 core`.
```glsl
#version 330 core
layout(location = 0) in vec2 position;
```

**The Problem:**
-   On macOS, requesting an OpenGL context without specific flags often results in a **Legacy (2.1)** context to ensure compatibility with older calls.
-   `#version 330` requires OpenGL 3.3+.
-   If the context is 2.1, shader compilation will fail, and nothing will render.
-   `OscilPluginEditor` initializes `openGLContext_` but does not explicitly request a Core Profile (3.2+).

**Fix Strategy:**
-   In `OscilPluginEditor`, configure the context before attachment:
    ```cpp
    juce::OpenGLPixelFormat pixelFormat;
    pixelFormat.openglVersionRequired = juce::OpenGLPixelFormat::openGL3_2; 
    openGLContext_.setPixelFormat(pixelFormat);
    ```
-   Alternatively, write shaders compatible with GLSL 1.20 (`#version 120`) or 1.50 (`#version 150`) to support a wider range of hosts/hardware.

## 2. Architectural & Logic Findings

### 2.1. Coordinate Systems

**Status:** Likely Correct (Logic verified)
**Location:** `NeonGlowShader::render` vs `WaveformGLRenderer`

-   **Renderer:** Uses `glViewport(0, 0, W, H)` (full window).
-   **Data Prep:** `populateGLRenderData` calculates bounds relative to the `AudioProcessorEditor` (top-left origin).
-   **Shader:** Uses an orthographic projection matrix.
    -   Calculations show the projection matrix correctly flips the Y-axis (mapping JUCE's Y-down to OpenGL's Y-up) and maps the pixel coordinates to NDC [-1, 1].
-   **Risk:** This relies on `getTopLevelComponent()` or the parent traversal correctly finding the component that corresponds to the GL viewport. If the hierarchy changes (e.g., wrapping the editor in another component), the `relativeBounds` might be offset incorrectly.

### 2.2. Data Upload Efficiency

**Status:** Suboptimal but Acceptable
**Location:** `NeonGlowShader::render` -> `buildLineGeometry`

-   The geometry (vertices) is rebuilt on the CPU and uploaded to the GPU (`glBufferData`) **every frame**.
-   For a few dozen oscillators, this is negligible.
-   For high counts (100+), this constitutes significant bus traffic.
-   **Optimization:** Consider using a Ring Buffer or `GL_MAP_WRITE_BIT` (persistent mapping) if performance bottlenecks appear. For now, `GL_DYNAMIC_DRAW` is the correct usage hint.

### 2.3. Thread Safety

**Status:** Good
**Location:** `WaveformGLRenderer`, `WaveformComponent`

-   The use of `WaveformRenderData` as a decoupled snapshot (POD struct) is excellent.
-   `juce::SpinLock` protects the map access.
-   Synchronization between the Message Thread (update) and Render Thread (draw) is handled correctly.

## 3. Implementation Bugs

### 3.1. Missing Context Cleanup

**Location:** `WaveformGLRenderer::openGLContextClosing`

-   The renderer calls `ShaderRegistry::releaseAll(*context_)`.
-   However, the `NeonGlowShader::release` method implementation:
    ```cpp
    if (gl_->vbo != 0) { ext.glDeleteBuffers(1, &gl_->vbo); ... }
    ```
    This is correct, *provided* the context is still active. JUCE guarantees the context is active during `openGLContextClosing`.
-   **Issue:** `NeonGlowShader` destructor doesn't clean up if `release` wasn't called. This is a potential leak if the app terminates unexpectedly or if the renderer is destroyed before the context closes (though RAII in `PluginEditor` usually handles this order).

### 3.2. Shader Uniform Validation

**Location:** `NeonGlowShader::compile`

-   The code checks `gl_->projectionLoc < 0`.
-   **False Positive Risk:** In optimized GLSL, unused uniforms are compiled out. If a uniform is temporarily unused in the shader code, `getUniformID` returns -1, and the C++ code returns `false` (compilation failure).
-   The current shader uses all uniforms, so this is fine, but be aware that strict uniform validation can block development if you comment out shader code.

## 4. Recommendations

### Immediate Action Items (Must Fix)

1.  **Restore UI Rendering:**
    -   **Disable `setRenderer()`** in `PluginEditor.cpp`.
    -   Refactor `WaveformGLRenderer` to be a helper class that is called from `WaveformComponent::paint()` (if using `OpenGLGraphicsContextCustomShader`) or simply rely on JUCE's standard OpenGL rendering of `Path` objects (which is often fast enough).
    -   *Alternative:* If "Neon Glow" requires custom shaders, implement a `juce::OpenGLGraphicsContextCustomShader` that can be used inside the standard `paint` loop.

2.  **Enforce OpenGL Core Profile:**
    -   In `OscilPluginEditor` constructor, set the pixel format to request OpenGL 3.2+.

### Future Improvements

1.  **Geometry Caching:**
    -   Only rebuild `buildLineGeometry` when the audio data actually changes (which is every frame for an oscilloscope, so maybe moot).
    -   However, static elements (grid lines, borders) should be cached in a static VBO, not rebuilt.

2.  **Batch Rendering:**
    -   Currently, `NeonGlowShader::render` is called per waveform. This means 1 draw call per oscillator.
    -   With 100 oscillators, that's 100 draw calls + state changes.
    -   **Optimization:** Pack multiple waveforms into a single large VBO and use `glMultiDrawArrays` or a single draw call with an index buffer (if they share the same shader/uniforms).
