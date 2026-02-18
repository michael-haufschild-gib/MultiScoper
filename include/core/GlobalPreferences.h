/*
    Oscil - Global User Preferences
    Stored separately from project state (user application data directory)
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <mutex>

namespace oscil
{

/**
 * Global user preferences (stored separately from project state)
 */
class GlobalPreferences
{
public:
    GlobalPreferences();
    ~GlobalPreferences();

    /**
     * Get the preferences file
     */
    [[nodiscard]] juce::File getPreferencesFile() const;

    /**
     * Load preferences from disk
     */
    void load();

    /**
     * Save preferences to disk
     */
    void save();

    // Preference accessors
    [[nodiscard]] juce::String getDefaultTheme() const;
    void setDefaultTheme(const juce::String& themeName);

    [[nodiscard]] int getDefaultColumnLayout() const;
    void setDefaultColumnLayout(int columns);

    [[nodiscard]] bool getShowStatusBar() const;
    void setShowStatusBar(bool show);

    // UI customization settings
    [[nodiscard]] bool getReducedMotion() const;
    void setReducedMotion(bool reduced);

    [[nodiscard]] bool getUIAudioFeedback() const;
    void setUIAudioFeedback(bool enabled);

    [[nodiscard]] bool getTooltipsEnabled() const;
    void setTooltipsEnabled(bool enabled);

    [[nodiscard]] int getDefaultSidebarWidth() const;
    void setDefaultSidebarWidth(int width);

    GlobalPreferences(const GlobalPreferences&) = delete;
    GlobalPreferences& operator=(const GlobalPreferences&) = delete;

private:
    // Internal save that assumes lock is already held
    // Returns true if save succeeded, false on error (errors are logged)
    bool saveUnlocked();

    // Schedule an async save with debouncing
    void scheduleSave();

    mutable std::mutex mutex_;  // Thread safety for multi-instance access
    juce::ValueTree preferences_;
    std::atomic<bool> savePending_{false};  // Debounce flag for async saves

    JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalPreferences)
};

} // namespace oscil
