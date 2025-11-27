# Oscil UI Component Library Plan

**Purpose**: Comprehensive plan for creating a reusable, consistent UI component library for the Oscil audio plugin.

**Reference**: `docs/ui-designs/ui-components.jpeg`

**Design Philosophy**: Components should feel responsive, professional, and delightful. Every interaction should provide clear feedback. The library should meet accessibility standards (EAA 2025) while supporting the unique requirements of real-time audio plugin development.

---

## Component Overview

| Component | Variants | States | Priority |
|-----------|----------|--------|----------|
| OscilButton | Primary/Secondary/Danger/Ghost/Icon | Default/Hover/Pressed/Disabled/Focused | P1 |
| OscilTextField | Text/Search/Number | Empty/Focused/Filled/Error/Disabled | P1 |
| OscilSlider | Single/Range/Vertical | Default/Hover/Dragging/Disabled/Focused | P2 |
| OscilToggle | Standard | On/Off/Disabled + animated | P1 |
| OscilCheckbox | Standard | Unchecked/Checked/Indeterminate/Disabled/Focused | P2 |
| OscilRadioButton | Standard + RadioGroup | Unselected/Selected/Disabled/Focused | P2 |
| OscilDropdown | Single/Multi/Searchable | Closed/Open/Disabled/Focused | P2 |
| OscilTabs | Horizontal/Vertical | Active/Hover/Disabled + animated | P3 |
| OscilAccordion | Standard | Collapsed/Expanded + animated | P3 |
| OscilBadge | Filled/Outline | Default (5 colors) | P3 |
| OscilModal | Standard | Visible/Hidden + animated | P3 |
| OscilColorPicker | Full | Interactive | P4 |
| OscilColorSwatches | Horizontal row | Hover/Selected | P4 |
| OscilMeterBar | Horizontal/Vertical | Continuous | P4 |
| OscilTransportSync | Composite | Play/Pause/Linked | P4 |
| OscilTooltip | Rich | Standard/Contextual | P1 |

---

## JUCE Audio Plugin Integration

### AudioProcessorValueTreeState (APVTS) Support

All value controls (sliders, toggles, dropdowns) should support optional APVTS attachment for DAW automation:

```cpp
class OscilSlider : public juce::Component, public ThemeManagerListener {
    // Optional APVTS attachment for DAW automation
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

public:
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId) {
        // Create internal JUCE slider for attachment compatibility
        attachment_ = std::make_unique<SliderAttachment>(apvts, paramId, internalSlider_);
    }

    void detachFromParameter() {
        attachment_.reset();
    }

    bool isAttachedToParameter() const { return attachment_ != nullptr; }
};
```

### DPI/Scale Factor Awareness

All components must render correctly at any scale factor:

```cpp
class OscilComponent : public juce::Component {
protected:
    float getEffectiveScale() const {
        return static_cast<float>(getDesktopScaleFactor());
    }

    float scaledValue(float value) const {
        return value * getEffectiveScale();
    }

    void paint(juce::Graphics& g) override {
        // Use scaled values for line widths
        g.drawLine(x1, y1, x2, y2, scaledValue(1.0f));
    }
};
```

### GPU Rendering Support

For performance-critical components (MeterBar, WaveformComponent), support OpenGL rendering:

```cpp
class OscilMeterBar : public juce::Component,
                      public juce::OpenGLRenderer {  // Optional GPU path
    bool useOpenGL_ = false;

public:
    void enableOpenGLRendering(juce::OpenGLContext& context) {
        useOpenGL_ = true;
        context.attachTo(*this);
    }

    void newOpenGLContextCreated() override {
        // Initialize shaders/buffers
    }

    void renderOpenGL() override {
        // GPU-accelerated rendering path
    }

    void paint(juce::Graphics& g) override {
        if (!useOpenGL_) {
            // CPU fallback path
            paintSoftware(g);
        }
    }
};
```

### Plugin State Persistence

Components should support ValueTree serialization for preset/state saving:

```cpp
class OscilComponent {
public:
    virtual juce::ValueTree getState() const {
        juce::ValueTree state("ComponentState");
        // Serialize component-specific state
        return state;
    }

    virtual void setState(const juce::ValueTree& state) {
        // Restore component-specific state
    }
};
```

### DAW Focus Management

Handle keyboard focus properly in plugin context:

```cpp
class OscilComponent : public juce::Component {
    void focusGained(FocusChangeType cause) override {
        hasFocus_ = true;
        repaint();  // Draw focus ring

        // Notify parent of focus for DAW keyboard handling
        if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
            editor->setWantsKeyboardFocus(true);
    }

    void focusLost(FocusChangeType cause) override {
        hasFocus_ = false;
        repaint();
    }
};
```

---

## Accessibility (JUCE 6.1+ Native Support)

### AccessibilityHandler Implementation

Every interactive component must implement accessibility:

```cpp
class OscilButton : public juce::Component {
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
        return std::make_unique<juce::AccessibilityHandler>(
            *this,
            juce::AccessibilityRole::button,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press,
                    [this] { if (onClick) onClick(); })
        );
    }

    juce::String getAccessibilityTitle() const {
        return label_.isEmpty() ? "Button" : label_;
    }

    juce::String getAccessibilityHelp() const {
        return shortcutKey_.isValid()
            ? "Press " + shortcutKey_.getTextDescription() + " to activate"
            : "";
    }
};

class OscilSlider : public juce::Component {
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
        return std::make_unique<juce::AccessibilityHandler>(
            *this,
            juce::AccessibilityRole::slider,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::showMenu,
                    [this] { showContextMenu(); })
        );
    }

    juce::AccessibleState getCurrentState() const override {
        auto state = juce::AccessibleState();
        state.isEnabled = enabled_;
        state.isFocusable = true;
        state.isFocused = hasKeyboardFocus(false);
        return state;
    }
};
```

