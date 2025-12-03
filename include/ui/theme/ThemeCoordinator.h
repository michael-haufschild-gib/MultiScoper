/*
    Oscil - Theme Coordinator
    Handles ThemeManagerListener callbacks for theme changes
*/

#pragma once

#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include <functional>

namespace oscil
{

/**
 * Coordinates theme change events for the plugin editor.
 * Encapsulates ThemeManagerListener implementation to reduce
 * editor responsibilities.
 */
class ThemeCoordinator : public ThemeManagerListener
{
public:
    using ThemeChangedCallback = std::function<void(const ColorTheme&)>;

    /**
     * Create coordinator with theme service reference and change callback.
     * Automatically registers as listener on construction.
     */
    ThemeCoordinator(IThemeService& themeService, ThemeChangedCallback onThemeChanged);

    /**
     * Destructor automatically unregisters from theme service.
     */
    ~ThemeCoordinator() override;

    // ThemeManagerListener interface
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Get the current theme.
     */
    const ColorTheme& getCurrentTheme() const;

    // Prevent copying
    ThemeCoordinator(const ThemeCoordinator&) = delete;
    ThemeCoordinator& operator=(const ThemeCoordinator&) = delete;

private:
    IThemeService& themeService_;
    ThemeChangedCallback onThemeChanged_;
};

} // namespace oscil
