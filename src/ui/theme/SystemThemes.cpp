/*
    Oscil - System Themes Implementation
*/

#include "ui/theme/ThemeManager.h"
#include <cmath>

namespace oscil
{

namespace SystemThemes
{
    ColorTheme createDarkProfessional()
    {
        ColorTheme theme;
        theme.name = "Dark Professional";
        theme.isSystemTheme = true;
        
        theme.backgroundPrimary = juce::Colour(0xFF1E1E1E);
        theme.backgroundSecondary = juce::Colour(0xFF2D2D2D);
        theme.backgroundPane = juce::Colour(0xFF252525);
        theme.gridMajor = juce::Colour(0xFF3A3A3A);
        theme.gridMinor = juce::Colour(0xFF2A2A2A);
        theme.gridZeroLine = juce::Colour(0xFF4A4A4A);
        theme.textPrimary = juce::Colour(0xFFE0E0E0);
        theme.textSecondary = juce::Colour(0xFFA0A0A0);
        theme.textHighlight = juce::Colour(0xFFFFFFFF);
        theme.controlBackground = juce::Colour(0xFF353535);
        theme.controlBorder = juce::Colour(0xFF454545);
        theme.controlHighlight = juce::Colour(0xFF505050);
        theme.controlActive = juce::Colour(0xFF007ACC);
        theme.statusActive = juce::Colour(0xFF00CC00);
        theme.statusWarning = juce::Colour(0xFFCCAA00);
        theme.statusError = juce::Colour(0xFFCC0000);
        
        theme.btnPrimaryBg = juce::Colour(0xFF007ACC);
        theme.btnPrimaryBgHover = juce::Colour(0xFF008AD9);
        theme.btnPrimaryBgActive = juce::Colour(0xFF0062A3);
        theme.btnPrimaryBgDisabled = juce::Colour(0xFF353535);
        theme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
        theme.btnPrimaryTextDisabled = juce::Colour(0xFFA0A0A0);

        theme.btnSecondaryBg = juce::Colour(0xFF3A3A3A);
        theme.btnSecondaryBgHover = juce::Colour(0xFF454545);
        theme.btnSecondaryBgActive = juce::Colour(0xFF303030);
        theme.btnSecondaryBgDisabled = juce::Colour(0xFF252525);
        theme.btnSecondaryText = juce::Colour(0xFFE0E0E0);
        theme.btnSecondaryTextDisabled = juce::Colour(0xFF606060);

        theme.btnTertiaryBg = juce::Colours::transparentBlack;
        theme.btnTertiaryBgHover = juce::Colour(0x1AFFFFFF);
        theme.btnTertiaryBgActive = juce::Colour(0x33FFFFFF);
        theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
        theme.btnTertiaryText = juce::Colour(0xFFE0E0E0);
        theme.btnTertiaryTextDisabled = juce::Colour(0xFF606060);
        return theme;
    }

    ColorTheme createClassicGreen()
    {
        ColorTheme theme;
        theme.name = "Classic Green";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF0A0A0A);
        theme.backgroundSecondary = juce::Colour(0xFF151515);
        theme.backgroundPane = juce::Colour(0xFF0D0D0D);
        theme.gridMajor = juce::Colour(0xFF1A3A1A);
        theme.gridMinor = juce::Colour(0xFF0D1F0D);
        theme.gridZeroLine = juce::Colour(0xFF2A4A2A);
        theme.textPrimary = juce::Colour(0xFF00FF00);
        theme.textSecondary = juce::Colour(0xFF00AA00);
        theme.textHighlight = juce::Colour(0xFF00FF00);
        theme.controlBackground = juce::Colour(0xFF1A1A1A);
        theme.controlBorder = juce::Colour(0xFF00AA00);
        theme.controlHighlight = juce::Colour(0xFF003300);
        theme.controlActive = juce::Colour(0xFF00FF00);
        
