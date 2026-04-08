/*
    Oscil - Global Preferences Implementation
    User-wide preferences stored separately from project state
*/

#include "core/GlobalPreferences.h"

namespace oscil
{

GlobalPreferences::GlobalPreferences()
{
    preferences_ = juce::ValueTree("GlobalPreferences");
    load();
}

GlobalPreferences::~GlobalPreferences() {}

juce::File GlobalPreferences::getPreferencesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appDataDir.getChildFile("Oscil").getChildFile("preferences.xml");
}

void GlobalPreferences::load()
{
    std::scoped_lock const lock(mutex_);
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
    std::scoped_lock const lock(mutex_);
    saveUnlocked();
}

void GlobalPreferences::saveUnlocked()
{
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
    return getPref<juce::String>("defaultTheme", "Dark Professional");
}
void GlobalPreferences::setDefaultTheme(const juce::String& themeName) { setPref("defaultTheme", themeName); }

int GlobalPreferences::getDefaultColumnLayout() const { return getPref<int>("defaultColumns", 1); }
void GlobalPreferences::setDefaultColumnLayout(int columns) { setPref("defaultColumns", juce::jlimit(1, 8, columns)); }

bool GlobalPreferences::getShowStatusBar() const { return getPref<bool>("showStatusBar", true); }
void GlobalPreferences::setShowStatusBar(bool show) { setPref("showStatusBar", show); }

bool GlobalPreferences::getReducedMotion() const { return getPref<bool>("reducedMotion", false); }
void GlobalPreferences::setReducedMotion(bool reduced) { setPref("reducedMotion", reduced); }

bool GlobalPreferences::getUIAudioFeedback() const { return getPref<bool>("uiAudioFeedback", false); }
void GlobalPreferences::setUIAudioFeedback(bool enabled) { setPref("uiAudioFeedback", enabled); }

bool GlobalPreferences::getTooltipsEnabled() const { return getPref<bool>("tooltipsEnabled", true); }
void GlobalPreferences::setTooltipsEnabled(bool enabled) { setPref("tooltipsEnabled", enabled); }

int GlobalPreferences::getDefaultSidebarWidth() const { return getPref<int>("defaultSidebarWidth", 280); }
void GlobalPreferences::setDefaultSidebarWidth(int width)
{
    setPref("defaultSidebarWidth", juce::jlimit(150, 600, width));
}

} // namespace oscil
