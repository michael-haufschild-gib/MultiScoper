/*
    MultiScoper - Theme Manager
    Theme system for customizable visual appearance
*/

#pragma once

#include "ColorTheme.h"
#include "IThemeService.h"
#include "ThemeManagerListener.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace multiscoper
{

// Forward declarations
class ThemeCoordinator;
class PluginFactory;

/**
 * Theme manager handles loading, saving, and applying themes.
 *
 * Implements IThemeService interface for dependency injection.
 * Owned by PluginFactory - do not create directly except in tests.
 */
class ThemeManager
    : public IThemeService
    , private juce::Timer
{
public:
    ThemeManager();
    ~ThemeManager() override;

    /**
     * Get the current active theme
     */
    const ColorTheme& getCurrentTheme() const override { return currentTheme_; }

    /**
     * Set the active theme by name
     */
    bool setCurrentTheme(const juce::String& themeName) override;

    /**
     * Get list of available theme names
     */
    std::vector<juce::String> getAvailableThemes() const override;

    /**
     * Get a theme by name
     */
    const ColorTheme* getTheme(const juce::String& themeName) const override;

    /**
     * Create a new custom theme (copies from source if provided)
     */
    bool createTheme(const juce::String& name, const juce::String& sourceTheme = "") override;

    /**
     * Update a custom theme
     */
    bool updateTheme(const juce::String& name, const ColorTheme& theme) override;

    /**
     * Delete a custom theme (system themes cannot be deleted)
     */
    bool deleteTheme(const juce::String& name) override;

    /**
     * Clone a theme with a new name
     */
    bool cloneTheme(const juce::String& sourceName, const juce::String& newName) override;

    /**
     * Rename a custom theme
     */
    bool renameTheme(const juce::String& oldName, const juce::String& newName) override;

    /**
     * Check if a theme is a system theme (immutable)
     */
    bool isSystemTheme(const juce::String& name) const override;

    /**
     * Import theme from XML string (JUCE ValueTree format).
     */
    bool importTheme(const juce::String& xmlString) override;

    /**
     * Export theme to XML string (JUCE ValueTree format).
     */
    juce::String exportTheme(const juce::String& name) const override;

    /**
     * Get the themes directory
     */
    juce::File getThemesDirectory() const;

    /**
     * Load all themes from disk
     */
    void loadThemes();

    /**
     * Save custom themes to disk
     */
    void saveThemes();

    /**
     * Add/remove listeners
     */
    void addListener(ThemeManagerListener* listener) override;
    void removeListener(ThemeManagerListener* listener) override;

    /// Write any pending theme changes to disk immediately (call on shutdown).
    void flushPendingSaves();

    // Prevent copying
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    /**
     * Validate a theme name for filesystem safety.
     * Rejects empty, path-traversal, and OS-reserved-character names.
     */
    static bool isValidThemeName(const juce::String& name);

private:
    void timerCallback() override;
    void initializeSystemThemes();
    void notifyListeners();

    /**
     * Gather all pending theme writes as (filePath, xmlContent) pairs.
     * Clears pendingSaves_ after gathering. Used by both destructor (sync)
     * and flushPendingSaves (async).
     */
    std::vector<std::pair<juce::String, juce::String>> gatherPendingWrites();

    /**
     * Save a single theme to disk immediately
     * Called by modifying methods to ensure themes are persisted on change
     */
    void saveTheme(const juce::String& themeName);

    /**
     * Delete a theme file from disk
     */
    void deleteThemeFile(const juce::String& themeName) const;

    ColorTheme currentTheme_;
    std::unordered_map<juce::String, ColorTheme> themes_;
    std::set<juce::String> pendingSaves_;
    juce::ListenerList<ThemeManagerListener> listeners_;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ThemeManager)
};

// Pre-defined system themes
namespace SystemThemes
{
/// Factory for the default dark professional theme.
ColorTheme createDarkProfessional();
/// Factory for the classic green oscilloscope theme.
ColorTheme createClassicGreen();
/// Factory for the classic amber oscilloscope theme.
ColorTheme createClassicAmber();
/// Factory for the high-contrast accessibility theme.
ColorTheme createHighContrast();
/// Factory for the light-mode theme.
ColorTheme createLightMode();
/// Factory for the glassmorphism dark-blue theme variant.
ColorTheme createGlassDarkBlue();
/// Factory for the glassmorphism dark-purple theme variant.
ColorTheme createGlassDarkPurple();
/// Factory for the glassmorphism dark-brown theme variant.
ColorTheme createGlassDarkBrown();
/// Factory for the glassmorphism dark-black theme variant.
ColorTheme createGlassDarkBlack();
} // namespace SystemThemes

} // namespace multiscoper
