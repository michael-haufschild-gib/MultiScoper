/*
    Oscil - Spring Animation
    Physics-based spring animation system for natural, responsive UI motion
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace oscil
{

/**
 * Physics-based spring animation
 *
 * Uses a critically damped spring model for smooth, natural motion.
 * Supports various presets from snappy to bouncy.
 */
struct SpringAnimation
{
    float position = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;

    // Spring parameters
    float stiffness = 300.0f; // Higher = faster/snappier
    float damping = 20.0f;    // Higher = less oscillation
    float mass = 1.0f;        // Higher = more momentum

    /**
     * Create a spring animation with default parameters
     */
    SpringAnimation() = default;

    /**
     * Create a spring animation with custom parameters
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
     * Update the spring physics
     * Call this once per frame with the time delta
     *
     * @param deltaTime Time since last update in seconds (e.g., 1/60 for 60fps)
     */
    void update(float deltaTime)
    {
        // Guard against invalid delta time
        if (deltaTime <= 0.0f)
            return;

        // Spring force: F = -k * x (Hooke's law)
        float displacement = position - target;
        float springForce = -stiffness * displacement;

        // Damping force: F = -c * v
        float dampingForce = -damping * velocity;

        // Total acceleration: a = F / m
        float acceleration = (springForce + dampingForce) / mass;

        // Update velocity and position (semi-implicit Euler)
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
    }

    /**
     * Check if the animation has essentially settled
     *
     * @param threshold How close to target counts as "settled"
     * @return true if position is close to target and velocity is near zero
     */
    bool isSettled(float threshold = 0.001f) const
    {
        return std::abs(position - target) < threshold && std::abs(velocity) < threshold;
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
        return from + (to - from) * t;
    }

    /**
     * Apply a brief impulse (for celebration effects, etc.)
     */
    void impulse(float amount) { velocity += amount; }
};

/**
 * Preset spring configurations for common use cases
 */
namespace SpringPresets
{
/**
 * Snappy - Quick response, minimal overshoot
 * Use for: Button presses, quick state changes
 */
inline SpringAnimation snappy() { return SpringAnimation(400.0f, 30.0f, 1.0f); }

/**
 * Bouncy - Playful with noticeable overshoot
 * Use for: Toggle celebrations, success states
 */
inline SpringAnimation bouncy() { return SpringAnimation(300.0f, 15.0f, 1.0f); }

/**
 * Smooth - Gentle, no overshoot
 * Use for: Panel transitions, layout animations
 */
inline SpringAnimation smooth() { return SpringAnimation(200.0f, 25.0f, 1.0f); }

/**
 * Gentle - Slow, dreamy motion
 * Use for: Background effects, ambient animations
 */
inline SpringAnimation gentle() { return SpringAnimation(150.0f, 20.0f, 1.0f); }

/**
 * Stiff - Very fast, almost instant
 * Use for: Hover states, micro-interactions
 */
inline SpringAnimation stiff() { return SpringAnimation(500.0f, 40.0f, 1.0f); }

/**
 * Wobbly - Fun, exaggerated motion
 * Use for: Playful UI, game-like interfaces
 */
inline SpringAnimation wobbly() { return SpringAnimation(250.0f, 10.0f, 1.0f); }
} // namespace SpringPresets

/**
 * Helper class for managing multiple spring animations
 * Useful for components with several animated properties
 */
class SpringAnimationGroup
{
public:
    SpringAnimation& add(const std::string& name, const SpringAnimation& spring = SpringPresets::snappy())
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
