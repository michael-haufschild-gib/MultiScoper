/*
    Oscil - Global Preferences
    User preferences stored separately from per-project plugin state
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <mutex>

namespace oscil
{

/**
 * Global user preferences (stored separately from project state).
 * Persisted to a user-level file so that settings survive across DAW sessions.
 *
 * Thread safety: All public methods acquire mutex_ internally, making them
 * safe to call from any thread. Multi-instance access (several plugin
 * instances in the same host) is serialized through the same mutex.
 */
class GlobalPreferences
{
public:
    /// Create global preferences, loading from disk if available.
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
    /** Persist preferences to disk without acquiring mutex_ (caller must hold it). */
    void saveUnlocked();

    mutable std::mutex mutex_;  // Thread safety for multi-instance access
    juce::ValueTree preferences_;
};

} // namespace oscil
