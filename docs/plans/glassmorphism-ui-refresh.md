# Glassmorphism UI Refresh Plan

**Status**: Historical. Superseded by the 2026-Q2 flat-surface uplift.
**Date**: 2026-04-04 (original), superseded 2026-Q2
**Reference**: See internal demo-ui project for web reference implementation
**Scope**: Full visual + interaction refresh of all Oscil UI components, no backward compatibility required

> **Note (2026-Q2 update)**: The glassmorphism aesthetic was short-lived.
> The current UI system is a **flat surface** design; `GlassStyle` /
> `GlassPainter` were renamed to `SurfaceStyle` / `SurfacePainter` and the
> translucent / multi-layer-shadow / inset-highlight treatments described
> below were removed. Some *Glass*-prefixed serialization tokens
> (`bgGlass`, `glassAlpha`, `accentGlow*`) are retained in `ColorTheme`
> for back-compat only — new code should treat them as opaque surface
> tokens. This document is kept as historical context for the 2026-Q1 pass;
> consult the current source (`include/ui/components/SurfaceStyle.h`,
> `include/ui/components/SurfacePainter.h`) for the authoritative API.

---

## Motivation

The current Oscil UI uses flat opaque backgrounds, single-layer borders, and ease-out animations. It looks dated compared to modern 2026 design language. The web reference project implements a glassmorphism design system with translucent panels, multi-layer shadows, accent glows, spring animations, and ripple feedback. This plan ports that visual language to JUCE.

## Design Decisions

### What we ARE doing

- Faux glassmorphism: translucent backgrounds + multi-layer shadows + inset light edges
- Accent color as a first-class concept (7 presets: cyan, green, magenta, orange, blue, violet, red)
- 4 new glass-aware dark themes matching the web reference palettes
- Spring physics (with overshoot) for select interactions
- Ripple click feedback on buttons
- Accent glow effects on active/focused elements
- Updated component painting for every control

### What we are NOT doing

- **Real backdrop blur** — JUCE has no `backdrop-filter`. Faux glass (translucent bg + borders + shadows) delivers 95% of the visual impact with zero runtime cost. The web reference's blur is barely visible on dark backgrounds.
- **OKLCH color space** — JUCE only supports RGB/HSV/HSL. We use RGB. The perceptual difference in alpha/lightness operations on dark themes is negligible.
- **Audio feedback (SoundManager)** — The web project stubs it as no-ops. Audio feedback in an audio plugin interferes with monitoring. Skip.
- **New component types** — Banner, LoadingSpinner, NumberInput (expression parser), Popover, ColorGradientPicker exist in the web project but have no Oscil consumers. Out of scope.
- **Full token system rebuild** — The web project's token → semantic → component CSS architecture solves CSS-specific problems (scoping, Tailwind, data attributes). JUCE doesn't have these problems. We extend the existing `ColorTheme` struct instead of replacing it.

### Key architectural choice: extend, don't replace

The existing `ColorTheme` struct has ~40 color fields used by 98 call sites in 17 component files, 22 test files, and user-saved custom themes on disk. Replacing it would break serialization, the theme editor, all tests, and user data.

Instead: **add** new glass-specific fields with defaults. Existing custom themes deserialize fine (new fields get defaults). Component painting code migrates to reading from a computed `GlassStyle` helper. Old fields remain for grid colors, waveform colors, and backward-compat serialization.

---

## Work Packages

### WP1: Theme System Extension

**Goal**: Add glass-aware fields and a computed style layer to the existing theme system.

**Files to modify**:
- `include/ui/theme/ThemeManager.h` — extend `ColorTheme` struct
- `src/ui/theme/SystemThemes.cpp` — update existing themes, add 4 new glass themes
- `include/ui/components/ThemedComponent.h` — add `getGlass()` accessor
- `src/ui/components/ThemedComponent.cpp` — compute `GlassStyle` on theme change
- `src/ui/theme/ThemeManagerSerialization.cpp` — serialize new fields with defaults

