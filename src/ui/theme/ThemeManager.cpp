/*
    Oscil - Theme Manager Implementation
    (ColorTheme serialization and file I/O are in ThemeManagerSerialization.cpp)
*/

#include "ui/theme/ThemeManager.h"

#include <algorithm>

namespace oscil
{

void ColorTheme::initializeDefaultWaveformColors()
{
    static const std::vector<juce::Colour> defaultColors = {
        juce::Colour(0xFF00FF00), // Green
        juce::Colour(0xFF00FFFF), // Cyan
        juce::Colour(0xFFFF00FF), // Magenta
        juce::Colour(0xFFFFFF00), // Yellow
        juce::Colour(0xFFFF8000), // Orange
        juce::Colour(0xFF8080FF), // Light blue
        juce::Colour(0xFFFF8080), // Light red
        juce::Colour(0xFF80FF80), // Light green
        juce::Colour(0xFFFFFFFF), // White
        juce::Colour(0xFF00FF80), // Spring green
        juce::Colour(0xFF8000FF), // Purple
        juce::Colour(0xFFFF0080), // Pink
        juce::Colour(0xFF80FFFF), // Light cyan
        juce::Colour(0xFFFFFF80), // Light yellow
        juce::Colour(0xFFFF80FF), // Light magenta
        juce::Colour(0xFF0080FF), // Sky blue
    };

    waveformColors = defaultColors;

    while (waveformColors.size() < 64)
    {
        float hue = static_cast<float>(waveformColors.size()) / 64.0f;
        waveformColors.push_back(juce::Colour::fromHSV(hue, 0.8f, 1.0f, 1.0f));
    }
}

// === ThemeManager Implementation ===

ThemeManager::ThemeManager()
{
    initializeSystemThemes();
    loadThemes();

    if (themes_.find("Dark Professional") != themes_.end())
    {
        currentTheme_ = themes_["Dark Professional"];
    }
}

ThemeManager::~ThemeManager()
{
    stopTimer();

    if (!pendingSaves_.empty())
    {
        auto themesDir = getThemesDirectory();
        themesDir.createDirectory();

        for (const auto& name : pendingSaves_)
        {
            auto it = themes_.find(name);
            if (it != themes_.end() && !it->second.isSystemTheme)
            {
                auto file = themesDir.getChildFile(name + ".xml");
                if (auto xml = it->second.toValueTree().createXml())
                {
                    xml->writeTo(file);
                }
            }
        }
    }
}

void ThemeManager::timerCallback()
{
    stopTimer();
    flushPendingSaves();
}

void ThemeManager::flushPendingSaves()
{
    if (pendingSaves_.empty())
        return;

    std::vector<std::pair<juce::String, juce::String>> filesToWrite;
    auto themesDir = getThemesDirectory();

    for (const auto& name : pendingSaves_)
    {
        auto it = themes_.find(name);
        if (it != themes_.end() && !it->second.isSystemTheme)
        {
            if (auto xml = it->second.toValueTree().createXml())
            {
                filesToWrite.emplace_back(themesDir.getChildFile(name + ".xml").getFullPathName(), xml->toString());
            }
        }
    }

    pendingSaves_.clear();

    if (filesToWrite.empty())
        return;

    juce::Thread::launch([filesToWrite]() {
        for (const auto& [path, content] : filesToWrite)
        {
            juce::File file(path);
            file.getParentDirectory().createDirectory();
            file.replaceWithText(content);
        }
    });
}

void ThemeManager::initializeSystemThemes()
{
    themes_["Dark Professional"] = SystemThemes::createDarkProfessional();
    themes_["Classic Green"] = SystemThemes::createClassicGreen();
    themes_["Classic Amber"] = SystemThemes::createClassicAmber();
    themes_["High Contrast"] = SystemThemes::createHighContrast();
    themes_["Light Mode"] = SystemThemes::createLightMode();
}

bool ThemeManager::setCurrentTheme(const juce::String& themeName)
{
    auto it = themes_.find(themeName);
    if (it == themes_.end())
        return false;

    currentTheme_ = it->second;
    notifyListeners();
    return true;
}

std::vector<juce::String> ThemeManager::getAvailableThemes() const
{
    std::vector<juce::String> result;
    result.reserve(themes_.size());
    for (const auto& [name, theme] : themes_)
    {
        result.push_back(name);
    }
    std::ranges::sort(result);
    return result;
}

const ColorTheme* ThemeManager::getTheme(const juce::String& themeName) const
{
    auto it = themes_.find(themeName);
    return it != themes_.end() ? &it->second : nullptr;
}

bool ThemeManager::createTheme(const juce::String& name, const juce::String& sourceTheme)
{
    if (themes_.find(name) != themes_.end())
        return false;

    ColorTheme newTheme;
    if (sourceTheme.isNotEmpty())
    {
        auto it = themes_.find(sourceTheme);
        if (it != themes_.end())
            newTheme = it->second;
    }

    newTheme.name = name;
    newTheme.isSystemTheme = false;
    themes_[name] = newTheme;

    saveTheme(name);
    return true;
}

bool ThemeManager::updateTheme(const juce::String& name, const ColorTheme& theme)
{
    auto it = themes_.find(name);
    if (it == themes_.end() || it->second.isSystemTheme)
        return false;

    it->second = theme;
    it->second.name = name;
    it->second.isSystemTheme = false;

    saveTheme(name);

    if (currentTheme_.name == name)
    {
        currentTheme_ = it->second;
        notifyListeners();
    }

    return true;
}

bool ThemeManager::deleteTheme(const juce::String& name)
{
    auto it = themes_.find(name);
    if (it == themes_.end() || it->second.isSystemTheme)
        return false;

    if (currentTheme_.name == name)
    {
        for (const auto& [themeName, theme] : themes_)
        {
            if (theme.isSystemTheme)
            {
                currentTheme_ = theme;
                notifyListeners();
                break;
            }
        }
    }

    themes_.erase(it);

    pendingSaves_.erase(name);

    deleteThemeFile(name);
    return true;
}

bool ThemeManager::cloneTheme(const juce::String& sourceName, const juce::String& newName)
{
    auto it = themes_.find(sourceName);
    if (it == themes_.end())
        return false;

    if (themes_.find(newName) != themes_.end())
        return false;

    ColorTheme clone = it->second;
    clone.name = newName;
    clone.isSystemTheme = false;
    themes_[newName] = clone;

    saveTheme(newName);
    return true;
}

bool ThemeManager::isSystemTheme(const juce::String& name) const
{
    auto it = themes_.find(name);
    return it != themes_.end() && it->second.isSystemTheme;
}

bool ThemeManager::importTheme(const juce::String& json)
{
    ColorTheme theme;
    if (!theme.fromXmlString(json))
        return false;

    if (theme.name.isEmpty())
        return false;

    theme.isSystemTheme = false;
    themes_[theme.name] = theme;

    saveTheme(theme.name);
    return true;
}

juce::String ThemeManager::exportTheme(const juce::String& name) const
{
    auto it = themes_.find(name);
    if (it == themes_.end())
        return {};

    return it->second.toXmlString();
}

void ThemeManager::addListener(ThemeManagerListener* listener) { listeners_.add(listener); }

void ThemeManager::removeListener(ThemeManagerListener* listener) { listeners_.remove(listener); }

void ThemeManager::notifyListeners()
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        listeners_.call([this](ThemeManagerListener& listener) { listener.themeChanged(currentTheme_); });
    }
    else
    {
        auto themeCopy = currentTheme_;
        juce::MessageManager::callAsync([this, themeCopy]() {
            listeners_.call([&themeCopy](ThemeManagerListener& listener) { listener.themeChanged(themeCopy); });
        });
    }
}

// === System Theme Definitions ===

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
    for (int i = 0; i < 64; ++i)
    {
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
    for (int i = 0; i < 64; ++i)
    {
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
    while (theme.waveformColors.size() < 64)
    {
        float hue = static_cast<float>(theme.waveformColors.size()) / 64.0f;
        theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 0.9f, 0.6f, 1.0f));
    }
    return theme;
}
} // namespace SystemThemes

} // namespace oscil
