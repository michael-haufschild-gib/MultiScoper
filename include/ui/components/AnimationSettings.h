/*
    Oscil - Animation Settings
    System-aware animation preferences including reduced motion support
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <cmath>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace oscil
{

/**
 * Animation settings that respect system accessibility preferences
 *
 * Provides utilities for:
 * - Detecting system "reduced motion" setting
 * - Adjusting animation durations based on preferences
 * - Respecting user's motion sensitivity
 */
class AnimationSettings
{
public:
    /**
     * Check if the user prefers reduced motion
     * Reads from OS accessibility settings on macOS/Windows
     *
     * @return true if animations should be minimized or disabled
     */
    static bool prefersReducedMotion()
    {
        // Check override first
        if (overrideReducedMotion_.load())
            return forceReducedMotion_.load();

        // Check cached value (updated periodically by apps that want real-time updates)
        // For simplicity, we just use app preference. Real OS detection would require
        // platform-specific code in a .mm file for macOS.
        return appPrefersReducedMotion_.load();
    }

    /**
     * Get animation duration adjusted for reduced motion preference
     *
     * @param normalDuration Duration in milliseconds for normal motion
     * @return 0 if reduced motion, otherwise normalDuration
     */
    static int getAnimationDuration(int normalDuration)
    {
        return prefersReducedMotion() ? 0 : normalDuration;
    }

    /**
     * Get animation duration as float seconds
     *
     * @param normalDurationMs Duration in milliseconds
     * @return Duration in seconds, 0 if reduced motion
     */
    static float getAnimationDurationSeconds(int normalDurationMs)
    {
        return prefersReducedMotion() ? 0.0f : (static_cast<float>(normalDurationMs) / 1000.0f);
    }

    /**
     * Check if spring animations should be used
     *
     * @return false if reduced motion is preferred
     */
    static bool shouldUseSpringAnimations()
    {
        return !prefersReducedMotion();
    }

    /**
     * Get a scale factor for animation intensity
     *
     * @return 0.0 for reduced motion, 1.0 for normal motion
     */
    static float getAnimationIntensity()
    {
        return prefersReducedMotion() ? 0.0f : 1.0f;
    }

    /**
     * Override system setting (for testing or app preferences)
     *
     * @param override true to use app setting instead of system
     * @param reduceMotion The value to use when override is true
     */
    static void setOverride(bool override, bool reduceMotion = true)
    {
        overrideReducedMotion_.store(override);
        forceReducedMotion_.store(reduceMotion);
    }

    /**
     * Clear any override, return to system setting
     */
    static void clearOverride()
    {
        overrideReducedMotion_.store(false);
    }

    /**
     * Set app-level preference (for platforms without system support)
     */
    static void setAppPreference(bool reduceMotion)
    {
        appPrefersReducedMotion_.store(reduceMotion);
    }

    // =========================================================================
    // Validation Methods - Ensure animation parameters are valid
    // =========================================================================

    /**
     * Validate that a duration value is valid for animations
     *
     * @param durationMs Duration in milliseconds
     * @return true if duration is valid (positive, finite, not NaN)
     */
    static bool validateDuration(double durationMs)
    {
        return durationMs >= 0.0 && !std::isnan(durationMs) && !std::isinf(durationMs);
    }

    /**
     * Sanitize a duration value, returning a safe default if invalid
     *
     * @param durationMs Duration in milliseconds
     * @param defaultMs Default value to use if invalid (default: 0.0)
     * @return Sanitized duration value
     */
    static double sanitizeDuration(double durationMs, double defaultMs = 0.0)
    {
        if (!validateDuration(durationMs))
            return defaultMs;
        return durationMs;
    }

    /**
     * Validate that an easing function is callable
     *
     * @param easingFn The easing function to validate
     * @return true if the easing function is valid and callable
     */
    static bool validateEasing(const std::function<float(float)>& easingFn)
    {
        return static_cast<bool>(easingFn);
    }

    /**
     * Get a safe easing function, using linear if the provided one is invalid
     *
     * @param easingFn The easing function to validate
     * @return The original function if valid, otherwise a linear easing function
     */
    static std::function<float(float)> safeEasing(std::function<float(float)> easingFn)
    {
        if (!easingFn)
            return [](float t) { return t; }; // Linear fallback
        return easingFn;
    }

