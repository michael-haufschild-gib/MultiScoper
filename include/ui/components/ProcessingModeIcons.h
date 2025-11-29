/*
    Oscil - Processing Mode Icons
    Application-specific convenience helpers for creating audio processing mode icons
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{
namespace ProcessingModeIcons
{

/**
 * Create a stereo icon (two overlapping circle outlines like a Venn diagram)
 * Represents stereo/linked channels
 * @param size The overall size of the icon (width and height)
 * @param strokeWidth Width of the circle outline stroke
 */
inline juce::Path createStereoIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    // Two overlapping circle outlines representing stereo channels
    float circleRadius = size * 0.32f;
    float overlap = size * 0.12f;
    float centerY = size * 0.5f;
    float leftCenterX = size * 0.5f - overlap;
    float rightCenterX = size * 0.5f + overlap;

    // Create left circle outline
    juce::Path leftCircle;
    leftCircle.addEllipse(leftCenterX - circleRadius, centerY - circleRadius,
                          circleRadius * 2, circleRadius * 2);

    // Create right circle outline
    juce::Path rightCircle;
    rightCircle.addEllipse(rightCenterX - circleRadius, centerY - circleRadius,
                           circleRadius * 2, circleRadius * 2);

    // Convert to stroked paths (outlines)
    juce::PathStrokeType stroke(strokeWidth);
    stroke.createStrokedPath(path, leftCircle);

    juce::Path rightStroked;
    stroke.createStrokedPath(rightStroked, rightCircle);
    path.addPath(rightStroked);

    return path;
}

/**
 * Create a mono icon (single circle outline)
 * Represents a mono/summed signal
 * @param size The overall size of the icon (width and height)
 * @param strokeWidth Width of the circle outline stroke
 */
inline juce::Path createMonoIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    // Single centered circle outline representing mono signal
    float circleRadius = size * 0.32f;
    float centerX = size * 0.5f;
    float centerY = size * 0.5f;

    juce::Path circle;
    circle.addEllipse(centerX - circleRadius, centerY - circleRadius,
                      circleRadius * 2, circleRadius * 2);

    // Convert to stroked path (outline)
    juce::PathStrokeType stroke(strokeWidth);
    stroke.createStrokedPath(path, circle);

    return path;
}

} // namespace ProcessingModeIcons
} // namespace oscil
