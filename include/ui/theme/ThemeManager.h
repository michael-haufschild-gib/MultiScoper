/*
    Oscil - Theme Manager
    Theme system for customizable visual appearance
*/

#pragma once

#include "IThemeService.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <set>
#include <unordered_map>
#include <vector>

namespace oscil
{

// Forward declarations
class ThemeCoordinator;
class PluginFactory;

/**
 * Color theme definition
 */
struct ColorTheme
{
    juce::String name;
    bool isSystemTheme = false; // System themes cannot be edited/deleted

    // Background colors
    juce::Colour backgroundPrimary{0xFF1E1E1E};
    juce::Colour backgroundSecondary{0xFF2D2D2D};
    juce::Colour backgroundPane{0xFF252525};

    // Grid colors
    juce::Colour gridMajor{0xFF3A3A3A};
    juce::Colour gridMinor{0xFF2A2A2A};
    juce::Colour gridZeroLine{0xFF4A4A4A};

    // Crosshair color
    juce::Colour crosshairLine{0x80FFFFFF};

    // Text colors
    juce::Colour textPrimary{0xFFE0E0E0};
    juce::Colour textSecondary{0xFFA0A0A0};
    juce::Colour textHighlight{0xFFFFFFFF};

    // Control colors
    juce::Colour controlBackground{0xFF353535};
    juce::Colour controlBorder{0xFF454545};
    juce::Colour controlHighlight{0xFF505050};
    juce::Colour controlActive{0xFF007ACC};

    // Status colors (bright enough for WCAG large text AA 3:1 on dark backgrounds)
    juce::Colour statusActive{0xFF00DD00};
    juce::Colour statusWarning{0xFFDDBB00};
    juce::Colour statusError{0xFFEE4444};

    // Button Colors - Primary
    juce::Colour btnPrimaryBg{0xFF007ACC};
    juce::Colour btnPrimaryBgHover{0xFF008AD9};
    juce::Colour btnPrimaryBgActive{0xFF0062A3};
    juce::Colour btnPrimaryBgDisabled{0xFF353535}; // Often dimmed
    juce::Colour btnPrimaryText{0xFFFFFFFF};
    juce::Colour btnPrimaryTextHover{0xFFFFFFFF};
    juce::Colour btnPrimaryTextActive{0xFFFFFFFF};
    juce::Colour btnPrimaryTextDisabled{0xFFA0A0A0};

    // Button Colors - Secondary
    juce::Colour btnSecondaryBg{0xFF3A3A3A};
    juce::Colour btnSecondaryBgHover{0xFF454545};
    juce::Colour btnSecondaryBgActive{0xFF303030};
    juce::Colour btnSecondaryBgDisabled{0xFF252525};
    juce::Colour btnSecondaryText{0xFFE0E0E0};
    juce::Colour btnSecondaryTextHover{0xFFFFFFFF};
    juce::Colour btnSecondaryTextActive{0xFFFFFFFF};
    juce::Colour btnSecondaryTextDisabled{0xFF606060};

    // Button Colors - Tertiary
    juce::Colour btnTertiaryBg{0x00000000};      // Transparent
    juce::Colour btnTertiaryBgHover{0x1AFFFFFF}; // Slight overlay
    juce::Colour btnTertiaryBgActive{0x33FFFFFF};
    juce::Colour btnTertiaryBgDisabled{0x00000000};
    juce::Colour btnTertiaryText{0xFFE0E0E0};
    juce::Colour btnTertiaryTextHover{0xFFFFFFFF};
    juce::Colour btnTertiaryTextActive{0xFFFFFFFF};
    juce::Colour btnTertiaryTextDisabled{0xFF606060};

    // === Glass / Accent System ===

    // Accent color (HSV-based, since JUCE doesn't support OKLCH)
    float accentHue = 220.0f;      // 0-360 hue
    float accentSaturation = 0.7f; // 0-1
    float accentLightness = 0.6f;  // 0-1

    // Glass parameters
    float glassAlpha = 0.55f; // bg alpha for glass panels
    float panelAlpha = 0.82f; // bg alpha for panel containers
    float blurRadius = 20.0f; // conceptual blur (drives shadow spread)

    // Border alpha levels (white * alpha for dark themes)
    float borderSubtleAlpha = 0.08f;
    float borderDefaultAlpha = 0.12f;
    float borderStrongAlpha = 0.20f;

    // Shadow
    float shadowIntensity = 0.4f; // overall shadow darkness
    float shadowSpread = 12.0f;   // shadow blur radius px

    // Inset light edge
    float lightEdgeAlpha = 0.06f; // top highlight on glass panels

    // Accent glow
    float accentGlowRadius = 12.0f; // glow spread px
    float accentGlowAlpha = 0.3f;   // glow opacity

    // Default waveform colors (up to 64)
    std::vector<juce::Colour> waveformColors;

    ColorTheme() { initializeDefaultWaveformColors(); }

    /// Populate the waveformColors vector with the default HSL-distributed palette.
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
     * Export to XML string for theme import/export.
     */
    juce::String toXmlString() const;

    /**
     * Import from XML string (as produced by toXmlString).
     * @return true if parsing succeeded.
     */
    bool fromXmlString(const juce::String& xmlString);

    //==========================================================================
    // Accessibility - Contrast Validation
    //==========================================================================

