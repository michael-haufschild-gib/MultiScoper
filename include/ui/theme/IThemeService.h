/*
    Oscil - Theme Service Interface
    Abstract interface for theme management to enable dependency injection
*/

#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace oscil
{

// Forward declarations
struct ColorTheme;
class ThemeManagerListener;

/**
 * Abstract interface for theme management.
 * Enables dependency injection and testability by decoupling
 * UI components from the concrete ThemeManager implementation.
 */
class IThemeService
{
public:
    virtual ~IThemeService() = default;

    /**
     * Get the current active theme
     */
    virtual const ColorTheme& getCurrentTheme() const = 0;

    /**
     * Set the active theme by name
     */
    virtual bool setCurrentTheme(const juce::String& themeName) = 0;

    /**
     * Get list of available theme names
     */
    virtual std::vector<juce::String> getAvailableThemes() const = 0;

    /**
     * Get a theme by name (returns nullptr if not found)
     */
    virtual const ColorTheme* getTheme(const juce::String& themeName) const = 0;

    /**
     * Check if a theme is a system theme (immutable)
     */
    virtual bool isSystemTheme(const juce::String& name) const = 0;

    /**
     * Create a new custom theme
     */
    virtual bool createTheme(const juce::String& name, const juce::String& sourceTheme = "") = 0;

    /**
     * Update a custom theme
     */
    virtual bool updateTheme(const juce::String& name, const ColorTheme& theme) = 0;

    /**
     * Delete a custom theme
     */
    virtual bool deleteTheme(const juce::String& name) = 0;

    /**
     * Rename a custom theme. Fails if oldName is a system theme,
     * newName is invalid, or newName already exists.
     */
    virtual bool renameTheme(const juce::String& oldName, const juce::String& newName) = 0;

    /**
     * Clone a theme with a new name
     */
    virtual bool cloneTheme(const juce::String& sourceName, const juce::String& newName) = 0;

    /**
     * Import a theme from its XML string representation.
     */
    virtual bool importTheme(const juce::String& xmlString) = 0;

    /**
     * Export a theme to its XML string representation.
     */
    virtual juce::String exportTheme(const juce::String& name) const = 0;

    /**
     * Add a listener for theme changes
     */
    virtual void addListener(ThemeManagerListener* listener) = 0;

    /**
     * Remove a listener
     */
    virtual void removeListener(ThemeManagerListener* listener) = 0;
};

} // namespace oscil
