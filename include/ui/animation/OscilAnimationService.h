/*
    Oscil - Animation Service
    Centralized animation management using JUCE 8's Animator system
    with VBlank synchronization for smooth, efficient animations
*/

#pragma once

#include <juce_animation/juce_animation.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/AnimationSettings.h"
#include <atomic>
#include <functional>
#include <optional>

namespace oscil
{

/**
 * Animation presets matching the feel of the old spring animations
 * but using fixed-duration easing for predictable performance
 */
namespace AnimationPresets
{
    // Snappy - Quick response, for button presses and quick state changes
    constexpr double SNAPPY_DURATION_MS = 120.0;
    
    // Smooth - Gentle motion, for panel transitions
    constexpr double SMOOTH_DURATION_MS = 200.0;
    
    // Modal - Entrance/exit for dialogs
    constexpr double MODAL_DURATION_MS = 180.0;
    
    // Hover - Quick feedback for hover states
    constexpr double HOVER_DURATION_MS = 100.0;
    
    // Expand - Accordion/dropdown expansion
    constexpr double EXPAND_DURATION_MS = 220.0;
    
    // Tab - Tab indicator movement
    constexpr double TAB_DURATION_MS = 180.0;
}

/**
 * OscilAnimationService provides a centralized animation system using JUCE 8's
 * VBlankAnimatorUpdater for smooth, VBlank-synchronized animations.
 *
 * Key features:
 * - Single VBlankAnimatorUpdater per editor for efficient updates
 * - Automatic reduced motion support
 * - Factory methods for common animation patterns
 * - No manual timers - animations are VBlank-synced
 * - Queue management with configurable limits
 *
 * Usage:
 * 1. Get the service from your component's editor
 * 2. Create an Animator using the builder methods
 * 3. Add it to the service and start it
 * 4. The animation runs automatically until complete
 */
class OscilAnimationService
{
public:
    /** Maximum number of queued/concurrent animations before dropping oldest */
    static constexpr size_t MAX_QUEUED_ANIMATIONS = 100;

    /** Maximum number of concurrent animators before logging warnings */
    static constexpr size_t MAX_ANIMATORS = 64;

    /** Maximum number of concurrent animators before rejecting new ones */
    static constexpr size_t HARD_LIMIT_ANIMATORS = 128;
    /**
     * Construct an animation service for a top-level component.
     * The component is used to synchronize with the display's VBlank.
     */
    explicit OscilAnimationService(juce::Component* topLevelComponent);
    
    ~OscilAnimationService();

    /**
     * Stop all running animations and release the VBlank updater.
     * Call this before destroying the service.
     */
    void stopAll();
    
    // Non-copyable, non-movable (owns the VBlankAnimatorUpdater)
    OscilAnimationService(const OscilAnimationService&) = delete;
    OscilAnimationService& operator=(const OscilAnimationService&) = delete;
    OscilAnimationService(OscilAnimationService&&) = delete;
    OscilAnimationService& operator=(OscilAnimationService&&) = delete;
    
    /**
     * Add an animator to be updated. The service holds a weak reference;
     * the caller owns the Animator and its lifetime.
     */
    void addAnimator(const juce::Animator& animator);
    
    /**
     * Remove an animator from the updater (optional - happens automatically
     * when the Animator is destroyed).
     */
    void removeAnimator(const juce::Animator& animator);
    
    // =========================================================================
    // Factory Methods - Create common animation patterns
    // =========================================================================
    
    /**
     * Create a fade animation (alpha 0->1 or 1->0)
     */
    juce::Animator createFadeIn(juce::Component& target, double durationMs = AnimationPresets::MODAL_DURATION_MS);
    juce::Animator createFadeOut(juce::Component& target, double durationMs = AnimationPresets::MODAL_DURATION_MS);
    
    /**
     * Create a value animation with callback
     * The callback receives values from 0.0 to 1.0 (or eased values)
     */
    juce::Animator createValueAnimation(
        double durationMs,
        std::function<void(float)> onValueChanged,
        std::function<float(float)> easing = juce::Easings::createEaseOut(),
        std::function<void()> onComplete = nullptr);
    
    /**
     * Create a "show" animation for modals/dialogs (scale + fade)
     */
    juce::Animator createModalShow(
        juce::Component& target,
        std::function<void()> onComplete = nullptr);
    
    /**
     * Create a "hide" animation for modals/dialogs
     */
    juce::Animator createModalHide(
        juce::Component& target,
        std::function<void()> onComplete = nullptr);
    
    /**
     * Create a hover animation (progress 0 -> 1)
     */
    juce::Animator createHoverAnimation(
        std::function<void(float)> onValueChanged,
        std::function<void()> onComplete = nullptr);

    /**
     * Create an expand/collapse animation for accordions (progress 0 -> 1)
     */
    juce::Animator createExpandAnimation(
        std::function<void(float)> onValueChanged,
        std::function<void()> onComplete = nullptr);

    /**
     * Create a tab indicator animation (progress 0 -> 1)
     */
    juce::Animator createTabAnimation(
        std::function<void(float)> onValueChanged,
        std::function<void()> onComplete = nullptr);
    
    // =========================================================================
    // Utility Methods
    // =========================================================================
    
    /**
     * Check if reduced motion is preferred
     */
    static bool shouldAnimate();
    
    /**
     * Get the adjusted duration based on reduced motion settings
     */
    static double getAdjustedDuration(double normalDurationMs);

    /**
     * Get the current number of active animators
     */
    size_t getAnimatorCount() const { return animatorCount_.load(); }

private:
    std::unique_ptr<juce::VBlankAnimatorUpdater> updater_;
    juce::Component* topLevelComponent_ = nullptr;

    /** Track number of active animators for queue management */
    std::atomic<size_t> animatorCount_{ 0 };

    /** Flag to track if we've already warned about high animator count */
    bool hasWarnedAboutLimit_ = false;
};

/**
 * Helper to get animation service from a component hierarchy.
 * Components should cache this pointer for efficiency.
 */
OscilAnimationService* findAnimationService(juce::Component* component);

/**
 * RAII wrapper for managing an animator's lifecycle.
 * Useful when you want to cancel an animation when a component is destroyed.
 */
class ScopedAnimator
{
public:
    ScopedAnimator() = default;
    explicit ScopedAnimator(juce::Animator animator) : animator_(std::move(animator)) {}
    
    ~ScopedAnimator()
    {
        if (animator_.has_value())
            animator_->complete();
    }
    
    // Move only
    ScopedAnimator(ScopedAnimator&&) = default;
    ScopedAnimator& operator=(ScopedAnimator&& other)
    {
        if (animator_.has_value())
            animator_->complete();
        animator_ = std::move(other.animator_);
        return *this;
    }
    
    ScopedAnimator(const ScopedAnimator&) = delete;
    ScopedAnimator& operator=(const ScopedAnimator&) = delete;
    
    void set(juce::Animator animator)
    {
        if (animator_.has_value())
            animator_->complete();
        animator_ = std::move(animator);
    }
    
    void start()
    {
        if (animator_.has_value())
            animator_->start();
    }
    
    void complete()
    {
        if (animator_.has_value())
            animator_->complete();
    }
    
    void reset()
    {
        if (animator_.has_value())
            animator_->complete();
        animator_.reset();
    }
    
    bool isActive() const
    {
        return animator_.has_value() && !animator_->isComplete();
    }
    
    juce::Animator* get() { return animator_.has_value() ? &*animator_ : nullptr; }
    const juce::Animator* get() const { return animator_.has_value() ? &*animator_ : nullptr; }

private:
    std::optional<juce::Animator> animator_;
};

} // namespace oscil