    /**
     * Update from system preferences
     * Call this periodically or when app becomes active
     *
     * Platform support:
     * - macOS: Reads NSWorkspace accessibilityDisplayShouldReduceMotion
     * - Windows: Reads SPI_GETCLIENTAREAANIMATION
     * - Linux: Reads GNOME/KDE animation settings if available
     */
#if JUCE_MAC
    // macOS implementation is in AnimationSettings.mm
    static void updateFromSystem();
#else
    static void updateFromSystem()
    {
#if JUCE_WINDOWS
        // Windows: Check if client area animations are disabled
        BOOL animationsEnabled = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0);
        appPrefersReducedMotion_.store(!animationsEnabled);
#elif JUCE_LINUX
        // Linux: Check GNOME reduce-motion setting via gsettings
        // For now, default to false; proper implementation would use GSettings
        // gsettings get org.gnome.desktop.interface enable-animations
        // Just use the app preference for now
#endif
    }
#endif

private:
    static inline std::atomic<bool> overrideReducedMotion_{ false };
    static inline std::atomic<bool> forceReducedMotion_{ false };
    static inline std::atomic<bool> appPrefersReducedMotion_{ false };
};

/**
 * RAII helper for temporarily disabling animations
 * Useful during batch operations or initial layout
 */
class ScopedReducedMotion
{
public:
    ScopedReducedMotion()
    {
        AnimationSettings::setOverride(true, true);
    }

    ~ScopedReducedMotion()
    {
        AnimationSettings::clearOverride();
    }

    // Non-copyable
    ScopedReducedMotion(const ScopedReducedMotion&) = delete;
    ScopedReducedMotion& operator=(const ScopedReducedMotion&) = delete;
};

/**
 * Animation helper that respects reduced motion
 * Provides common animation patterns with automatic preference handling
 */
namespace AnimationHelper
{
    /**
     * Perform an action with animation if allowed, instantly otherwise
     *
     * @param animated Function to call if animations are enabled
     * @param instant Function to call if animations are disabled
     */
    template<typename AnimatedFn, typename InstantFn>
    void performWithMotionPreference(AnimatedFn animated, InstantFn instant)
    {
        if (AnimationSettings::prefersReducedMotion())
            instant();
        else
            animated();
    }

    /**
     * Clamp progress value to valid range [0.0, 1.0]
     * Handles edge cases where t < 0, t > 1, or t is NaN
     *
     * @param progress Raw progress value
     * @return Clamped progress in [0.0, 1.0]
     */
    inline float clampProgress(float progress)
    {
        // Handle NaN by returning 0
        if (std::isnan(progress))
            return 0.0f;
        // Clamp to valid range
        return std::max(0.0f, std::min(1.0f, progress));
    }

    /**
     * Linear interpolation with reduced motion awareness
     *
     * @param from Starting value
     * @param to Ending value
     * @param progress 0.0 to 1.0 animation progress (clamped internally)
     * @return Interpolated value, or 'to' if reduced motion
     */
    template<typename T>
    T lerp(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        float t = clampProgress(progress);
        return from + (to - from) * t;
    }

    /**
     * Ease-out interpolation with reduced motion awareness
     * Handles edge cases: t=0 returns from, t=1 returns to, t>1 clamped to 1
     */
    template<typename T>
    T easeOut(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        float t = clampProgress(progress);
        // Handle exact edge cases for precision
        if (t <= 0.0f) return from;
        if (t >= 1.0f) return to;
        float eased = 1.0f - std::pow(1.0f - t, 3.0f);
        return from + (to - from) * eased;
    }

    /**
     * Ease-in-out interpolation with reduced motion awareness
     * Handles edge cases: t=0 returns from, t=1 returns to, t>1 clamped to 1
     */
    template<typename T>
    T easeInOut(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        float t = clampProgress(progress);
        // Handle exact edge cases for precision
        if (t <= 0.0f) return from;
        if (t >= 1.0f) return to;
        float eased = t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        return from + (to - from) * eased;
    }
}

} // namespace oscil
