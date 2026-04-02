/*
    Oscil - Component Types
    Shared enums and type definitions for the UI component library
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>
#include <utility>

namespace oscil
{

// Button variants
enum class ButtonVariant : std::uint8_t
{
    Primary,   // Blue filled button for primary actions
    Secondary, // Dark filled button (formerly with border, now borderless)
    Tertiary,  // Subtle button (similar to Ghost)
    Danger,    // Red filled button for destructive actions
    Ghost,     // Transparent with text only
    Icon       // Square icon-only button
};

// Button states (for internal use)
enum class ButtonState : std::uint8_t
{
    Default,
    Hovered,
    Pressed,
    Disabled,
    Focused
};

// Text field variants
enum class TextFieldVariant : std::uint8_t
{
    Text,   // Standard text input
    Search, // With search icon prefix
    Number  // Numeric with stepper buttons
};

// Text field states
enum class TextFieldState : std::uint8_t
{
    Empty,
    Focused,
    Filled,
    Error,
    Disabled
};

// Slider variants
enum class SliderVariant : std::uint8_t
{
    Single,  // Standard single-value slider
    Range,   // Dual-handle range slider
    Vertical // Vertical orientation
};

// Checkbox state (tri-state support)
enum class CheckState : std::uint8_t
{
    Unchecked,
    Checked,
    Indeterminate
};

// Badge visual style
enum class BadgeVariant : std::uint8_t
{
    Filled, // Solid background
    Outline // Border only
};

// Badge colors
enum class BadgeColor : std::uint8_t
{
    Default, // Blue
    Success, // Green
    Warning, // Yellow/Orange
    Error,   // Red
    Info     // Light blue
};

// Tab orientation
enum class TabOrientation : std::uint8_t
{
    Horizontal,
    Vertical
};

// Dropdown variants
enum class DropdownVariant : std::uint8_t
{
    Single,    // Single selection
    Multi,     // Multiple selection
    Searchable // With search filter
};

// Tooltip position preference
enum class TooltipPosition : std::uint8_t
{
    Above,
    Below,
    Left,
    Right,
    Auto // Automatically determine best position
};

// Modal size presets
enum class ModalSize : std::uint8_t
{
    Small,     // 280px / 320px
    Medium,    // 400px / 480px
    Large,     // 560px / 640px
    Auto,      // Fit content
    FullScreen // Full parent bounds with margin
};

// Animation easing types
enum class EasingType : std::uint8_t
{
    Linear,
    EaseOut,
    EaseIn,
    EaseInOut,
    Spring // Physics-based spring animation
};

// Segment position for segmented controls
enum class SegmentPosition : std::uint8_t
{
    None,   // Standalone button (all corners rounded)
    First,  // First segment (left corners rounded)
    Middle, // Middle segment (no corners rounded)
    Last,   // Last segment (right corners rounded)
    Only    // Single segment (all corners rounded)
};

// Meter bar orientation
enum class MeterOrientation : std::uint8_t
{
    Horizontal,
    Vertical
};

// Transport state for sync component
enum class TransportState : std::uint8_t
{
    Stopped,
    Playing,
    Paused
};

// Common callback types
namespace Callbacks
{
using VoidCallback = std::function<void()>;
using BoolCallback = std::function<void(bool)>;
using IntCallback = std::function<void(int)>;
using FloatCallback = std::function<void(float)>;
using DoubleCallback = std::function<void(double)>;
using StringCallback = std::function<void(const juce::String&)>;
using ColourCallback = std::function<void(juce::Colour)>;
using RangeCallback = std::function<void(double, double)>;
using ValidationCallback = std::function<bool(const juce::String&)>;
using FormatCallback = std::function<juce::String(double)>;
} // namespace Callbacks

// Note: DropdownItem is defined in ui/components/dropdown/DropdownTypes.h

// Rich tooltip content
struct RichTooltipContent
{
    juce::String title;
    juce::String value;
    juce::String shortcut;
    juce::String hint;
    juce::Colour previewColour;
    bool showColourPreview = false;

    RichTooltipContent() = default;
    RichTooltipContent(juce::String t, juce::String v = {}) : title(std::move(t)), value(std::move(v)) {}
};

// Validation result
struct ValidationResult
{
    bool isValid = true;
    juce::String errorMessage;

    static ValidationResult valid() { return {.isValid = true, .errorMessage = {}}; }
    static ValidationResult invalid(const juce::String& message) { return {.isValid = false, .errorMessage = message}; }
};

} // namespace oscil