#### ColorTheme additions

```cpp
// Accent system
float accentHue = 260.0f;         // OKLCH-inspired hue (mapped to HSV for JUCE)
float accentSaturation = 0.7f;    // Accent color saturation
float accentLightness = 0.6f;     // Accent color lightness

// Glass parameters
float glassAlpha = 0.55f;         // Background alpha for glass panels
float panelAlpha = 0.82f;         // Background alpha for panel containers
float blurRadius = 20.0f;         // Conceptual blur (used for shadow spread)

// Border parameters
float borderSubtleAlpha = 0.08f;  // Subtle border (white * alpha)
float borderDefaultAlpha = 0.12f; // Default border
float borderStrongAlpha = 0.20f;  // Strong border

// Shadow parameters
float shadowIntensity = 0.4f;     // Overall shadow darkness multiplier
float shadowSpread = 12.0f;       // Shadow blur radius in pixels

// Light edge (inset highlight at top of glass panels)
float lightEdgeAlpha = 0.06f;     // Inset top highlight

// Accent glow
float accentGlowRadius = 12.0f;   // Glow spread around accent elements
float accentGlowAlpha = 0.3f;     // Glow opacity
```

#### GlassStyle computed struct

```cpp
struct GlassStyle {
    // Computed from ColorTheme fields on theme change
    juce::Colour bgGlass;          // backgroundPane.withAlpha(glassAlpha)
    juce::Colour bgPanel;          // backgroundPane.withAlpha(panelAlpha)
    juce::Colour bgHover;          // textPrimary.withAlpha(0.08f)
    juce::Colour bgActive;         // textPrimary.withAlpha(0.12f)
    
    juce::Colour borderSubtle;     // textPrimary.withAlpha(borderSubtleAlpha)
    juce::Colour borderDefault;    // textPrimary.withAlpha(borderDefaultAlpha)
    juce::Colour borderStrong;     // textPrimary.withAlpha(borderStrongAlpha)
    
    juce::Colour accent;           // from accentHue/Sat/Light
    juce::Colour accentSubtle;     // accent.withAlpha(0.15f)
    juce::Colour accentMuted;      // accent.withAlpha(0.30f)
    juce::Colour accentGlow;       // accent (used for glow painting)
    
    juce::Colour insetLightEdge;   // white.withAlpha(lightEdgeAlpha)
    
    float shadowIntensity;
    float shadowSpread;
    float accentGlowRadius;
    float accentGlowAlpha;
    
    // Recompute all values from a ColorTheme
    void computeFrom(const ColorTheme& theme);
};
```

#### New system themes

| Theme Name | Web Reference | Base Hue | Tint |
|-----------|---------------|----------|------|
| **Glass Dark Blue** (default) | `theme-dark-blue.css` | 250deg | Steel-blue borders, cyan undertones |
| **Glass Dark Purple** | `theme-dark-purple.css` | 300deg | Lavender-tinted borders, violet glass |
| **Glass Dark Brown** | `theme-dark-brown.css` | 55deg | Amber-tinted borders, warm earth tones |
| **Glass Dark Black** | `theme-dark-black.css` | 240deg (near-zero chroma) | Neutral gray, minimal tint |

Existing themes (Dark Professional, Classic Green, Classic Amber, High Contrast, Light Mode) remain but get glass parameters set to match their aesthetic (High Contrast gets `glassAlpha = 1.0f` for full opacity — no translucency for accessibility).

#### Accent color presets

```cpp
namespace AccentPresets {
    struct AccentColor { float hue; float saturation; float lightness; juce::String name; };
    
    constexpr AccentColor cyan    = { 200.0f, 0.7f, 0.65f, "Cyan" };
    constexpr AccentColor green   = { 145.0f, 0.8f, 0.65f, "Green" };
    constexpr AccentColor magenta = { 330.0f, 0.8f, 0.60f, "Magenta" };
    constexpr AccentColor orange  = {  40.0f, 0.8f, 0.65f, "Orange" };
    constexpr AccentColor blue    = { 220.0f, 0.7f, 0.60f, "Blue" };
    constexpr AccentColor violet  = { 270.0f, 0.8f, 0.60f, "Violet" };
    constexpr AccentColor red     = {  10.0f, 0.8f, 0.55f, "Red" };
}
```

