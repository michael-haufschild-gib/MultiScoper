/*
    Oscil - Themed Component Base Class Implementation
*/

#include "ui/components/ThemedComponent.h"

namespace oscil
{

ThemedComponent::ThemedComponent(IThemeService& themeService)
    : themeService_(themeService)
{
    // Initialize theme cache from service
    theme_ = themeService_.getCurrentTheme();

    // Register as listener for theme changes
    themeService_.addListener(this);
}

ThemedComponent::~ThemedComponent()
{
    // Unregister from theme service
    themeService_.removeListener(this);
}

void ThemedComponent::themeChanged(const ColorTheme& newTheme)
{
    // Update cached theme
    theme_ = newTheme;

    // Allow subclasses to customize behavior
    onThemeChanged(newTheme);

    // Trigger repaint with new theme
    repaint();
}

void ThemedComponent::onThemeChanged(const ColorTheme& /*newTheme*/)
{
    // Default implementation does nothing
    // Subclasses can override to add custom logic
}

} // namespace oscil