### Keyboard Navigation

Full keyboard support for all components:

```cpp
namespace KeyboardNav {
    // Standard navigation keys
    constexpr auto TAB = juce::KeyPress::tabKey;
    constexpr auto SHIFT_TAB = juce::KeyPress(juce::KeyPress::tabKey, juce::ModifierKeys::shiftModifier, 0);
    constexpr auto ENTER = juce::KeyPress::returnKey;
    constexpr auto SPACE = juce::KeyPress::spaceKey;
    constexpr auto ESCAPE = juce::KeyPress::escapeKey;
    constexpr auto ARROW_UP = juce::KeyPress::upKey;
    constexpr auto ARROW_DOWN = juce::KeyPress::downKey;
    constexpr auto ARROW_LEFT = juce::KeyPress::leftKey;
    constexpr auto ARROW_RIGHT = juce::KeyPress::rightKey;
}

class OscilComponent : public juce::Component {
    bool keyPressed(const juce::KeyPress& key) override {
        if (key == KeyboardNav::ESCAPE) {
            // Return focus to parent
            if (auto* parent = getParentComponent())
                parent->grabKeyboardFocus();
            return true;
        }
        return false;
    }
};
```

### Focus Ring Rendering

Visible focus indicators for keyboard users:

```cpp
void OscilComponent::paintFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (!hasKeyboardFocus(false)) return;

    auto focusColour = theme_.controlActive.withAlpha(0.5f);
    g.setColour(focusColour);
    g.drawRoundedRectangle(bounds.expanded(2.0f), cornerRadius_ + 2.0f, 2.0f);
}
```

---

## Animation System

### Spring Physics for Natural Motion

Replace linear animations with spring physics for a premium feel:

```cpp
// include/ui/components/SpringAnimation.h
struct SpringAnimation {
    float position = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;

    // Spring constants - tune for different feels
    float stiffness = 300.0f;   // Higher = snappier
    float damping = 20.0f;      // Higher = less oscillation
    float mass = 1.0f;

    void setTarget(float newTarget) {
        target = newTarget;
    }

    void update(float deltaTime) {
        float displacement = position - target;
        float springForce = -stiffness * displacement;
        float dampingForce = -damping * velocity;
        float acceleration = (springForce + dampingForce) / mass;

        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
    }

    bool isSettled(float threshold = 0.001f) const {
        return std::abs(position - target) < threshold &&
               std::abs(velocity) < threshold;
    }

    void snapToTarget() {
        position = target;
        velocity = 0.0f;
    }
};

// Preset spring configurations
namespace SpringPresets {
    constexpr auto SNAPPY = SpringAnimation{ .stiffness = 400.0f, .damping = 30.0f };
    constexpr auto BOUNCY = SpringAnimation{ .stiffness = 300.0f, .damping = 15.0f };
    constexpr auto SMOOTH = SpringAnimation{ .stiffness = 200.0f, .damping = 25.0f };
    constexpr auto GENTLE = SpringAnimation{ .stiffness = 150.0f, .damping = 20.0f };
}
```

### Reduced Motion Support

Respect user accessibility preferences:

```cpp
class AnimationSettings {
public:
    static bool prefersReducedMotion() {
        #if JUCE_MAC
        // Check macOS accessibility setting
        return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
        #elif JUCE_WINDOWS
        BOOL reduceAnimations = FALSE;
        SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &reduceAnimations, 0);
        return !reduceAnimations;
        #else
        return GlobalPreferences::getInstance().getReducedMotion();
        #endif
    }

    static float getAnimationDuration(float normalDuration) {
        return prefersReducedMotion() ? 0.0f : normalDuration;
    }
};

// Usage in components
void OscilToggle::setValue(bool newValue, bool animate) {
    value_ = newValue;

    if (animate && !AnimationSettings::prefersReducedMotion()) {
        spring_.setTarget(value_ ? 1.0f : 0.0f);
        startTimerHz(60);
    } else {
        spring_.snapToTarget();
        repaint();
    }
}
```

### Theme Transition Animation

Smooth transitions when switching themes:

```cpp
class ThemeTransition {
    ColorTheme fromTheme_;
    ColorTheme toTheme_;
    float progress_ = 1.0f;

public:
    void startTransition(const ColorTheme& from, const ColorTheme& to) {
        fromTheme_ = from;
        toTheme_ = to;
        progress_ = 0.0f;
    }

    juce::Colour interpolateColour(const juce::Colour& from, const juce::Colour& to) const {
        return from.interpolatedWith(to, progress_);
    }

    void update(float deltaTime) {
        progress_ = std::min(1.0f, progress_ + deltaTime * 4.0f);  // 250ms transition
    }

    bool isComplete() const { return progress_ >= 1.0f; }
};
```

---

## Microinteractions & Delighters

### Universal Control Enhancements

Apply these interactions to ALL value controls (sliders, number fields, etc.):

| Feature | Input | Behavior | User Benefit |
|---------|-------|----------|--------------|
| Double-click reset | Double-click | Reset to `defaultValue_` | Quick return to defaults |
| Scroll-wheel adjust | Mouse wheel | Adjust by `step_` | Fast value changes without drag |
| Fine control | Alt+drag | 0.1x sensitivity | Precision adjustments |
| Snap to grid | Shift+drag | Snap to magnetic points | Quick jumps to common values |
| Copy value | Cmd/Ctrl+C | Copy value to clipboard | Share between controls |
| Paste value | Cmd/Ctrl+V | Paste from clipboard | Transfer values |
| Context menu | Right-click | Show Reset/Copy/Paste | Discoverability |
| Value scrub | Drag on label | Adjust value | Alternative input |