#### Accessibility

Update `ColorTheme::validateAccessibility()` to include:
- `textPrimary` on `bgGlass` (computed as `backgroundPane.withAlpha(glassAlpha)` over `backgroundPrimary`)
- `textSecondary` on `bgGlass`
- Accent on `backgroundPrimary` (for accent-colored text/icons)
- Maintain existing checks for non-glass contexts

The effective background behind glass is computed as: `backgroundPrimary` blended with `backgroundPane` at `glassAlpha`. This is deterministic (no real blur = no variable background).

#### Serialization

New fields serialize with the existing ValueTree approach. On deserialize, missing fields get their struct default values. Zero migration code needed — old themes load fine, just with default glass params.

---

### WP2: Shadow & Glass Painting Utilities

**Goal**: Reusable painting functions that all components share for consistent glass rendering.

**New files**:
- `include/ui/components/GlassPainter.h`
- `src/ui/components/GlassPainter.cpp`

#### GlassPainter API

```cpp
namespace GlassPainter {

    // Paint a glass panel background with full treatment:
    // translucent fill + inset light edge + multi-layer shadow + border
    void paintGlassPanel(juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         const GlassStyle& glass,
                         float cornerRadius,
                         BorderLevel border = BorderLevel::Subtle);

    // Paint just the glass background (no border/shadow) — for inline elements
    void paintGlassBackground(juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              const GlassStyle& glass,
                              float cornerRadius);

    // Paint a glass input field background (slightly different treatment)
    void paintGlassInput(juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         const GlassStyle& glass,
                         float cornerRadius,
                         bool focused,
                         bool hovered,
                         bool error);

    // Paint multi-layer shadow behind a rounded rect
    void paintShadow(juce::Graphics& g,
                     juce::Rectangle<float> bounds,
                     float cornerRadius,
                     float intensity,
                     float spread);

    // Paint inset light edge at top of a rounded rect
    void paintInsetLightEdge(juce::Graphics& g,
                             juce::Rectangle<float> bounds,
                             float cornerRadius,
                             float alpha);

    // Paint accent glow behind an element
    void paintAccentGlow(juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         juce::Colour accent,
                         float radius,
                         float alpha);

    // Paint expanding ripple effect(s) from click point
    void paintRipples(juce::Graphics& g,
                      juce::Rectangle<float> bounds,
                      const std::vector<RippleState>& ripples,
                      juce::Colour colour);

    // Paint focus ring (WCAG-compliant)
    void paintFocusRing(juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        float cornerRadius,
                        juce::Colour accent);

    enum class BorderLevel { None, Subtle, Default, Strong, Accent };
}
```

#### RippleState

```cpp
struct RippleState {
    float originX;           // Click position relative to component
    float originY;
    float birthTime;         // Time in seconds when created
    float maxRadius;         // Expands to fill component
    static constexpr float DURATION = 0.6f; // seconds
    
    float getProgress(float currentTime) const;
    float getAlpha(float currentTime) const;   // Fades out
    float getRadius(float currentTime) const;  // Expands
    bool isExpired(float currentTime) const;
};

// Manages active ripples for a component
class RippleManager {
public:
    void spawn(float x, float y, float componentWidth, float componentHeight);
    void update(float currentTime);  // Remove expired
    const std::vector<RippleState>& getRipples() const;
    bool hasActiveRipples() const;
private:
    std::vector<RippleState> ripples_;
    float getTime() const; // juce::Time based
};
```

#### Shadow rendering approach

