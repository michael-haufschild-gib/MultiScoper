/*
    Oscil - Ease Animation
    Smooth exponential ease-out animation system for professional UI motion.
    No overshoot, no bounce — clean fade-in/fade-out transitions.

    Retained as SpringAnimation for API compatibility across the codebase.
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace oscil
{

/**
 * Exponential ease-out animation
 *
 * Produces smooth, monotonic transitions with no overshoot.
 * Position approaches target via exponential decay each frame.
 *
 * Retained as "SpringAnimation" for API compatibility.
 */
struct SpringAnimation
{
    float position = 0.0f;
    float velocity = 0.0f; // Unused — kept for struct layout compatibility
    float target = 0.0f;

    // Animation speed (0..1 range, higher = faster settling)
    // Derived from stiffness for backward compatibility: speed = stiffness / 1200
    float stiffness = 300.0f;
    float damping = 20.0f; // Unused — kept for constructor compatibility
    float mass = 1.0f;     // Unused — kept for constructor compatibility

    // Maximum delta time to prevent jumps after stalls/freezes
    static constexpr float MAX_DELTA_TIME = 1.0f / 15.0f; // ~67ms floor

    /**
     * Create an animation with default parameters
     */
    SpringAnimation() = default;

    /**
     * Create an animation with custom parameters
     * First arg (stiff) controls speed: higher = faster settling.
     * damping and mass are accepted for API compatibility but unused.
     */
    SpringAnimation(float stiff, float damp, float m = 1.0f) : stiffness(stiff), damping(damp), mass(m) {}

    /**
     * Set the target value (animation destination)
     */
    void setTarget(float newTarget) { target = newTarget; }

    /**
     * Set the target and immediately start from a specific position
     */
    void setTarget(float newTarget, float startPosition)
    {
        target = newTarget;
        position = startPosition;
        velocity = 0.0f;
    }

    /**
     * Update the animation — exponential ease-out
     * Call this once per frame with the time delta
     *
     * @param deltaTime Time since last update in seconds (e.g., 1/60 for 60fps)
     */
    void update(float deltaTime)
    {
        if (deltaTime <= 0.0f)
            return;
        deltaTime = std::min(deltaTime, MAX_DELTA_TIME);

        // Exponential ease-out: position moves toward target by a fraction each frame.
        // speed factor derived from stiffness (higher stiffness = faster approach)
        float const speed = std::clamp(stiffness / 1200.0f, 0.0f, 0.99f);
        float const factor = 1.0f - std::pow(1.0f - speed, deltaTime * 60.0f);
        position += (target - position) * factor;
        velocity = 0.0f;

        // Snap to target when very close to avoid asymptotic tail
        if (std::abs(position - target) < 0.0005f)
        {
            position = target;
        }
    }

    /**
     * Check if the animation has essentially settled
     *
     * @param threshold How close to target counts as "settled"
     * @return true if position is close to target
     */
    bool isSettled(float threshold = 0.001f) const { return std::abs(position - target) < threshold; }

    /**
     * Check if animation needs updating (not yet settled)
     */
    bool needsUpdate(float threshold = 0.001f) const { return !isSettled(threshold); }

    /**
     * Immediately snap to target (skip animation)
     */
    void snapToTarget()
    {
        position = target;
        velocity = 0.0f;
    }

    /**
     * Reset animation to initial state
     */
    void reset()
    {
        position = 0.0f;
        velocity = 0.0f;
        target = 0.0f;
    }

    /**
     * Get current value (clamped between 0 and 1 for normalized animations)
     */
    float getNormalized() const { return std::clamp(position, 0.0f, 1.0f); }

    /**
     * Get interpolated value between two values based on current position
     */
    template <typename T>
    T interpolate(const T& from, const T& to) const
    {
        float t = std::clamp(position, 0.0f, 1.0f);
        return from + ((to - from) * t);
    }
};

/**
 * Preset animation configurations
 */
namespace SpringPresets
{
/**
 * Fast — Quick response for hover states and micro-interactions
 */
inline SpringAnimation fast() { return {500.0f, 0.0f, 1.0f}; }

/**
 * Medium — Standard speed for state changes and transitions
 */
inline SpringAnimation medium() { return {350.0f, 0.0f, 1.0f}; }

/**
 * Slow — Gentle motion for layout and panel transitions
 */
inline SpringAnimation slow() { return {200.0f, 0.0f, 1.0f}; }

} // namespace SpringPresets

/**
 * Helper class for managing multiple animations
 * Useful for components with several animated properties
 */
class SpringAnimationGroup
{
public:
    SpringAnimation& add(const std::string& name, const SpringAnimation& spring = SpringPresets::medium())
    {
        springs_[name] = spring;
        return springs_[name];
    }

    SpringAnimation* get(const std::string& name)
    {
        auto it = springs_.find(name);
        return it != springs_.end() ? &it->second : nullptr;
    }

    const SpringAnimation* get(const std::string& name) const
    {
        auto it = springs_.find(name);
        return it != springs_.end() ? &it->second : nullptr;
    }

    void updateAll(float deltaTime)
    {
        for (auto& [name, spring] : springs_)
            spring.update(deltaTime);
    }

    bool anyNeedsUpdate(float threshold = 0.001f) const
    {
        for (const auto& [name, spring] : springs_)
            if (spring.needsUpdate(threshold))
                return true;
        return false;
    }

    void snapAllToTarget()
    {
        for (auto& [name, spring] : springs_)
            spring.snapToTarget();
    }

private:
    std::map<std::string, SpringAnimation> springs_;
};

} // namespace oscil