    /**
     * Calculate the WCAG relative luminance of a color
     * Returns a value between 0 (black) and 1 (white)
     */
    static float calculateLuminance(juce::Colour colour)
    {
        auto linearize = [](float channel) {
            return channel <= 0.03928f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
        };

        float const r = linearize(colour.getFloatRed());
        float const g = linearize(colour.getFloatGreen());
        float const b = linearize(colour.getFloatBlue());

        return (0.2126f * r) + (0.7152f * g) + (0.0722f * b);
    }

    /**
     * Calculate WCAG contrast ratio between two colors
     * Returns a value between 1 (no contrast) and 21 (max contrast)
     */
    static float calculateContrastRatio(juce::Colour fg, juce::Colour bg)
    {
        float const fgLum = calculateLuminance(fg);
        float const bgLum = calculateLuminance(bg);

        // Ensure lighter is numerator
        float const l1 = std::max(fgLum, bgLum);
        float const l2 = std::min(fgLum, bgLum);

        return (l1 + 0.05f) / (l2 + 0.05f);
    }

    /**
     * Check if contrast meets WCAG AA standard for normal text (4.5:1)
     */
    static bool meetsContrastAA(juce::Colour fg, juce::Colour bg) { return calculateContrastRatio(fg, bg) >= 4.5f; }

    /**
     * Check if contrast meets WCAG AAA standard for normal text (7:1)
     */
    static bool meetsContrastAAA(juce::Colour fg, juce::Colour bg) { return calculateContrastRatio(fg, bg) >= 7.0f; }

    /**
     * Check if contrast meets WCAG AA standard for large text (3:1)
     */
    static bool meetsLargeTextContrastAA(juce::Colour fg, juce::Colour bg)
    {
        return calculateContrastRatio(fg, bg) >= 3.0f;
    }

    /**
     * Composite a semi-transparent foreground color onto an opaque background.
     * Returns the effective opaque color as it would appear when rendered.
     */
    static juce::Colour compositeOnBackground(juce::Colour fg, juce::Colour bg)
    {
        float const a = fg.getFloatAlpha();
        if (a >= 1.0f)
            return fg;
        return bg.interpolatedWith(fg.withAlpha(1.0f), a);
    }

    /**
     * Validate all critical text/background pairs in this theme
     * Returns a list of accessibility issues (empty if all pass)
     */
    std::vector<juce::String> validateAccessibility() const
    {
        std::vector<juce::String> issues;

        // Composite semi-transparent text colors for accurate measurement
        auto textPriOnPrimary = compositeOnBackground(textPrimary, backgroundPrimary);
        auto textSecOnPrimary = compositeOnBackground(textSecondary, backgroundPrimary);
        auto textPriOnSecondary = compositeOnBackground(textPrimary, backgroundSecondary);
        auto textSecOnSecondary = compositeOnBackground(textSecondary, backgroundSecondary);

        // Check primary text on backgrounds
        if (!meetsContrastAA(textPriOnPrimary, backgroundPrimary))
            issues.emplace_back("textPrimary on backgroundPrimary fails AA contrast");
        if (!meetsContrastAA(textPriOnSecondary, backgroundSecondary))
            issues.emplace_back("textPrimary on backgroundSecondary fails AA contrast");
        if (!meetsContrastAA(textSecOnPrimary, backgroundPrimary))
            issues.emplace_back("textSecondary on backgroundPrimary fails AA contrast");
        if (!meetsContrastAA(textSecOnSecondary, backgroundSecondary))
            issues.emplace_back("textSecondary on backgroundSecondary fails AA contrast");

        // Check button text contrast
        if (!meetsContrastAA(btnPrimaryText, btnPrimaryBg))
            issues.emplace_back("Primary button text fails AA contrast");
        if (!meetsContrastAA(btnSecondaryText, btnSecondaryBg))
            issues.emplace_back("Secondary button text fails AA contrast");

        // Tertiary buttons render on the page background
        auto tertiaryTextComposited = compositeOnBackground(btnTertiaryText, backgroundPrimary);
        if (!meetsContrastAA(tertiaryTextComposited, backgroundPrimary))
            issues.emplace_back("Tertiary button text on background fails AA contrast");

        // Check status colors on background
        if (!meetsLargeTextContrastAA(statusActive, backgroundPrimary))
            issues.emplace_back("statusActive on background fails large text AA contrast");
        if (!meetsLargeTextContrastAA(statusWarning, backgroundPrimary))
            issues.emplace_back("statusWarning on background fails large text AA contrast");
        if (!meetsLargeTextContrastAA(statusError, backgroundPrimary))
            issues.emplace_back("statusError on background fails large text AA contrast");

        // Glass background checks: effective bg behind glass panels
        auto effectiveGlassBg = backgroundPrimary.interpolatedWith(backgroundPane, glassAlpha);
        auto textPriOnGlass = compositeOnBackground(textPrimary, effectiveGlassBg);
        if (!meetsContrastAA(textPriOnGlass, effectiveGlassBg))
            issues.emplace_back("textPrimary on glass background fails AA contrast");

        return issues;
    }
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
     * Import theme from JSON
     */
    bool importTheme(const juce::String& xmlString) override;

    /**
     * Export theme to JSON
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
ColorTheme createDarkProfessional();
ColorTheme createClassicGreen();
ColorTheme createClassicAmber();
ColorTheme createHighContrast();
ColorTheme createLightMode();
ColorTheme createGlassDarkBlue();
ColorTheme createGlassDarkPurple();
ColorTheme createGlassDarkBrown();
ColorTheme createGlassDarkBlack();
} // namespace SystemThemes

} // namespace oscil