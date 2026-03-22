/*
    Oscil - Animation Settings
    System-aware animation preferences including reduced motion support
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

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
    /// Update animation preference from system accessibility settings (macOS .mm).
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
     * Linear interpolation with reduced motion awareness
     *
     * @param from Starting value
     * @param to Ending value
     * @param progress 0.0 to 1.0 animation progress
     * @return Interpolated value, or 'to' if reduced motion
     */
    template<typename T>
    T lerp(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        return from + (to - from) * progress;
    }

    /**
     * Ease-out interpolation with reduced motion awareness
     */
    template<typename T>
    T easeOut(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        float eased = 1.0f - std::pow(1.0f - progress, 3.0f);
        return from + (to - from) * eased;
    }

    /**
     * Ease-in-out interpolation with reduced motion awareness
     */
    template<typename T>
    T easeInOut(const T& from, const T& to, float progress)
    {
        if (AnimationSettings::prefersReducedMotion())
            return to;
        float eased = progress < 0.5f
            ? 4.0f * progress * progress * progress
            : 1.0f - std::pow(-2.0f * progress + 2.0f, 3.0f) / 2.0f;
        return from + (to - from) * eased;
    }
}

} // namespace oscil
