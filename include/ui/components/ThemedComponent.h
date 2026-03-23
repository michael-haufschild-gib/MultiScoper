/*
    Oscil - Themed Component Base Class
    Eliminates duplicate theme change handling across UI components
*/

#pragma once

#include "ui/theme/IThemeService.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

/**
 * Base class for components that need theme awareness
 *
 * Automatically handles:
 * - Theme service listener registration/unregistration
 * - Theme cache updates
 * - Repaint triggering on theme changes
 *
 * Usage:
 *   class MyComponent : public ThemedComponent
 *   {
 *   public:
 *       MyComponent(IThemeService& themeService)
 *           : ThemedComponent(themeService)
 *       {
 *           // Component-specific initialization
 *       }
 *
 *   protected:
 *       void onThemeChanged(const ColorTheme& newTheme) override
 *       {
 *           // Optional: Custom theme change logic
 *           // Base class already updated theme_ and will call repaint()
 *       }
 *
 *       void paint(juce::Graphics& g) override
 *       {
 *           // Use getTheme() to access current theme
 *           g.fillAll(getTheme().backgroundPrimary);
 *       }
 *   };
 */
class ThemedComponent
    : public juce::Component
    , public ThemeManagerListener
{
public:
    /**
     * Construct a themed component
     * @param themeService The theme service to listen to
     */
    explicit ThemedComponent(IThemeService& themeService);

    /**
     * Destructor - automatically unregisters from theme service
     */
    ~ThemedComponent() override;

    /**
     * ThemeManagerListener implementation (final - do not override)
     * Automatically updates theme cache and calls onThemeChanged()
     */
    void themeChanged(const ColorTheme& newTheme) final;

protected:
    /**
     * Override this method to customize theme change behavior
     * Default implementation does nothing (base class handles repaint)
     *
     * @param newTheme The new theme that was just applied
     */
    virtual void onThemeChanged(const ColorTheme& newTheme);

    /**
     * Get the current theme
     * @return Reference to the cached theme
     */
    const ColorTheme& getTheme() const { return theme_; }

    /**
     * Get the theme service
     * @return Reference to the theme service
     */
    IThemeService& getThemeService() { return themeService_; }

private:
    ColorTheme theme_;
    IThemeService& themeService_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemedComponent)
};

} // namespace oscil