```cpp
class OscilValueControl : public juce::Component {
protected:
    double defaultValue_ = 0.0;
    std::vector<double> magneticPoints_;

    void mouseDoubleClick(const juce::MouseEvent&) override {
        setValue(defaultValue_);
        // Celebration feedback
        triggerResetAnimation();
    }

    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override {
        if (!enabled_) return;

        double delta = wheel.deltaY * step_;
        if (e.mods.isShiftDown()) delta *= 10.0;  // Coarse
        if (e.mods.isAltDown()) delta *= 0.1;     // Fine

        setValue(juce::jlimit(minValue_, maxValue_, value_ + delta));
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        double sensitivity = e.mods.isAltDown() ? 0.1 : 1.0;
        // ... apply sensitivity to drag delta
    }

    bool keyPressed(const juce::KeyPress& key) override {
        if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
            juce::SystemClipboard::copyTextToClipboard(juce::String(value_));
            return true;
        }
        if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0)) {
            auto text = juce::SystemClipboard::getTextFromClipboard();
            if (auto parsed = text.getDoubleValue(); parsed != 0.0 || text == "0") {
                setValue(parsed);
            }
            return true;
        }
        return false;
    }
};
```

### Magnetic Snapping

Sliders snap to meaningful values with satisfying feedback:

```cpp
void OscilSlider::applyMagneticSnapping(double& value) {
    // Build magnetic points
    magneticPoints_ = {
        minValue_,
        (minValue_ + maxValue_) * 0.25,
        (minValue_ + maxValue_) * 0.5,   // Center
        (minValue_ + maxValue_) * 0.75,
        maxValue_,
        defaultValue_                     // Always snap to default
    };

    const double snapThreshold = (maxValue_ - minValue_) * 0.02;  // 2% of range

    for (double point : magneticPoints_) {
        if (std::abs(value - point) < snapThreshold) {
            value = point;
            triggerSnapFeedback();  // Haptic-style visual feedback
            break;
        }
    }
}

void OscilSlider::triggerSnapFeedback() {
    // Brief scale pulse on the thumb
    thumbScale_ = 1.15f;
    startTimerHz(60);  // Animate back to 1.0
}
```

### Visual Feedback Delighters

```cpp
// Hover state with subtle scale
void OscilButton::mouseEnter(const juce::MouseEvent&) {
    isHovered_ = true;
    scaleSpring_.setTarget(1.02f);  // Subtle 2% scale up
    startTimerHz(60);
}

void OscilButton::mouseExit(const juce::MouseEvent&) {
    isHovered_ = false;
    scaleSpring_.setTarget(1.0f);
}

// Press feedback with depth
void OscilButton::mouseDown(const juce::MouseEvent&) {
    isPressed_ = true;
    scaleSpring_.setTarget(0.97f);   // Press inward
    brightnessOffset_ = -0.05f;      // Slightly darker
    repaint();
}

// Success celebration for toggles
void OscilToggle::setValue(bool on) {
    if (on && !value_) {
        // Brief "pop" animation on activation
        celebrationScale_.setTarget(1.15f);
        juce::Timer::callAfterDelay(100, [this] {
            celebrationScale_.setTarget(1.0f);
        });
    }
    value_ = on;
    // ...
}
```

### Rich Tooltips with Contextual Help

```cpp
struct RichTooltipContent {
    juce::String title;            // "Line Width"
    juce::String value;            // "2.0 px"
    juce::String shortcut;         // "Scroll to adjust"
    juce::String hint;             // "Double-click to reset"
    juce::Colour previewColour;    // For color controls
    bool showPreview = false;
};

class OscilTooltip : public juce::Component {
    RichTooltipContent content_;

    void paint(juce::Graphics& g) override {
        // Background with subtle shadow
        g.setColour(theme_.backgroundPane);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

        int y = 8;

        // Title (bold)
        g.setColour(theme_.textPrimary);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(content_.title, 8, y, getWidth() - 16, 16, juce::Justification::left);
        y += 18;

        // Value
        g.setFont(juce::Font(12.0f));
        g.drawText(content_.value, 8, y, getWidth() - 16, 14, juce::Justification::left);
        y += 16;

        // Shortcut hint (muted)
        if (content_.shortcut.isNotEmpty()) {
            g.setColour(theme_.textSecondary);
            g.setFont(juce::Font(11.0f));
            g.drawText(content_.shortcut, 8, y, getWidth() - 16, 14, juce::Justification::left);
        }
    }
};
```

### Optional Audio Feedback

User-configurable UI sounds for satisfying interactions:

