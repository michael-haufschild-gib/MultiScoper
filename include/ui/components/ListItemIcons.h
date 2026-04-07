/*
    Oscil - List Item Icons
    SVG-based icons for oscillator list item buttons (delete, settings, visibility)
    Loaded from BinaryData resources for theme-aware coloring
*/

#pragma once

#include "BinaryData.h"

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil::ListItemIcons
{

/**
 * Load an SVG from BinaryData and create a Drawable
 * The Drawable can be rendered at any size and recolored for themes
 */
inline std::unique_ptr<juce::Drawable> loadSvgDrawable(const char* data, int dataSize)
{
    auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, dataSize));
    if (xml != nullptr)
    {
        return juce::Drawable::createFromSVG(*xml);
    }
    return nullptr;
}

/**
 * Create a path from SVG drawable, scaled to target size
 * This allows the icon to be rendered with theme colors via OscilButton::setIconPath()
 */
inline juce::Path getPathFromDrawable(juce::Drawable* drawable, float targetSize)
{
    if (drawable == nullptr)
        return {};

    juce::Path path;

    // Get the drawable's bounds
    auto bounds = drawable->getDrawableBounds();
    if (bounds.isEmpty())
        return {};

    // Calculate scale to fit target size with padding
    float const padding = targetSize * 0.15f;
    float const availableSize = targetSize - (padding * 2);
    float const scale = availableSize / juce::jmax(bounds.getWidth(), bounds.getHeight());

    // Get path from drawable and transform it
    if (auto* drawablePath = dynamic_cast<juce::DrawablePath*>(drawable))
    {
        path = drawablePath->getPath();
    }
    else if (auto* composite = dynamic_cast<juce::DrawableComposite*>(drawable))
    {
        // Combine paths from all children
        for (int i = 0; i < composite->getNumChildComponents(); ++i)
        {
            if (auto* childDrawable = dynamic_cast<juce::Drawable*>(composite->getChildComponent(i)))
            {
                if (auto* childPath = dynamic_cast<juce::DrawablePath*>(childDrawable))
                {
                    path.addPath(childPath->getPath());
                }
            }
        }
    }

    // Transform: scale and center
    auto transform = juce::AffineTransform::scale(scale).translated(padding - (bounds.getX() * scale),
                                                                    padding - (bounds.getY() * scale));
    path.applyTransform(transform);

    return path;
}

/**
 * Get trash/delete icon as a path
 */
inline juce::Path createTrashIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::bin_svg, BinaryData::bin_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get gear/settings icon as a path
 */
inline juce::Path createGearIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::cog_svg, BinaryData::cog_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get eye/visibility (visible) icon as a path
 */
inline juce::Path createEyeOpenIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::eye_svg, BinaryData::eye_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get eye-blocked/hidden icon as a path
 */
inline juce::Path createEyeClosedIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::eyeblocked_svg, BinaryData::eyeblocked_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get close/cross icon as a path (for modal close buttons)
 */
inline juce::Path createCloseIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::cross_svg, BinaryData::cross_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get checkmark/confirm icon as a path (for save/confirm buttons)
 */
inline juce::Path createCheckmarkIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::checkmark_svg, BinaryData::checkmark_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get stats bars icon as a path (for statistics toggle)
 */
inline juce::Path createStatsIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::statsbars_svg, BinaryData::statsbars_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get pause icon as a path
 */
inline juce::Path createPauseIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::pause2_svg, BinaryData::pause2_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get play icon as a path
 */
inline juce::Path createPlayIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::play3_svg, BinaryData::play3_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

/**
 * Get redo/reset icon as a path
 */
inline juce::Path createRedoIcon(float size)
{
    auto drawable = loadSvgDrawable(BinaryData::redo_svg, BinaryData::redo_svgSize);
    return getPathFromDrawable(drawable.get(), size);
}

} // namespace oscil::ListItemIcons
