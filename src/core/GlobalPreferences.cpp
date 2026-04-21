/*
    MultiScoper - Global Preferences Implementation
    User-wide preferences stored separately from project state
*/

#include "core/GlobalPreferences.h"

namespace multiscoper
{

// Sidebar width bounds — must match WindowLayout::MIN/MAX/DEFAULT_SIDEBAR_WIDTH.
// Duplicated here because core/ must not depend on ui/.
static constexpr int SIDEBAR_MIN = 200;
static constexpr int SIDEBAR_MAX = 800;
static constexpr int SIDEBAR_DEFAULT = 300;

GlobalPreferences::GlobalPreferences()
{
    preferences_ = juce::ValueTree("GlobalPreferences");
    load();
}

GlobalPreferences::~GlobalPreferences() {}

juce::File GlobalPreferences::getPreferencesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    appDataDir = appDataDir.getChildFile("Application Support");
#endif
    return appDataDir.getChildFile("MultiScoper").getChildFile("preferences.xml");
}

void GlobalPreferences::load()
{
    std::scoped_lock const lock(mutex_);
    auto file = getPreferencesFile();
    if (!file.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse(file);
    if (!xml)
        return; // Malformed XML — keep the default-constructed tree.

    auto loaded = juce::ValueTree::fromXml(*xml);
    // Only adopt the loaded tree when it matches our canonical root type.
    // A valid-but-wrong-type tree (from format drift or a tampered file) is
    // rejected to prevent persisting with the wrong root on the next save().
    if (loaded.isValid() && loaded.hasType("GlobalPreferences"))
    {
        preferences_ = loaded;
        clampPersistedValues();
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

int GlobalPreferences::getDefaultColumnLayout() const { return juce::jlimit(1, 3, getPref<int>("defaultColumns", 1)); }
void GlobalPreferences::setDefaultColumnLayout(int columns) { setPref("defaultColumns", juce::jlimit(1, 3, columns)); }

bool GlobalPreferences::getShowStatusBar() const { return getPref<bool>("showStatusBar", true); }
void GlobalPreferences::setShowStatusBar(bool show) { setPref("showStatusBar", show); }

bool GlobalPreferences::getReducedMotion() const { return getPref<bool>("reducedMotion", false); }
void GlobalPreferences::setReducedMotion(bool reduced) { setPref("reducedMotion", reduced); }

bool GlobalPreferences::getUIAudioFeedback() const { return getPref<bool>("uiAudioFeedback", false); }
void GlobalPreferences::setUIAudioFeedback(bool enabled) { setPref("uiAudioFeedback", enabled); }

bool GlobalPreferences::getTooltipsEnabled() const { return getPref<bool>("tooltipsEnabled", true); }
void GlobalPreferences::setTooltipsEnabled(bool enabled) { setPref("tooltipsEnabled", enabled); }

int GlobalPreferences::getDefaultSidebarWidth() const
{
    return juce::jlimit(SIDEBAR_MIN, SIDEBAR_MAX, getPref<int>("defaultSidebarWidth", SIDEBAR_DEFAULT));
}
void GlobalPreferences::setDefaultSidebarWidth(int width)
{
    setPref("defaultSidebarWidth", juce::jlimit(SIDEBAR_MIN, SIDEBAR_MAX, width));
}

void GlobalPreferences::clampPersistedValues()
{
    // Sanitize values loaded from disk so out-of-range data from older versions
    // or manual edits cannot propagate through the rest of the application.
    auto clampInt = [this](const juce::Identifier& key, int lo, int hi, int fallback) {
        int const raw = static_cast<int>(preferences_.getProperty(key, fallback));
        int const clamped = juce::jlimit(lo, hi, raw);
        if (clamped != raw)
            preferences_.setProperty(key, clamped, nullptr);
    };

    clampInt("defaultColumns", 1, 3, 1);
    clampInt("defaultSidebarWidth", SIDEBAR_MIN, SIDEBAR_MAX, SIDEBAR_DEFAULT);
}

} // namespace multiscoper
