/*
    Oscil - Theme Manager Listener Interface
*/

#pragma once

namespace oscil
{

struct ColorTheme;

/**
 * Listener interface for theme changes
 */
class ThemeManagerListener
{
public:
    virtual ~ThemeManagerListener() = default;
    virtual void themeChanged(const ColorTheme& newTheme) = 0;
};

} // namespace oscil
