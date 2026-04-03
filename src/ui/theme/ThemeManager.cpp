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
        float const hue = static_cast<float>(waveformColors.size()) / 64.0f;
        waveformColors.push_back(juce::Colour::fromHSV(hue, 0.8f, 1.0f, 1.0f));
    }
}

// === ThemeManager Implementation ===

ThemeManager::ThemeManager()
{
    initializeSystemThemes();
    loadThemes();

    if (themes_.contains("Dark Professional"))
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
            juce::File const file(path);
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

bool ThemeManager::isValidThemeName(const juce::String& name)
{
    if (name.isEmpty())
        return false;
    if (name.length() > 255)
        return false;
    if (name.contains("..") || name.containsAnyOf("/\\:*?\"<>|"))
        return false;
    return true;
}

bool ThemeManager::createTheme(const juce::String& name, const juce::String& sourceTheme)
{
    if (!isValidThemeName(name) || themes_.contains(name))
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
        // Fall back to Dark Professional, or any system theme if that's missing.
        auto fallback = themes_.find("Dark Professional");
        if (fallback == themes_.end() || fallback->first == name)
        {
            for (auto& [themeName, theme] : themes_)
            {
                if (theme.isSystemTheme && themeName != name)
                {
                    fallback = themes_.find(themeName);
                    break;
                }
            }
        }

        if (fallback != themes_.end())
        {
            currentTheme_ = fallback->second;
            notifyListeners();
        }
    }

    themes_.erase(it);

    pendingSaves_.erase(name);

    deleteThemeFile(name);
    return true;
}

bool ThemeManager::cloneTheme(const juce::String& sourceName, const juce::String& newName)
{
    if (!isValidThemeName(newName))
        return false;

    auto it = themes_.find(sourceName);
    if (it == themes_.end())
        return false;

    if (themes_.contains(newName))
        return false;

    ColorTheme clone = it->second;
    clone.name = newName;
    clone.isSystemTheme = false;
    themes_[newName] = clone;

    saveTheme(newName);
    return true;
}

bool ThemeManager::renameTheme(const juce::String& oldName, const juce::String& newName)
{
    if (oldName == newName)
        return true;

    if (!isValidThemeName(newName))
        return false;

    auto it = themes_.find(oldName);
    if (it == themes_.end() || it->second.isSystemTheme)
        return false;

    if (themes_.contains(newName))
        return false;

    auto theme = std::move(it->second);
    theme.name = newName;
    themes_.erase(it);
    themes_[newName] = std::move(theme);

    if (currentTheme_.name == oldName)
    {
        currentTheme_.name = newName;
        notifyListeners();
    }

    pendingSaves_.erase(oldName);
    deleteThemeFile(oldName);
    saveTheme(newName);

    return true;
}

bool ThemeManager::isSystemTheme(const juce::String& name) const
{
    auto it = themes_.find(name);
    return it != themes_.end() && it->second.isSystemTheme;
}

bool ThemeManager::importTheme(const juce::String& xmlString)
{
    ColorTheme theme;
    if (!theme.fromXmlString(xmlString))
        return false;

    auto importedName = theme.name.trim();
    if (!isValidThemeName(importedName))
        return false;

    // Prevent imported themes from overwriting protected system themes
    if (isSystemTheme(importedName))
        return false;

    theme.name = importedName;
    theme.isSystemTheme = false;
    themes_[theme.name] = theme;

    saveTheme(theme.name);

    // Refresh active theme if the import overwrites the currently selected theme
    if (currentTheme_.name == theme.name)
    {
        currentTheme_ = theme;
        notifyListeners();
    }

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
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<ThemeManager>(this), themeCopy]() {
            if (auto* self = weakThis.get())
                self->listeners_.call(
                    [&themeCopy](ThemeManagerListener& listener) { listener.themeChanged(themeCopy); });
        });
    }
}

} // namespace oscil
