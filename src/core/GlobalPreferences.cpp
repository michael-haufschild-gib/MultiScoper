/*
    Oscil - Global User Preferences Implementation
*/

#include "core/GlobalPreferences.h"

namespace oscil
{

GlobalPreferences::GlobalPreferences()
{
    preferences_ = juce::ValueTree("GlobalPreferences");
    load();
}

GlobalPreferences::~GlobalPreferences()
{
    if (savePending_.load())
    {
        juce::Logger::writeToLog("GlobalPreferences: Destructor called with pending save - preferences may not be persisted");
    }
}

juce::File GlobalPreferences::getPreferencesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appDataDir.getChildFile("MultiScoper").getChildFile("preferences.xml");
}

void GlobalPreferences::load()
{
    std::lock_guard<std::mutex> lock(mutex_);
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
    std::lock_guard<std::mutex> lock(mutex_);
    saveUnlocked();
}

bool GlobalPreferences::saveUnlocked()
{
    // NOTE: Caller must hold mutex_ before calling this method
    auto file = getPreferencesFile();

    // Create parent directory if it doesn't exist
    auto parentDir = file.getParentDirectory();
    if (!parentDir.exists())
    {
        auto result = parentDir.createDirectory();
        if (result.failed())
        {
            juce::Logger::writeToLog("GlobalPreferences::save() - Failed to create directory: " + result.getErrorMessage());
            return false;
        }
    }

    if (auto xml = preferences_.createXml())
    {
        if (!xml->writeTo(file))
        {
            juce::Logger::writeToLog("GlobalPreferences::save() - Failed to write preferences to: " + file.getFullPathName());
            return false;
        }
        return true;
    }
    else
    {
        juce::Logger::writeToLog("GlobalPreferences::save() - Failed to create XML from preferences");
        return false;
    }
}

void GlobalPreferences::scheduleSave()
{
    // Debounce: only schedule one save at a time
    if (!savePending_.exchange(true))
    {
        // Use WeakReference to safely handle case where GlobalPreferences
        // is destroyed before the async callback executes
        juce::MessageManager::callAsync(
            [weakThis = juce::WeakReference<GlobalPreferences>(this)]() {
                if (auto* self = weakThis.get())
                {
                    if (self->savePending_.load())
                    {
                        std::lock_guard<std::mutex> lock(self->mutex_);
                        self->saveUnlocked();
                        self->savePending_.store(false);
                    }
                }
            });
    }
}

juce::String GlobalPreferences::getDefaultTheme() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("defaultTheme", "Dark Professional");
}

void GlobalPreferences::setDefaultTheme(const juce::String& themeName)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("defaultTheme", themeName, nullptr);
    }
    scheduleSave();
}

int GlobalPreferences::getDefaultColumnLayout() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("defaultColumns", 1);
}

void GlobalPreferences::setDefaultColumnLayout(int columns)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("defaultColumns", columns, nullptr);
    }
    scheduleSave();
}

bool GlobalPreferences::getShowStatusBar() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("showStatusBar", true);
}

void GlobalPreferences::setShowStatusBar(bool show)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("showStatusBar", show, nullptr);
    }
    scheduleSave();
}

bool GlobalPreferences::getReducedMotion() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("reducedMotion", false);
}

void GlobalPreferences::setReducedMotion(bool reduced)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("reducedMotion", reduced, nullptr);
    }
    scheduleSave();
}

bool GlobalPreferences::getUIAudioFeedback() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("uiAudioFeedback", false);
}

void GlobalPreferences::setUIAudioFeedback(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("uiAudioFeedback", enabled, nullptr);
    }
    scheduleSave();
}

bool GlobalPreferences::getTooltipsEnabled() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("tooltipsEnabled", true);
}

void GlobalPreferences::setTooltipsEnabled(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("tooltipsEnabled", enabled, nullptr);
    }
    scheduleSave();
}

int GlobalPreferences::getDefaultSidebarWidth() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return preferences_.getProperty("defaultSidebarWidth", 280);
}

void GlobalPreferences::setDefaultSidebarWidth(int width)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        preferences_.setProperty("defaultSidebarWidth", width, nullptr);
    }
    scheduleSave();
}

} // namespace oscil
