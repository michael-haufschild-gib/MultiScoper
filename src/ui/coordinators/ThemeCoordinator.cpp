/*
    Oscil - Theme Coordinator Implementation
*/

#include "ui/coordinators/ThemeCoordinator.h"

namespace oscil
{

ThemeCoordinator::ThemeCoordinator(IThemeService& themeService, ThemeChangedCallback onThemeChanged)
    : themeService_(themeService)
    , onThemeChanged_(std::move(onThemeChanged))
{
    // Register as listener
    themeService_.addListener(this);
}

ThemeCoordinator::~ThemeCoordinator()
{
    // Unregister from theme service
    themeService_.removeListener(this);
}

void ThemeCoordinator::themeChanged(const ColorTheme& newTheme)
{
    if (onThemeChanged_)
    {
        onThemeChanged_(newTheme);
    }
}

const ColorTheme& ThemeCoordinator::getCurrentTheme() const
{
    return themeService_.getCurrentTheme();
}

} // namespace oscil
