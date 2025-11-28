/*
    Oscil - Theme Manager
    Theme system for customizable visual appearance
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <unordered_map>
#include "IThemeService.h"

namespace oscil
{

/**
 * Color theme definition
 */
struct ColorTheme
{
    juce::String name;
    bool isSystemTheme = false;     // System themes cannot be edited/deleted

    // Background colors
    juce::Colour backgroundPrimary{ 0xFF1E1E1E };
    juce::Colour backgroundSecondary{ 0xFF2D2D2D };
    juce::Colour backgroundPane{ 0xFF252525 };

    // Grid colors
    juce::Colour gridMajor{ 0xFF3A3A3A };
    juce::Colour gridMinor{ 0xFF2A2A2A };
    juce::Colour gridZeroLine{ 0xFF4A4A4A };

    // Text colors
    juce::Colour textPrimary{ 0xFFE0E0E0 };
    juce::Colour textSecondary{ 0xFFA0A0A0 };
    juce::Colour textHighlight{ 0xFFFFFFFF };

    // Control colors
    juce::Colour controlBackground{ 0xFF353535 };
    juce::Colour controlBorder{ 0xFF454545 };
    juce::Colour controlHighlight{ 0xFF505050 };
    juce::Colour controlActive{ 0xFF007ACC };

    // Status colors
    juce::Colour statusActive{ 0xFF00CC00 };
    juce::Colour statusWarning{ 0xFFCCAA00 };
    juce::Colour statusError{ 0xFFCC0000 };

    // Default waveform colors (up to 64)
    std::vector<juce::Colour> waveformColors;

    ColorTheme()
    {
        initializeDefaultWaveformColors();
    }

    void initializeDefaultWaveformColors();

    /**
     * Get waveform color by index (cycles if index > count)
     */
    juce::Colour getWaveformColor(int index) const
    {
        if (waveformColors.empty())
            return juce::Colours::green;
        return waveformColors[static_cast<size_t>(index) % waveformColors.size()];
    }

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    /**
     * Export to JSON string
     */
    juce::String toJson() const;

    /**
     * Import from JSON string
     */
    bool fromJson(const juce::String& json);
};

/**
 * Listener interface for theme changes
 */
class ThemeManagerListener
{
public:
    virtual ~ThemeManagerListener() = default;
    virtual void themeChanged(const ColorTheme& newTheme) = 0;
};

/**
 * Theme manager handles loading, saving, and applying themes.
 *
 * Implements IThemeService interface for dependency injection support.
 */
class ThemeManager : public IThemeService
{
public:
    static ThemeManager& getInstance();

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
     * Check if a theme is a system theme (immutable)
     */
    bool isSystemTheme(const juce::String& name) const override;

    /**
     * Import theme from JSON
     */
    bool importTheme(const juce::String& json);

    /**
     * Export theme to JSON
     */
    juce::String exportTheme(const juce::String& name) const;

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

    // Prevent copying
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

private:
    ThemeManager();
    ~ThemeManager() override;

    void initializeSystemThemes();
    void notifyListeners();

    /**
     * Save a single theme to disk immediately
     * Called by modifying methods to ensure themes are persisted on change
     */
    void saveTheme(const juce::String& themeName);

    /**
     * Delete a theme file from disk
     */
    void deleteThemeFile(const juce::String& themeName);

    ColorTheme currentTheme_;
    std::unordered_map<juce::String, ColorTheme> themes_;
    juce::ListenerList<ThemeManagerListener> listeners_;
};

// Pre-defined system themes
namespace SystemThemes
{
    ColorTheme createDarkProfessional();
    ColorTheme createClassicGreen();
    ColorTheme createClassicAmber();
    ColorTheme createHighContrast();
    ColorTheme createLightMode();
}

} // namespace oscil
