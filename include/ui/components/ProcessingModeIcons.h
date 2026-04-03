/*
    Oscil - Processing Mode Icons
    Application-specific convenience helpers for creating audio processing mode icons
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil::ProcessingModeIcons
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
    float const circleRadius = size * 0.32f;
    float const overlap = size * 0.12f;
    float const centerY = size * 0.5f;
    float const leftCenterX = (size * 0.5f) - overlap;
    float const rightCenterX = (size * 0.5f) + overlap;

    // Create left circle outline
    juce::Path leftCircle;
    leftCircle.addEllipse(leftCenterX - circleRadius, centerY - circleRadius, circleRadius * 2, circleRadius * 2);

    // Create right circle outline
    juce::Path rightCircle;
    rightCircle.addEllipse(rightCenterX - circleRadius, centerY - circleRadius, circleRadius * 2, circleRadius * 2);

    // Convert to stroked paths (outlines)
    const juce::PathStrokeType stroke(strokeWidth);
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
    float const circleRadius = size * 0.32f;
    float const centerX = size * 0.5f;
    float const centerY = size * 0.5f;

    juce::Path circle;
    circle.addEllipse(centerX - circleRadius, centerY - circleRadius, circleRadius * 2, circleRadius * 2);

    // Convert to stroked path (outline)
    const juce::PathStrokeType stroke(strokeWidth);
    stroke.createStrokedPath(path, circle);

    return path;
}

/**
 * Create a Mid icon (M shape)
 * Represents the Mid component (L+R)/2
 */
inline juce::Path createMidIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    float const pad = size * 0.25f;
    float const w = size - (2 * pad);
    float const h = size - (2 * pad);
    float const x = pad;
    float const y = pad;

    path.startNewSubPath(x, y + h);
    path.lineTo(x, y);
    path.lineTo(x + (w * 0.5f), y + (h * 0.8f));
    path.lineTo(x + w, y);
    path.lineTo(x + w, y + h);

    juce::Path stroked;
    juce::PathStrokeType stroke(strokeWidth);
    stroke.setJointStyle(juce::PathStrokeType::mitered);
    stroke.createStrokedPath(stroked, path);

    return stroked;
}

/**
 * Create a Side icon (S shape)
 * Represents the Side component (L-R)/2
 */
inline juce::Path createSideIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    float const pad = size * 0.25f;
    float const w = size - (2 * pad);
    float const h = size - (2 * pad);
    float const x = pad;
    float const y = pad;

    // Simplified S shape (3 segments)
    path.startNewSubPath(x + w, y);
    path.lineTo(x, y);
    path.lineTo(x, y + (h * 0.5f));
    path.lineTo(x + w, y + (h * 0.5f));
    path.lineTo(x + w, y + h);
    path.lineTo(x, y + h);

    juce::Path stroked;
    juce::PathStrokeType stroke(strokeWidth);
    stroke.setJointStyle(juce::PathStrokeType::curved); // Use curved joins for S feel
    stroke.createStrokedPath(stroked, path);

    return stroked;
}

/**
 * Create a Left icon (L shape)
 * Represents the Left channel
 */
inline juce::Path createLeftIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    float const pad = size * 0.25f;
    float const w = size - (2 * pad);
    float const h = size - (2 * pad);
    float const x = pad;
    float const y = pad;

    path.startNewSubPath(x, y);
    path.lineTo(x, y + h);
    path.lineTo(x + w, y + h);

    juce::Path stroked;
    juce::PathStrokeType stroke(strokeWidth);
    stroke.setJointStyle(juce::PathStrokeType::mitered);
    stroke.createStrokedPath(stroked, path);

    return stroked;
}

/**
 * Create a Right icon (R shape)
 * Represents the Right channel
 */
inline juce::Path createRightIcon(float size, float strokeWidth = 1.5f)
{
    juce::Path path;

    float const pad = size * 0.25f;
    float const w = size - (2 * pad);
    float const h = size - (2 * pad);
    float const x = pad;
    float const y = pad;

    // Vertical line
    path.startNewSubPath(x, y + h);
    path.lineTo(x, y);

    // Loop (P shape)
    path.lineTo(x + (w * 0.8f), y);
    path.lineTo(x + (w * 0.8f), y + (h * 0.5f));
    path.lineTo(x, y + (h * 0.5f));

    // Leg
    path.startNewSubPath(x + (w * 0.2f), y + (h * 0.5f));
    path.lineTo(x + w, y + h);

    juce::Path stroked;
    juce::PathStrokeType stroke(strokeWidth);
    stroke.setJointStyle(juce::PathStrokeType::mitered);
    stroke.createStrokedPath(stroked, path);

    return stroked;
}

} // namespace oscil::ProcessingModeIcons
