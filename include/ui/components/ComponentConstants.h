/*
    Oscil - Component Constants
    Shared sizing, spacing, and animation constants for the UI component library
*/

#pragma once

namespace oscil
{
namespace ComponentLayout
{
// Component Sizing
static constexpr int BUTTON_HEIGHT = 36;
static constexpr int BUTTON_MIN_WIDTH = 80;
static constexpr int BUTTON_ICON_SIZE = 32;
static constexpr int INPUT_HEIGHT = 32;
static constexpr int TOGGLE_WIDTH = 40;
static constexpr int TOGGLE_HEIGHT = 20;
static constexpr int TOGGLE_KNOB_SIZE = 16;
static constexpr int CHECKBOX_SIZE = 18;
static constexpr int RADIO_SIZE = 18;
static constexpr int SLIDER_THUMB_SIZE = 14;
static constexpr int SLIDER_TRACK_HEIGHT = 4;
static constexpr int SWATCH_SIZE = 24;
static constexpr int TAB_HEIGHT = 36;
static constexpr int BADGE_HEIGHT = 22;
static constexpr int BADGE_COMPACT_HEIGHT = 24;
static constexpr int TOOLTIP_MAX_WIDTH = 200;
static constexpr int DROPDOWN_MAX_HEIGHT = 300;
static constexpr int MODAL_MIN_WIDTH = 280;

// Typography
static constexpr float FONT_SIZE_CAPTION = 11.0f;
static constexpr float FONT_SIZE_SMALL = 12.0f;
static constexpr float FONT_SIZE_DEFAULT = 14.0f;
static constexpr float FONT_SIZE_HEADER = 16.0f;

// Button Internal Layout
static constexpr int BUTTON_SEGMENT_PADDING = 6;
static constexpr float BUTTON_NARROW_PADDING_RATIO = 0.15f;

// Spacing (8px grid system)
static constexpr int SPACING_XS = 4;
static constexpr int SPACING_SM = 8;
static constexpr int SPACING_MD = 12;
static constexpr int SPACING_LG = 16;
static constexpr int SPACING_XL = 24;
static constexpr int SPACING_XXL = 32;

// Border Radius
static constexpr float RADIUS_XS = 2.0f;
static constexpr float RADIUS_SM = 4.0f;
static constexpr float RADIUS_MD = 6.0f;      // Inputs, checkboxes
static constexpr float RADIUS_LG = 8.0f;      // Buttons
static constexpr float RADIUS_XL = 12.0f;     // Panels, modals
static constexpr float RADIUS_FULL = 9999.0f; // Fully rounded (pills, toggles)

// Border Widths
static constexpr float BORDER_THIN = 1.0f;
static constexpr float BORDER_MEDIUM = 1.5f;
static constexpr float BORDER_THICK = 2.0f;

// Focus Ring - WCAG 2.1 requires 3:1 contrast for focus indicators
static constexpr float FOCUS_RING_WIDTH = 2.0f;
static constexpr float FOCUS_RING_OFFSET = 3.0f; // Increased for better separation from borders
static constexpr float FOCUS_RING_ALPHA = 0.75f; // Higher alpha for better contrast in all modes

// Animation Timing (milliseconds)
static constexpr int ANIMATION_INSTANT_MS = 0;
static constexpr int ANIMATION_FAST_MS = 100;
static constexpr int ANIMATION_NORMAL_MS = 200;
static constexpr int ANIMATION_SLOW_MS = 300;
static constexpr int ANIMATION_VERY_SLOW_MS = 500;

// Delays (milliseconds)
static constexpr int TOOLTIP_DELAY_MS = 500;
static constexpr int HOVER_DELAY_MS = 150;
static constexpr int DEBOUNCE_DELAY_MS = 100;
static constexpr int DOUBLE_CLICK_MS = 400;

// Interaction Multipliers
static constexpr float FINE_CONTROL_FACTOR = 0.1f;      // Alt+drag
static constexpr float COARSE_CONTROL_FACTOR = 10.0f;   // Shift+drag
static constexpr float MAGNETIC_SNAP_THRESHOLD = 0.02f; // 2% of range

// Visual Feedback
static constexpr float HOVER_SCALE = 1.02f;
static constexpr float PRESS_SCALE = 0.97f;
static constexpr float CELEBRATION_SCALE = 1.15f;
static constexpr float HOVER_BRIGHTNESS_OFFSET = 0.1f;
static constexpr float PRESS_BRIGHTNESS_OFFSET = -0.05f;
static constexpr float DISABLED_OPACITY = 0.4f;

// Z-Index Layers (for overlays)
static constexpr int Z_INDEX_BASE = 0;
static constexpr int Z_INDEX_DROPDOWN = 100;
static constexpr int Z_INDEX_TOOLTIP = 200;
static constexpr int Z_INDEX_MODAL = 300;
static constexpr int Z_INDEX_TOAST = 400;

// Timer Refresh Rates
static constexpr int ANIMATION_FPS = 60;
static constexpr int METER_FPS = 30;
} // namespace ComponentLayout

// Animation Frame Rate Helper
namespace AnimationTiming
{
static constexpr float FRAME_DURATION_60FPS = 1.0f / 60.0f;
static constexpr float FRAME_DURATION_30FPS = 1.0f / 30.0f;

// Convert milliseconds to frame duration
static constexpr float msToSeconds(int ms) { return static_cast<float>(ms) / 1000.0f; }
} // namespace AnimationTiming

} // namespace oscil