```cpp
class UIAudioFeedback {
    static inline std::atomic<bool> enabled_{ false };

public:
    static void setEnabled(bool enabled) { enabled_ = enabled; }
    static bool isEnabled() { return enabled_; }

    static void playClick() {
        if (!enabled_) return;
        // Short 2ms click at -24dB
        synthesizeAndPlay(440.0f, 0.002f, 0.06f);
    }

    static void playToggleOn() {
        if (!enabled_) return;
        // Rising two-tone: 440Hz -> 660Hz
        synthesizeAndPlay(440.0f, 0.01f, 0.04f);
        synthesizeAndPlay(660.0f, 0.01f, 0.04f, 0.01f);
    }

    static void playToggleOff() {
        if (!enabled_) return;
        // Falling tone: 440Hz -> 330Hz
        synthesizeAndPlay(440.0f, 0.01f, 0.04f);
        synthesizeAndPlay(330.0f, 0.01f, 0.04f, 0.01f);
    }

    static void playError() {
        if (!enabled_) return;
        // Low buzz
        synthesizeAndPlay(150.0f, 0.05f, 0.08f);
    }

    static void playSnap() {
        if (!enabled_) return;
        // Brief tick
        synthesizeAndPlay(800.0f, 0.001f, 0.03f);
    }

private:
    static void synthesizeAndPlay(float freq, float duration, float amplitude,
                                  float delay = 0.0f);
};
```

### Hover Preview for Colors

Preview color changes before applying:

```cpp
class OscilColorSwatches : public juce::Component {
    juce::Colour previewColour_;
    bool showingPreview_ = false;

    void mouseMove(const juce::MouseEvent& e) override {
        int hoveredIndex = getSwatchIndexAt(e.position);
        if (hoveredIndex >= 0 && hoveredIndex != selectedIndex_) {
            previewColour_ = colours_[hoveredIndex];
            showingPreview_ = true;
            if (onPreviewChanged) onPreviewChanged(previewColour_);
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        showingPreview_ = false;
        if (onPreviewChanged) onPreviewChanged(colours_[selectedIndex_]);
    }

    std::function<void(juce::Colour)> onPreviewChanged;  // Live preview callback
};
```

---

## Directory Structure

```
include/ui/components/           # Component library headers
├── ComponentConstants.h         # Shared sizing/spacing constants
├── ComponentTypes.h             # Shared enums and structs
├── SpringAnimation.h            # Physics-based animation
├── AnimationSettings.h          # Reduced motion support
├── UIAudioFeedback.h            # Optional sound feedback
├── OscilButton.h
├── OscilTextField.h
├── OscilSlider.h
├── OscilToggle.h
├── OscilCheckbox.h
├── OscilRadioButton.h
├── OscilRadioGroup.h
├── OscilDropdown.h
├── OscilTabs.h
├── OscilAccordion.h
├── OscilBadge.h
├── OscilModal.h
├── OscilTooltip.h
├── OscilColorPicker.h
├── OscilColorSwatches.h
├── OscilMeterBar.h
└── OscilTransportSync.h

src/ui/components/              # Component implementations
├── SpringAnimation.cpp
├── AnimationSettings.cpp
├── UIAudioFeedback.cpp
├── OscilButton.cpp
├── OscilTextField.cpp
└── ... (matching .cpp files)
```

---

## Shared Constants

### ComponentConstants.h

```cpp
#pragma once

namespace oscil {
namespace ComponentLayout {
    // Sizing
    static constexpr int BUTTON_HEIGHT = 36;
    static constexpr int BUTTON_MIN_WIDTH = 80;
    static constexpr int INPUT_HEIGHT = 32;
    static constexpr int TOGGLE_WIDTH = 40;
    static constexpr int TOGGLE_HEIGHT = 20;
    static constexpr int CHECKBOX_SIZE = 18;
    static constexpr int ICON_BUTTON_SIZE = 32;
    static constexpr int SWATCH_SIZE = 24;
    static constexpr int TAB_HEIGHT = 36;
    static constexpr int BADGE_HEIGHT = 22;
    static constexpr int BADGE_COMPACT_HEIGHT = 24;
    static constexpr int TOOLTIP_MAX_WIDTH = 200;

    // Spacing (8px grid system)
    static constexpr int SPACING_XS = 4;
    static constexpr int SPACING_SM = 8;
    static constexpr int SPACING_MD = 12;
    static constexpr int SPACING_LG = 16;
    static constexpr int SPACING_XL = 24;

    // Border Radius
    static constexpr float RADIUS_SM = 4.0f;
    static constexpr float RADIUS_MD = 6.0f;   // Inputs
    static constexpr float RADIUS_LG = 8.0f;   // Buttons
    static constexpr float RADIUS_XL = 12.0f;  // Panels/Modals

    // Animation
    static constexpr int ANIMATION_FAST_MS = 100;
    static constexpr int ANIMATION_NORMAL_MS = 200;
    static constexpr int ANIMATION_SLOW_MS = 300;
    static constexpr int TOOLTIP_DELAY_MS = 500;
    static constexpr int HOVER_DELAY_MS = 150;

    // Interaction
    static constexpr float FINE_CONTROL_FACTOR = 0.1f;
    static constexpr float COARSE_CONTROL_FACTOR = 10.0f;
    static constexpr float MAGNETIC_SNAP_THRESHOLD = 0.02f;  // 2% of range

    // Focus
    static constexpr float FOCUS_RING_WIDTH = 2.0f;
    static constexpr float FOCUS_RING_OFFSET = 2.0f;
}
}
```

### ComponentTypes.h

```cpp
#pragma once

namespace oscil {
    // Button variants
    enum class ButtonVariant { Primary, Secondary, Danger, Ghost, Icon };

    // Text field variants
    enum class TextFieldVariant { Text, Search, Number };
    enum class TextFieldState { Empty, Focused, Filled, Error, Disabled };

    // Slider variants
    enum class SliderVariant { Single, Range, Vertical };

    // Checkbox state
    enum class CheckState { Unchecked, Checked, Indeterminate };

    // Badge styling
    enum class BadgeVariant { Filled, Outline };
    enum class BadgeColor { Default, Success, Warning, Error, Info };

    // Tab orientation
    enum class TabOrientation { Horizontal, Vertical };

    // Animation easing
    enum class EasingType { Linear, EaseOut, EaseInOut, Spring };

    // Tooltip position
    enum class TooltipPosition { Above, Below, Left, Right, Auto };
}
```

