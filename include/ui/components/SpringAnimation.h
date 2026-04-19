/*
    MultiScoper - Animation System
    Dual-mode animation: exponential ease-out (default) and real spring physics.

    EaseOut mode: smooth monotonic transitions with no overshoot.
    Spring mode: semi-implicit Euler integration with configurable stiffness/damping/mass,
    producing natural overshoot for underdamped parameters.

    Retained as SpringAnimation for API compatibility across the codebase.
*/

#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <map>

namespace multiscoper
{

/**
 * Animation mode selector
 *
 * EaseOut — exponential decay, no overshoot (default)
 * Spring  — semi-implicit Euler spring physics, supports overshoot
 */
enum class SpringMode
{
    EaseOut,
    Spring
};

/**
 * Dual-mode animation: ease-out or spring physics
 *
 * EaseOut mode (default): smooth monotonic transitions with no overshoot.
 * Position approaches target via exponential decay each frame.
 *
 * Spring mode: real spring physics using semi-implicit Euler integration.
 * Stiffness, damping, and mass control the motion. Underdamped parameters
 * produce natural overshoot.
 *
 * Retained as "SpringAnimation" for API compatibility.
 */
struct SpringAnimation
{
    float position = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;

    // EaseOut: speed derived from stiffness (speed = stiffness / 1200)
    // Spring: stiffness controls spring constant, damping controls friction, mass controls inertia
    float stiffness = 300.0f;
    float damping = 20.0f;
    float mass = 1.0f;

    SpringMode mode = SpringMode::EaseOut;

    // Maximum delta time to prevent jumps after stalls/freezes
    static constexpr float MAX_DELTA_TIME = 1.0f / 15.0f; // ~67ms floor

    /**
     * Create an animation with default parameters
     */
    SpringAnimation() = default;

    /**
     * Create an animation with custom parameters
     * EaseOut mode: stiff controls speed (higher = faster settling); damp and m are unused.
     * Spring mode: stiff = spring constant, damp = friction, m = inertia.
     */
    SpringAnimation(float stiff, float damp, float m = 1.0f) : stiffness(stiff), damping(damp), mass(m) {}

    /**
     * Set the animation mode (EaseOut or Spring)
     */
    void setMode(SpringMode m) { mode = m; }

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
     * Update the animation — dispatches to ease-out or spring based on mode
     * Call this once per frame with the time delta
     *
     * @param deltaTime Time since last update in seconds (e.g., 1/60 for 60fps)
     */
    void update(float deltaTime)
    {
        if (mode == SpringMode::Spring)
            updateSpring(deltaTime);
        else
            updateEaseOut(deltaTime);
    }

    /**
     * Check if the animation has essentially settled
     *
     * @param threshold How close to target counts as "settled"
     * @return true if position is close to target
     */
    bool isSettled(float threshold = 0.001f) const
    {
        if (mode == SpringMode::Spring)
            return std::abs(position - target) < threshold && std::abs(velocity) < 0.01f;
        return std::abs(position - target) < threshold;
    }

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

private:
    /**
     * Exponential ease-out update — smooth monotonic approach to target
     */
    void updateEaseOut(float deltaTime)
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
     * Spring physics update — semi-implicit Euler integration
     * Produces natural overshoot for underdamped parameters (stiffness >> damping)
     */
    void updateSpring(float deltaTime)
    {
        if (deltaTime <= 0.0f)
            return;
        deltaTime = std::min(deltaTime, MAX_DELTA_TIME);

        const float displacement = position - target;
        const float springForce = -stiffness * displacement;
        const float dampingForce = -damping * velocity;
        jassert(mass > 0.0f);
        const float safeMass = mass > 0.0f ? mass : 1.0f;
        const float acceleration = (springForce + dampingForce) / safeMass;

        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;

        // Settle when both position and velocity are negligible
        if (std::abs(position - target) < 0.0005f && std::abs(velocity) < 0.01f)
        {
            position = target;
            velocity = 0.0f;
        }
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

/// Switch/toggle thumb — fast with slight overshoot (matches web motion/react stiffness:700 damping:30)
inline SpringAnimation springSwitch()
{
    SpringAnimation s{700.0f, 30.0f, 1.0f};
    s.setMode(SpringMode::Spring);
    return s;
}

/// Sliding indicator — medium spring for segmented controls and tab underlines
inline SpringAnimation springIndicator()
{
    SpringAnimation s{500.0f, 30.0f, 1.0f};
    s.setMode(SpringMode::Spring);
    return s;
}

/// Popup/dropdown entrance — softer spring for modal/dropdown open animations
inline SpringAnimation springPopup()
{
    SpringAnimation s{400.0f, 25.0f, 1.0f};
    s.setMode(SpringMode::Spring);
    return s;
}

/// Gentle spring — for subtle hover effects and micro-interactions
inline SpringAnimation springGentle()
{
    SpringAnimation s{300.0f, 20.0f, 1.0f};
    s.setMode(SpringMode::Spring);
    return s;
}

} // namespace SpringPresets

/**
 * Helper class for managing multiple animations
 * Useful for components with several animated properties
 */
class SpringAnimationGroup
{
public:
    SpringAnimation& add(const juce::String& name, const SpringAnimation& spring = SpringPresets::medium())
    {
        springs_[name] = spring;
        return springs_[name];
    }

    SpringAnimation* get(const juce::String& name)
    {
        auto it = springs_.find(name);
        return it != springs_.end() ? &it->second : nullptr;
    }

    const SpringAnimation* get(const juce::String& name) const
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
    std::map<juce::String, SpringAnimation> springs_;
};

} // namespace multiscoper