        theme.waveformColors.clear();
        for (int i = 0; i < 64; ++i) {
            float brightness = 0.5f + 0.5f * (static_cast<float>(i) / 64.0f);
            theme.waveformColors.push_back(juce::Colour::fromHSV(0.33f, 1.0f, brightness, 1.0f));
        }
        return theme;
    }

    ColorTheme createClassicAmber()
    {
        ColorTheme theme;
        theme.name = "Classic Amber";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF0A0A00);
        theme.backgroundSecondary = juce::Colour(0xFF151508);
        theme.backgroundPane = juce::Colour(0xFF0D0D05);
        theme.gridMajor = juce::Colour(0xFF3A3A1A);
        theme.gridMinor = juce::Colour(0xFF1F1F0D);
        theme.gridZeroLine = juce::Colour(0xFF4A4A2A);
        theme.textPrimary = juce::Colour(0xFFFFAA00);
        theme.textSecondary = juce::Colour(0xFFAA7700);
        theme.textHighlight = juce::Colour(0xFFFFCC00);
        theme.controlBackground = juce::Colour(0xFF1A1A0A);
        theme.controlBorder = juce::Colour(0xFFAA7700);
        theme.controlHighlight = juce::Colour(0xFF332200);
        theme.controlActive = juce::Colour(0xFFFFAA00);
        
        theme.waveformColors.clear();
        for (int i = 0; i < 64; ++i) {
            float hue = 0.08f + 0.04f * std::sin(static_cast<float>(i) * 0.2f);
            theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
        }
        return theme;
    }

    ColorTheme createHighContrast()
    {
        ColorTheme theme;
        theme.name = "High Contrast";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF000000);
        theme.backgroundSecondary = juce::Colour(0xFF000000);
        theme.backgroundPane = juce::Colour(0xFF000000);
        theme.gridMajor = juce::Colour(0xFF404040);
        theme.gridMinor = juce::Colour(0xFF202020);
        theme.gridZeroLine = juce::Colour(0xFF606060);
        theme.textPrimary = juce::Colour(0xFFFFFFFF);
        theme.textSecondary = juce::Colour(0xFFCCCCCC);
        theme.textHighlight = juce::Colour(0xFFFFFF00);
        theme.controlBackground = juce::Colour(0xFF000000);
        theme.controlBorder = juce::Colour(0xFFFFFFFF);
        theme.controlHighlight = juce::Colour(0xFF404040);
        theme.controlActive = juce::Colour(0xFFFFFF00);
        return theme;
    }

    ColorTheme createLightMode()
    {
        ColorTheme theme;
        theme.name = "Light Mode";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFFF5F5F5);
        theme.backgroundSecondary = juce::Colour(0xFFE8E8E8);
        theme.backgroundPane = juce::Colour(0xFFFFFFFF);
        theme.gridMajor = juce::Colour(0xFFCCCCCC);
        theme.gridMinor = juce::Colour(0xFFE0E0E0);
        theme.gridZeroLine = juce::Colour(0xFFAAAAAA);
        theme.textPrimary = juce::Colour(0xFF202020);
        theme.textSecondary = juce::Colour(0xFF606060);
        theme.textHighlight = juce::Colour(0xFF000000);
        theme.controlBackground = juce::Colour(0xFFFFFFFF);
        theme.controlBorder = juce::Colour(0xFFCCCCCC);
        theme.controlHighlight = juce::Colour(0xFFE0E0E0);
        theme.controlActive = juce::Colour(0xFF0066CC);
        
        theme.waveformColors.clear();
        theme.waveformColors = {
            juce::Colour(0xFF007700), juce::Colour(0xFF007777), juce::Colour(0xFF770077), juce::Colour(0xFF777700),
            juce::Colour(0xFFCC5500), juce::Colour(0xFF0000CC), juce::Colour(0xFFCC0000), juce::Colour(0xFF00CC00),
        };
        while (theme.waveformColors.size() < 64) {
            float hue = static_cast<float>(theme.waveformColors.size()) / 64.0f;
            theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 0.9f, 0.6f, 1.0f));
        }
        return theme;
    }
}

} // namespace oscil
