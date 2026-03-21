/*
    Oscil - Global Preferences Implementation
    User-wide preferences stored separately from project state
*/

#include "core/OscilState.h"

namespace oscil
{

GlobalPreferences::GlobalPreferences()
{
    preferences_ = juce::ValueTree("GlobalPreferences");
    load();
}

GlobalPreferences::~GlobalPreferences()
{
}

juce::File GlobalPreferences::getPreferencesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appDataDir.getChildFile("Oscil").getChildFile("preferences.xml");
}

void GlobalPreferences::load()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto file = getPreferencesFile();
    if (file.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(file))
        {
            preferences_ = juce::ValueTree::fromXml(*xml);
        }
    }
}

void GlobalPreferences::save()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto file = getPreferencesFile();

    auto parentDir = file.getParentDirectory();
    if (!parentDir.exists())
    {
        auto result = parentDir.createDirectory();
        if (result.failed())
        {
            DBG("GlobalPreferences::save() - Failed to create directory: " << result.getErrorMessage());
            return;
        }
    }

    if (auto xml = preferences_.createXml())
    {
        if (!xml->writeTo(file))
        {
            DBG("GlobalPreferences::save() - Failed to write preferences to: " << file.getFullPathName());
        }
    }
    else
    {
        DBG("GlobalPreferences::save() - Failed to create XML from preferences");
    }
}

juce::String GlobalPreferences::getDefaultTheme() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultTheme", "Dark Professional");
}

void GlobalPreferences::setDefaultTheme(const juce::String& themeName)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultTheme", themeName, nullptr);
    save();
}

int GlobalPreferences::getDefaultColumnLayout() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultColumns", 1);
}

void GlobalPreferences::setDefaultColumnLayout(int columns)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultColumns", columns, nullptr);
    save();
}

bool GlobalPreferences::getShowStatusBar() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("showStatusBar", true);
}

void GlobalPreferences::setShowStatusBar(bool show)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("showStatusBar", show, nullptr);
    save();
}

bool GlobalPreferences::getReducedMotion() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("reducedMotion", false);
}

void GlobalPreferences::setReducedMotion(bool reduced)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("reducedMotion", reduced, nullptr);
    save();
}

bool GlobalPreferences::getUIAudioFeedback() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("uiAudioFeedback", false);
}

void GlobalPreferences::setUIAudioFeedback(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("uiAudioFeedback", enabled, nullptr);
    save();
}

bool GlobalPreferences::getTooltipsEnabled() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("tooltipsEnabled", true);
}

void GlobalPreferences::setTooltipsEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("tooltipsEnabled", enabled, nullptr);
    save();
}

int GlobalPreferences::getDefaultSidebarWidth() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultSidebarWidth", 280);
}

void GlobalPreferences::setDefaultSidebarWidth(int width)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultSidebarWidth", width, nullptr);
    save();
}

} // namespace oscil