JUCE's `juce::DropShadow` is single-layer. The web project uses stacked shadows:
```css
box-shadow:
    var(--shadow-soft),                          /* Outer: soft ambient */
    inset 0 1px 0 0 oklch(100% 0 0deg / 6%),   /* Inset: top light edge */
    inset 0 0 0 1px oklch(100% 0 0deg / 2%),    /* Inset: full perimeter glow */
    0 0 0 0.5px oklch(0% 0 0deg / 15%);         /* Outer: tight dark outline */
```

In JUCE, we render this as:
1. **Outer shadow**: Paint filled rounded rect offset down with blur (via multiple expanding semi-transparent rects or `juce::DropShadow`)
2. **Dark outline**: Stroke rounded rect with 0.5px dark semi-transparent
3. **Glass fill**: Fill rounded rect with `bgGlass`
4. **Inset perimeter glow**: Stroke inner rounded rect with 1px white at 2% alpha
5. **Top light edge**: Draw horizontal line/thin rect at top with 6% white alpha

---

### WP3: SpringAnimation Enhancement

**Goal**: Add real spring physics mode alongside existing ease-out, for select interactions.

**Files to modify**:
- `include/ui/components/SpringAnimation.h`

#### Changes

```cpp
enum class SpringMode { EaseOut, Spring };

struct SpringAnimation {
    // ... existing fields ...
    SpringMode mode = SpringMode::EaseOut;
    
    void setMode(SpringMode m) { mode = m; }
    
    void update(float deltaTime) {
        if (mode == SpringMode::EaseOut) {
            updateEaseOut(deltaTime);  // Current implementation
        } else {
            updateSpring(deltaTime);   // New: real spring physics
        }
    }
    
private:
    void updateEaseOut(float deltaTime);  // Extract current update() body
    void updateSpring(float deltaTime);   // Verlet/semi-implicit Euler integration
};
```

#### Spring physics implementation

```cpp
void SpringAnimation::updateSpring(float deltaTime) {
    deltaTime = std::min(deltaTime, MAX_DELTA_TIME);
    
    // Semi-implicit Euler (stable, cheap)
    float displacement = position - target;
    float springForce = -stiffness * displacement;
    float dampingForce = -damping * velocity;
    float acceleration = (springForce + dampingForce) / mass;
    
    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;
    
    // Settle when both position and velocity are negligible
    if (std::abs(position - target) < 0.0005f && std::abs(velocity) < 0.01f) {
        position = target;
        velocity = 0.0f;
    }
}
```

#### New presets

```cpp
namespace SpringPresets {
    // Existing (unchanged, use EaseOut mode)
    inline SpringAnimation fast()   { return {500.0f, 0.0f, 1.0f}; }
    inline SpringAnimation medium() { return {350.0f, 0.0f, 1.0f}; }
    inline SpringAnimation slow()   { return {200.0f, 0.0f, 1.0f}; }
    
    // New spring-physics presets (matching web reference motion/react values)
    inline SpringAnimation springSwitch() {
        SpringAnimation s{700.0f, 30.0f, 1.0f};
        s.mode = SpringMode::Spring;
        return s;
    }
    inline SpringAnimation springIndicator() {
        SpringAnimation s{500.0f, 30.0f, 1.0f};
        s.mode = SpringMode::Spring;
        return s;
    }
    inline SpringAnimation springPopup() {
        SpringAnimation s{400.0f, 25.0f, 1.0f};
        s.mode = SpringMode::Spring;
        return s;
    }
}
```

---

### WP4: Component Visual Refresh

**Goal**: Rewrite painting code for every component to use glass rendering, new color scheme, and new interactions.

This is the largest work package. Each component is an independent unit of work that can be done in parallel (different files, no shared mutable state).

#### OscilButton

**Files**: `OscilButton.h`, `OscilButton.cpp`, `OscilButtonPainting.cpp`