---

## Component Specifications

### 1. OscilButton

**Visual States:**
- Default: Base color, subtle bottom shadow
- Hover: Brightened background (+10% lightness), subtle scale (1.02x)
- Pressed: Inset shadow, darkened fill (-10% lightness), scale (0.97x)
- Disabled: 40% opacity, no interaction
- Focused: Focus ring with animated glow

**Variants:**
- Primary: Blue (#3B82F6) with white text
- Secondary: Slate (#1E293B) with border (#334155)
- Danger: Red (#EF4444) with white text
- Ghost: Transparent with blue text
- Icon: 32px square, centered glyph

**Data Model:**
```cpp
class OscilButton : public juce::Component, public ThemeManagerListener {
    ButtonVariant variant_ = ButtonVariant::Primary;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool isPressed_ = false;
    juce::String label_;
    juce::Image icon_;  // For Icon variant
    juce::KeyPress shortcutKey_;  // Optional keyboard shortcut

    // Animation state
    SpringAnimation scaleSpring_{ SpringPresets::SNAPPY };
    float brightnessOffset_ = 0.0f;

    std::function<void()> onClick;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    static constexpr int HEIGHT = 36;
    static constexpr int MIN_WIDTH = 80;
    static constexpr int ICON_SIZE = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilButton)
};
```

---

### 2. OscilTextField

**Visual States:**
- Empty: Placeholder "Enter value..." in muted gray (#64748B)
- Focused: Blue (#3B82F6) border glow, white cursor, focus ring
- Filled: White (#F8FAFC) text content
- Error: Red (#EF4444) border + inline error message below
- Disabled: Reduced opacity background

**Variants:**
- Text: Standard text input
- Search: Magnifying glass prefix icon
- Number: +/- stepper buttons, monospace value display

**Enhanced Features:**
- Scroll wheel support for Number variant
- Double-click to select all
- Cmd+A to select all
- Live validation with debounce

**Data Model:**
```cpp
class OscilTextField : public juce::Component, public ThemeManagerListener {
    TextFieldVariant variant_ = TextFieldVariant::Text;
    TextFieldState state_ = TextFieldState::Empty;
    juce::String value_;
    juce::String placeholder_ = "Enter value...";
    juce::String errorMessage_;

    // Number variant
    double numValue_ = 0.0;
    double defaultValue_ = 0.0;
    double minValue_ = 0.0;
    double maxValue_ = 100.0;
    double step_ = 1.0;
    int decimalPlaces_ = 0;

    std::unique_ptr<juce::TextEditor> editor_;
    std::unique_ptr<juce::Label> errorLabel_;
    std::unique_ptr<OscilButton> decrementButton_;
    std::unique_ptr<OscilButton> incrementButton_;

    // APVTS support
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

    std::function<void(const juce::String&)> onTextChanged;
    std::function<void(double)> onValueChanged;
    std::function<bool(const juce::String&)> validator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilTextField)
};
```

---

### 3. OscilSlider

**Visual States:**
- Default: Charcoal (#242438) track, blue (#3B82F6) fill
- Hover: Value tooltip floating above thumb, thumb scale 1.1x
- Dragging: Full opacity, no tooltip delay
- Disabled: Gray track, static thumb
- Focused: Focus ring around track

**Enhanced Features:**
- Magnetic snapping to 0%, 25%, 50%, 75%, 100%, default
- Alt+drag for fine control (0.1x sensitivity)
- Shift+drag for coarse control (10x sensitivity)
- Double-click to reset to default
- Scroll wheel support
- Value label scrubbing (drag on the value display)

**Data Model:**
```cpp
class OscilSlider : public juce::Component, public ThemeManagerListener {
    SliderVariant variant_ = SliderVariant::Single;
    bool enabled_ = true;
    bool showTooltip_ = true;

    double value_ = 0.0;
    double defaultValue_ = 0.0;
    double minValue_ = 0.0;
    double maxValue_ = 100.0;
    double step_ = 1.0;

    // Range variant
    double rangeStart_ = 0.0;
    double rangeEnd_ = 100.0;

    // Display
    juce::String label_;
    juce::String suffix_;  // "px", "khz", "%"
    int decimalPlaces_ = 1;

    // Magnetic snapping
    std::vector<double> magneticPoints_;
    bool enableMagneticSnap_ = true;

    // Animation
    SpringAnimation thumbScale_{ SpringPresets::SNAPPY };

    // APVTS support
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

    std::function<void(double)> onValueChanged;
    std::function<void(double, double)> onRangeChanged;
    std::function<juce::String(double)> formatValue;

    static constexpr int THUMB_SIZE = 14;
    static constexpr int TRACK_HEIGHT = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilSlider)
};
```

---

### 4. OscilToggle

**Visual States:**
- Off: Dark track (#334155), gray knob on left
- On: Green (#22C55E) track, white knob shifted right
- Disabled Off/On: Muted colors at reduced opacity
- Focused: Focus ring around track
- Spring-animated transition with slight overshoot

**Enhanced Features:**
- Space/Enter to toggle when focused
- Brief "pop" animation on activation
- Optional audio feedback

**Data Model:**
```cpp
class OscilToggle : public juce::Component,
                    public ThemeManagerListener,
                    private juce::Timer {
    bool value_ = false;
    bool enabled_ = true;
    juce::String label_;
    bool labelOnRight_ = true;

    // Animation
    SpringAnimation positionSpring_{ SpringPresets::BOUNCY };
    SpringAnimation celebrationScale_{ SpringPresets::SNAPPY };

    // APVTS support
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment_;

    std::function<void(bool)> onValueChanged;

    static constexpr int WIDTH = 40;
    static constexpr int HEIGHT = 20;
    static constexpr int KNOB_SIZE = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilToggle)
};
```

---

## Memory Safety Patterns

### 1. Ownership via unique_ptr
```cpp
// All child components owned via unique_ptr
std::unique_ptr<OscilButton> button_;
std::unique_ptr<juce::TextEditor> editor_;
```

### 2. No dangling pointers
```cpp
// Use SafePointer for cross-component references
juce::Component::SafePointer<OscilModal> activeModal_;

// Listeners unregister in destructor
~OscilComponent() override {
    ThemeManager::getInstance().removeListener(this);
}
```

### 3. Thread safety for audio->UI
```cpp
// Use atomic for audio thread data
std::atomic<float> pendingLevel_{ 0.0f };

// Use MessageManager for callbacks from other threads
juce::MessageManager::callAsync([this]() {
    // Safe UI update
});
```

### 4. RAII cleanup
```cpp
~OscilToggle() override {
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
    // unique_ptr members auto-cleaned
}
```

### 5. Prevent copying
```cpp
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentName)
```

---

## Implementation Phases

### Phase 1 - Foundation
1. ComponentConstants.h, ComponentTypes.h
2. SpringAnimation.h
3. AnimationSettings.h
4. OscilButton (with accessibility)
5. OscilTextField (with accessibility)
6. OscilToggle (with accessibility)
7. OscilTooltip (rich tooltips)

### Phase 2 - Form Controls
8. OscilSlider (with APVTS, magnetic snap)
9. OscilCheckbox
10. OscilRadioButton + OscilRadioGroup
11. OscilDropdown

### Phase 3 - Layout Components
12. OscilTabs
13. OscilAccordion
14. OscilBadge
15. OscilModal

### Phase 4 - Audio-Specific
16. OscilColorSwatches (with preview)
17. OscilColorPicker
18. OscilMeterBar (with OpenGL option)
19. OscilTransportSync
20. UIAudioFeedback

### Phase 5 - Integration
21. Refactor existing components
22. Update SegmentedButtonBar
23. Update OscillatorConfigPopup
24. Update sidebar sections
25. Add keyboard navigation map

---

## Testing Strategy

### Unit Tests
- State management (all state transitions)
- Callback invocation
- Validation logic
- Thread safety (MeterBar)
- Spring animation convergence
- Magnetic snapping behavior

### Accessibility Tests
- Screen reader announces all controls
- Keyboard navigation works through all components
- Focus is visible and moves logically
- ARIA-equivalent properties set correctly

### Visual Tests
- Snapshot tests via E2E test harness
- All visual states captured
- Theme change verification
- Animation smoothness (no jank)
- DPI scaling correctness

### Integration Tests
- Component composition
- Focus management
- Keyboard navigation
- APVTS parameter binding
- Plugin state persistence

---

## User Preferences (GlobalPreferences)

Add these settings to GlobalPreferences for user customization:

```cpp
class GlobalPreferences {
public:
    // Animation
    bool getReducedMotion() const;
    void setReducedMotion(bool reduced);

    // Audio feedback
    bool getUIClicksEnabled() const;
    void setUIClicksEnabled(bool enabled);

    // Interaction
    bool getMagneticSnapEnabled() const;
    void setMagneticSnapEnabled(bool enabled);

    float getFineControlSensitivity() const;  // Default 0.1
    void setFineControlSensitivity(float sensitivity);

    // Tooltips
    bool getShowTooltips() const;
    void setShowTooltips(bool show);

    int getTooltipDelay() const;  // ms
    void setTooltipDelay(int delay);
};
```

---

## Design Tokens Reference

### Colors (from ThemeManager)
| Token | Hex | Usage |
|-------|-----|-------|
| backgroundPrimary | #1A1A2E | Main canvas |
| backgroundSecondary | #242438 | Inputs |
| controlActive | #3B82F6 | Primary actions, focus |
| statusActive | #22C55E | Success/Toggle on |
| statusWarning | #F59E0B | Warning badges |
| statusError | #EF4444 | Error/Danger |
| textPrimary | #F8FAFC | Main text |
| textSecondary | #94A3B8 | Labels, hints |
| controlBorder | #334155 | Borders |

### Spacing (8px grid)
- XS: 4px
- SM: 8px
- MD: 12px
- LG: 16px
- XL: 24px

### Border Radius
- SM: 4px
- MD: 6px (inputs)
- LG: 8px (buttons)
- XL: 12px (panels)

### Component Sizing
- Button height: 36px
- Input height: 32px
- Toggle: 40x20px
- Checkbox: 18px
- Icon button: 32px
- Swatch: 24px

---

## References

- [UX/UI Design Trends 2025](https://userguiding.com/blog/ux-ui-trends)
- [JUCE 8 GPU Rendering](https://forum.juce.com/t/graphics-best-practice/42916)
- [JUCE Accessibility](https://www.kvraudio.com/news/juce-6-1-released---includes-accessibility-support-52875)
- [Microinteractions in UX](https://www.interaction-design.org/literature/article/micro-interactions-ux)
- [Spring Animation Physics](https://www.joshwcomeau.com/animation/a-friendly-introduction-to-spring-physics/)

---

## Usage Examples

Practical code patterns for using the Oscil component library.

### OscilButton

```cpp
#include "ui/components/OscilButton.h"

// In your component's header:
std::unique_ptr<oscil::OscilButton> saveButton_;
std::unique_ptr<oscil::OscilButton> deleteButton_;
std::unique_ptr<oscil::OscilButton> iconButton_;

// In constructor:
void setupButtons()
{
    // Primary action button
    saveButton_ = std::make_unique<oscil::OscilButton>("Save");
    saveButton_->setVariant(oscil::ButtonVariant::Primary);
    saveButton_->onClick = [this]() { handleSave(); };
    addAndMakeVisible(*saveButton_);

    // Danger button with shortcut
    deleteButton_ = std::make_unique<oscil::OscilButton>("Delete");
    deleteButton_->setVariant(oscil::ButtonVariant::Danger);
    deleteButton_->setShortcut(juce::KeyPress(juce::KeyPress::deleteKey));
    deleteButton_->onClick = [this]() { handleDelete(); };
    addAndMakeVisible(*deleteButton_);

    // Icon-only button
    iconButton_ = std::make_unique<oscil::OscilButton>(settingsIcon);
    iconButton_->setVariant(oscil::ButtonVariant::Ghost);
    iconButton_->setTooltip("Settings");
    iconButton_->onClick = [this]() { showSettings(); };
    addAndMakeVisible(*iconButton_);
}

// In resized():
void resized() override
{
    auto bounds = getLocalBounds().reduced(8);
    saveButton_->setBounds(bounds.removeFromRight(100).withHeight(36));
    deleteButton_->setBounds(bounds.removeFromRight(100).withHeight(36));
}
```

### OscilButton as Toggleable Segment

```cpp
// For segmented button bars (like TIME/MELODIC toggle)
button_->setToggleable(true);
button_->setVariant(oscil::ButtonVariant::Secondary);
button_->setSegmentPosition(oscil::SegmentPosition::First);  // First, Middle, Last, Only
button_->setButtonId(1);

button_->onToggle = [this](bool toggled) {
    if (toggled) handleModeSelected();
};
```

### OscilSlider

```cpp
#include "ui/components/OscilSlider.h"

// In your component's header:
std::unique_ptr<oscil::OscilSlider> lineWidthSlider_;
std::unique_ptr<oscil::OscilSlider> gainSlider_;

// In constructor:
void setupSliders()
{
    // Basic slider with label and suffix
    lineWidthSlider_ = std::make_unique<oscil::OscilSlider>();
    lineWidthSlider_->setLabel("Line Width");
    lineWidthSlider_->setRange(0.5, 5.0);
    lineWidthSlider_->setDefaultValue(2.0);
    lineWidthSlider_->setValue(2.0);
    lineWidthSlider_->setSuffix(" px");
    lineWidthSlider_->setDecimalPlaces(1);
    lineWidthSlider_->onValueChanged = [this](double value) {
        handleLineWidthChange(static_cast<float>(value));
    };
    addAndMakeVisible(*lineWidthSlider_);

    // Slider with custom magnetic snap points
    gainSlider_ = std::make_unique<oscil::OscilSlider>();
    gainSlider_->setLabel("Gain");
    gainSlider_->setRange(-24.0, 24.0);
    gainSlider_->setDefaultValue(0.0);
    gainSlider_->setSuffix(" dB");
    gainSlider_->setMagneticPoints({-12.0, -6.0, 0.0, 6.0, 12.0});
    gainSlider_->onValueChanged = [this](double dB) {
        handleGainChange(dB);
    };
    addAndMakeVisible(*gainSlider_);
}

// APVTS integration for DAW automation:
gainSlider_->attachToParameter(apvts, "gain");
```

### OscilToggle

```cpp
#include "ui/components/OscilToggle.h"

// In your component's header:
std::unique_ptr<oscil::OscilToggle> visibilityToggle_;
std::unique_ptr<oscil::OscilToggle> syncToggle_;

// In constructor:
void setupToggles()
{
    // Toggle with label
    visibilityToggle_ = std::make_unique<oscil::OscilToggle>("Visible");
    visibilityToggle_->setValue(true);
    visibilityToggle_->onValueChanged = [this](bool visible) {
        handleVisibilityChange(visible);
    };
    addAndMakeVisible(*visibilityToggle_);

    // Toggle without label
    syncToggle_ = std::make_unique<oscil::OscilToggle>();
    syncToggle_->setValue(false);
    syncToggle_->onValueChanged = [this](bool synced) {
        handleSyncChange(synced);
    };
    addAndMakeVisible(*syncToggle_);
}

// APVTS integration:
syncToggle_->attachToParameter(apvts, "hostSync");
```

### OscilDropdown

```cpp
#include "ui/components/OscilDropdown.h"

// In your component's header:
std::unique_ptr<oscil::OscilDropdown> paneSelector_;
std::unique_ptr<oscil::OscilDropdown> modeDropdown_;

// In constructor:
void setupDropdowns()
{
    // Simple string items
    paneSelector_ = std::make_unique<oscil::OscilDropdown>("Select Pane...");
    paneSelector_->addItems({"Pane 1", "Pane 2", "Pane 3"});
    paneSelector_->setSelectedIndex(0);
    paneSelector_->onSelectionChanged = [this](int index) {
        handlePaneChange(index);
    };
    addAndMakeVisible(*paneSelector_);

    // Items with IDs for programmatic selection
    modeDropdown_ = std::make_unique<oscil::OscilDropdown>();
    modeDropdown_->addItem({"stereo", "Full Stereo", "Both L+R channels"});
    modeDropdown_->addItem({"left", "Left Only", "Left channel only"});
    modeDropdown_->addItem({"right", "Right Only", "Right channel only"});
    modeDropdown_->addItem({"mid", "Mid (M/S)", "Mid signal from M/S"});
    modeDropdown_->onSelectionChangedId = [this](const juce::String& id) {
        handleModeChange(id);
    };
    addAndMakeVisible(*modeDropdown_);
}

// Searchable multi-select dropdown:
tagsDropdown_->setMultiSelect(true);
tagsDropdown_->setSearchable(true);
tagsDropdown_->onMultiSelectionChanged = [this](const std::set<int>& indices) {
    handleTagsChange(indices);
};
```

### OscilTabs

```cpp
#include "ui/components/OscilTabs.h"

// In your component's header:
std::unique_ptr<oscil::OscilTabs> settingsTabs_;

// In constructor:
void setupTabs()
{
    settingsTabs_ = std::make_unique<oscil::OscilTabs>();
    settingsTabs_->addTabs({"General", "Display", "Audio", "MIDI"});
    settingsTabs_->setSelectedIndex(0);
    settingsTabs_->onTabChanged = [this](int index) {
        showTabContent(index);
    };
    addAndMakeVisible(*settingsTabs_);
}

// Vertical tabs with pills variant:
sidebarTabs_ = std::make_unique<oscil::OscilTabs>(oscil::OscilTabs::Orientation::Vertical);
sidebarTabs_->setVariant(oscil::OscilTabs::Variant::Pills);
sidebarTabs_->setStretchTabs(true);

// Tabs with badges:
settingsTabs_->setTabBadge(2, 3);  // Show "3" badge on Audio tab
```

### OscilAccordion

```cpp
#include "ui/components/OscilAccordion.h"

// In your component's header:
std::unique_ptr<oscil::OscilAccordion> settingsAccordion_;
std::unique_ptr<juce::Component> displaySettings_;
std::unique_ptr<juce::Component> audioSettings_;

// In constructor:
void setupAccordion()
{
    settingsAccordion_ = std::make_unique<oscil::OscilAccordion>();
    settingsAccordion_->setAllowMultiExpand(false);  // Only one section open at a time

    // Add sections with content components
    auto* displaySection = settingsAccordion_->addSection("Display Settings", displaySettings_.get());
    displaySection->setExpanded(true);

    auto* audioSection = settingsAccordion_->addSection("Audio Settings", audioSettings_.get());
    audioSection->onExpandedChanged = [this](bool expanded) {
        if (expanded) refreshAudioDevices();
    };

    addAndMakeVisible(*settingsAccordion_);
}

// Control programmatically:
settingsAccordion_->expandSection(1);
settingsAccordion_->collapseAll();
```

### SegmentedButtonBar

```cpp
#include "ui/SegmentedButtonBar.h"

// In your component's header:
std::unique_ptr<oscil::SegmentedButtonBar> modeBar_;

// In constructor:
void setupSegmentedBar()
{
    modeBar_ = std::make_unique<oscil::SegmentedButtonBar>();
    modeBar_->addButton("TIME", 0);
    modeBar_->addButton("MELODIC", 1);
    modeBar_->setSelectedId(0);
    modeBar_->onSelectionChanged = [this](int id) {
        handleModeChange(id == 0 ? TimeMode : MelodicMode);
    };
    addAndMakeVisible(*modeBar_);
}

// In resized():
modeBar_->setBounds(bounds.removeFromTop(SegmentedButtonBar::PREFERRED_HEIGHT));
```

### Common Patterns

#### Registering as ThemeManagerListener

All Oscil components automatically register with ThemeManager. If you need theme updates in your parent component:

```cpp
class MyPanel : public juce::Component,
                public oscil::ThemeManagerListener
{
public:
    MyPanel()
    {
        oscil::ThemeManager::getInstance().addListener(this);
    }

    ~MyPanel() override
    {
        oscil::ThemeManager::getInstance().removeListener(this);
    }

    void themeChanged(const oscil::ColorTheme& newTheme) override
    {
        theme_ = newTheme;
        repaint();
    }

private:
    oscil::ColorTheme theme_;
};
```

#### Component Cleanup in Destructor

Components automatically unregister from ThemeManager and stop animations:

```cpp
~MyComponent() override
{
    // unique_ptr members auto-cleaned
    // ThemeManager listeners auto-removed by OscilButton, OscilSlider, etc.
}
```

#### Respecting Reduced Motion

When implementing custom animations, check user preferences:

```cpp
#include "ui/components/AnimationSettings.h"

void animateTransition()
{
    if (oscil::AnimationSettings::prefersReducedMotion())
    {
        // Snap immediately without animation
        position_ = targetPosition_;
        repaint();
    }
    else
    {
        // Use spring animation
        spring_.setTarget(targetPosition_);
        startTimerHz(60);
    }
}
```

#### Keyboard Navigation

Components support standard keyboard navigation:

| Component | Keys |
|-----------|------|
| OscilButton | Space/Enter to click |
| OscilSlider | Left/Right arrows, Home/End |
| OscilToggle | Space/Enter to toggle |
| OscilDropdown | Space to open, Up/Down to navigate, Enter to select, Escape to close |
| OscilTabs | Left/Right (horizontal), Up/Down (vertical) |
| OscilAccordion | Space/Enter to expand/collapse |
| SegmentedButtonBar | Left/Right arrows (with wrap) |

All components also support Tab navigation between them.
