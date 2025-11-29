/*
    Oscil - Particle
    Core particle data structures for the particle system
*/

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "rendering/VisualConfiguration.h"

namespace oscil
{

/**
 * Single particle data.
 */
struct Particle
{
    // Position in screen space (pixels)
    float x = 0.0f;
    float y = 0.0f;

    // Velocity in pixels per second
    float vx = 0.0f;
    float vy = 0.0f;

    // Color (RGBA)
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    // Size in pixels
    float size = 4.0f;

    // Life (0.0 = dead, 1.0 = just born)
    float life = 1.0f;
    float maxLife = 2.0f;

    // Rotation (radians)
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;

    /**
     * Check if particle is alive.
     */
    [[nodiscard]] bool isAlive() const { return life > 0.0f; }

    /**
     * Get normalized age (0 = just born, 1 = about to die).
     */
    [[nodiscard]] float getAge() const
    {
        return maxLife > 0.0f ? 1.0f - (life / maxLife) : 1.0f;
    }

    /**
     * Update particle physics.
     * @param deltaTime Time step in seconds
     * @param gravity Gravity in pixels/sec^2
     * @param drag Velocity damping (0-1 per second)
     */
    void update(float deltaTime, float gravity = 0.0f, float drag = 0.0f)
    {
        // Apply gravity
        vy += gravity * deltaTime;

        // Apply drag
        float dragFactor = 1.0f - (drag * deltaTime);
        vx *= dragFactor;
        vy *= dragFactor;

        // Update position
        x += vx * deltaTime;
        y += vy * deltaTime;

        // Update rotation
        rotation += rotationSpeed * deltaTime;

        // Update life
        life -= deltaTime;
        if (life < 0.0f)
            life = 0.0f;
    }

    /**
     * Initialize particle with position and velocity.
     */
    void spawn(float px, float py, float pvx, float pvy,
               float psize, float plife, juce::Colour colour)
    {
        x = px;
        y = py;
        vx = pvx;
        vy = pvy;
        size = psize;
        life = plife;
        maxLife = plife;
        r = colour.getFloatRed();
        g = colour.getFloatGreen();
        b = colour.getFloatBlue();
        a = colour.getFloatAlpha();
        rotation = 0.0f;
        rotationSpeed = 0.0f;
    }
};

// ParticleEmissionMode and ParticleBlendMode are defined in VisualConfiguration.h

} // namespace oscil