Visual changes:
- **Primary**: `accentSubtle` background + `accent` border + accent text. Hover: brighter accent bg + accent glow (`paintAccentGlow`). Active: darker accent bg.
- **Secondary**: Transparent bg + `borderDefault` border + `textSecondary` text. Hover: `bgHover` fill + `textPrimary` text + border brightens. Active: `bgActive`.
- **Ghost**: Transparent + no border. Hover: `bgHover`. Active: `bgActive`.
- **Danger**: Transparent + danger border + danger text. Hover: danger bg fill.
- **Icon**: Same as Ghost but square with icon sizing.
- **Tertiary**: Remove (merge behavior into Ghost — they're identical in the web reference).

Interaction changes:
- Add `RippleManager rippleManager_` member
- On `mouseDown`: `rippleManager_.spawn(e.x, e.y, ...)`
- In `paint()`: `GlassPainter::paintRipples(g, bounds, rippleManager_.getRipples(), textPrimary.withAlpha(0.1f))`
- Hover: `translateY(-0.5px)` effect via slight bounds offset
- Active: `translateY(1px)` effect

Color resolution rewrite (`getBackgroundColour`, `getTextColour`, `getBorderColour`):
- Replace per-variant per-state color lookups from `ColorTheme.btnPrimary*` fields
- Instead derive from `GlassStyle`: accent, accentSubtle, bgHover, bgActive, borderDefault, textPrimary, textSecondary
- Much simpler logic — the web reference uses ~4 semantic colors, not 24 explicit per-state colors

#### OscilSlider

**Files**: `OscilSlider.h`, `OscilSliderPainting.cpp`

Visual changes:
- Track: Glass background (`bgSurface` fill + `borderSubtle` border), `RADIUS_FULL` corners
- Fill: `accentMuted` from left to thumb position (60% accent opacity)
- Thumb: White circle with `accent` border (2px), `shadow-sm`
- Hover on thumb: `scale(1.1)` via spring animation
- Focus: Accent glow behind thumb

No API changes. APVTS integration unaffected (internal `juce::Slider` unchanged).

#### OscilToggle

**Files**: `OscilToggle.h`, `OscilToggle.cpp`

Visual changes:
- Track OFF: `bgSurface` fill + `borderDefault` border
- Track ON: `accent` fill + `accent` border + accent glow (`shadow: 0 0 15px accentGlow`)
- Thumb OFF: `textSecondary` colored circle, hover: `textPrimary`
- Thumb ON: White circle with subtle shadow
- Use `SpringPresets::springSwitch()` for thumb position animation

#### OscilTextField

**Files**: `OscilTextField.h`, `OscilTextFieldPainting.cpp`

Visual changes:
- Background: `paintGlassInput()` — glass bg, subtle border
- Hover: Border brightens to `borderDefault`, bg lightens slightly
- Focus: Accent border + accent glow ring + bg tints toward accent
- Error: Danger border + shake animation (horizontal oscillation via spring)
- Placeholder: `textMuted` color

Add error shake: on validation error, set a horizontal offset spring with target 0 and initial position ±2px, let it oscillate.

#### OscilDropdown

**Files**: `OscilDropdown.h`, `OscilDropdown.cpp`, `OscilDropdownPopup.cpp`

Visual changes:
- Trigger: `paintGlassInput()` styling (matches text field)
- Chevron: `textTertiary`, animates rotation + translateY on hover
- Popup: `paintGlassPanel()` with `shadow-xl`, spring open animation (`springPopup()`)
- Items: Hover = `bgHover`, active = `bgActive`
- Selected item: `accent` text + `accentSubtle` bg

#### OscilCheckbox

**Files**: `OscilCheckbox.h`, `OscilCheckbox.cpp`

Visual changes:
- Unchecked: `bgSurface` fill + `borderDefault` border, `RADIUS_MD` corners
- Checked: `accent` fill + white checkmark, `RADIUS_MD`
- Hover: Border brightens
- Spring scale on check state change

#### OscilRadioButton

**Files**: `OscilRadioButton.h`, `OscilRadioButtonPainting.cpp`

Visual changes:
- Outer ring: `borderDefault`, hover: `borderStrong`
- Selected: `accent` fill dot in center
- Spring scale on selection change

#### OscilTabs

**Files**: `OscilTabs.h`, `OscilTabsPainting.cpp`

Visual changes:
- Default variant: Bottom border on tab list. Active tab has animated accent underline (slides between tabs using `springIndicator()`).
- Pills variant: `bgHover` background on tab list. Active tab has `accentSubtle` bg pill with `accentMuted` border.
- Tab text: `textSecondary` default, `accent` when active (default), `textPrimary` when active (pills)

#### OscilAccordion / OscilAccordionSection

**Files**: `OscilAccordionSection.cpp`

Visual changes:
- Section borders: `borderSubtle`
- Header: Hover = `bgHover`
- Chevron: Animated rotation (already exists, verify it uses spring)
- Content: Animated height on expand/collapse

#### OscilModal

**Files**: `OscilModal.h`, `OscilModalPainting.cpp`

Visual changes:
- Overlay: `backgroundPrimary` at 50% alpha (dark scrim)
- Panel: `paintGlassPanel()` with `shadow-2xl`, `RADIUS_XL` corners
- Header: `bgHover` tinted bg, `borderDefault` bottom border
- Entrance: Scale from 0.95 → 1.0 + fade in, using `springPopup()`

#### SegmentedButtonBar

**Files**: `SegmentedButtonBar.h`, `SegmentedButtonBar.cpp`

Visual changes — **significant rework**:
- Container: Glass-input styling (`paintGlassInput()` on the bar itself)
- Sliding indicator: Accent-subtle bg + accent-muted border, spring-animated position (`springIndicator()`)
- Button text: `textSecondary` default, `accent` when selected
- Remove per-button background painting, replace with single sliding indicator

#### OscilBadge

**Files**: `OscilBadge.h`, `OscilBadge.cpp`

Visual changes:
- Default: Glass bg + subtle border
- Status variants: Use semantic status colors from theme (danger/success/warning bg + text)

#### OscilColorPicker, OscilColorSwatches

**Files**: `OscilColorPicker.h/cpp`, `OscilColorSwatches.cpp`

Visual changes:
- Use glass panel styling for container
- Focus ring uses accent color
- Otherwise minimal changes — these are specialized components

#### InlineEditLabel

**Files**: `InlineEditLabel.h/cpp`

Visual changes:
- Edit mode: Glass-input styling
- Focus: Accent border + glow

---

### WP5: Consumer File Updates

**Goal**: Update non-component files that paint backgrounds or reference theme fields.

**Files to modify**:
- `src/ui/layout/SidebarComponent.cpp` — paint sidebar background using `GlassPainter::paintGlassPanel()`
- `src/ui/layout/pane/PaneHeader.cpp` — glass panel header treatment
- `src/ui/layout/pane/PaneActionBar.cpp` — update border color reference
- `src/ui/panels/OscillatorListComponent.cpp` — update textSecondary reference
- `src/ui/panels/SourceListItem.cpp` — hover/selection using glass colors
- `src/ui/panels/SoftwareGridRenderer.cpp` — grid background
- `src/ui/theme/ThemeEditorComponent.cpp` — add accent color picker, glass parameter controls
- `src/ui/dialogs/*.cpp` — dialog backgrounds use glass panels
- `src/plugin/PluginEditor.cpp` — main background: radial gradient with accent tint (matching web `.pf-shell-backdrop`)

#### Main application background

The web reference uses a radial gradient backdrop:
```css
background:
    radial-gradient(ellipse 80% 50% at 20% 10%, accent 10%, transparent 60%),
    radial-gradient(ellipse 60% 40% at 80% 90%, accent 6%, transparent 60%),
    linear-gradient(180deg, bg-app+5% 0%, bg-app 25%, bg-app-6% 100%);
```

In JUCE: paint the main editor background with:
1. Solid `backgroundPrimary` base
2. Two radial gradient overlays using accent at low alpha (10%, 6%)
3. Subtle vertical gradient (lighter top, darker bottom)

This gives the plugin a subtle accent-tinted ambiance that ties the whole UI together.

---

### WP6: Test Updates

**Goal**: All tests pass after the visual refresh.

**Files to modify**:
- `tests/OscilTestFixtures.h` — add new glass fields to test theme construction
- `tests/test_theme_manager_persistence.cpp` — add serialization round-trip tests for new fields
- `tests/test_theme_manager_apply.cpp` — verify GlassStyle recomputes on theme change
- `tests/test_theme_accessibility.cpp` — add glass-contrast validation tests
- `tests/test_config_structs.cpp` — if touched by theme changes
- Component test files — verify new interaction states (ripple, spring mode) are testable

**New tests to add**:
- `GlassStyle::computeFrom()` produces correct derived colors
- `GlassPainter` functions don't crash with edge-case inputs (zero-size bounds, null colors)
- `RippleManager` spawns, updates, and expires ripples correctly
- `SpringAnimation` in `SpringMode::Spring` settles to target, doesn't diverge
- Accent preset colors pass WCAG contrast on all glass theme backgrounds
- Old theme files (without new fields) deserialize without error and get defaults

**Verification**: `ctest --preset dev` must pass 100%.

---

## Implementation Order

```text
WP1 (Theme Extension) ──────────┐
                                 ├── WP4 (Component Visual Refresh)
WP2 (Glass Painting Utilities) ──┤     ├── OscilButton
                                 │     ├── OscilSlider
WP3 (Spring Enhancement) ────────┘     ├── OscilToggle
                                       ├── OscilTextField
                                       ├── OscilDropdown + Popup
                                       ├── OscilCheckbox
                                       ├── OscilRadioButton
                                       ├── OscilTabs
                                       ├── OscilAccordion
                                       ├── OscilModal
                                       ├── SegmentedButtonBar
                                       ├── OscilBadge
                                       ├── OscilColorPicker/Swatches
                                       └── InlineEditLabel
                                            │
                                            ├── WP5 (Consumer Updates)
                                            └── WP6 (Test Updates)
```

- **WP1 + WP2 + WP3** are foundation — must complete first
- **WP4 components** are independent of each other — can parallelize
- **WP5 + WP6** depend on WP4 completion

## Blast Radius Summary

| Category | Count | Impact |
|----------|-------|--------|
| New files | 2 | `GlassPainter.h`, `GlassPainter.cpp` |
| Modified component files | ~30 | All painting + interaction files |
| Modified theme files | ~6 | Theme struct, system themes, serialization, ThemedComponent |
| Modified consumer files | ~10-15 | Dialogs, panels, layout, editor |
| Modified test files | ~8-10 | Theme tests, component tests, fixtures |
| **Total files touched** | **~50-60** | |

## Risk Register

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Glass panels look flat without real blur | Medium | Multi-layer shadow + inset light edge + subtle translucency create depth without blur. Verify visually. |
| Spring overshoot causes visual artifacts | Medium | Use spring mode only for specific interactions (toggle, indicator, popup). Tune damping to minimize overshoot. Ease-out remains default. |
| Performance regression from shadow painting | Low | Shadows are simple filled rects — cheaper than OpenGL. Cache shadow images if needed. Profile. |
| Accessibility regression with translucent backgrounds | High | Validate contrast against computed effective background (solid base + translucent overlay). High Contrast theme uses glassAlpha=1.0 (no translucency). |
| Custom user themes look wrong with glass defaults | Low | New fields have sensible defaults. Existing custom themes gain glass treatment automatically. Users can adjust in theme editor. |

## Verification Checklist

Before declaring any work package complete:

- [ ] `cmake --build --preset dev` compiles without warnings
- [ ] `ctest --preset dev` passes 100%
- [ ] Visual inspection in standalone app confirms glass treatment
- [ ] High Contrast theme still fully opaque and WCAG AAA compliant
- [ ] Accent color switch works across all system themes
- [ ] Existing custom themes load without error
- [ ] No performance regression in waveform rendering (glass is UI only, not OpenGL path)
