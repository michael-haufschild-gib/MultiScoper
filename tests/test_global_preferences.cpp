/*
    Oscil - Global Preferences Tests
    Tests for user-wide preference storage, defaults, persistence, and edge cases.

    Bug targets:
    - Defaults wrong type or value after fresh construction
    - set* not persisting (saveUnlocked not called)
    - get* returning stale values after set*
    - Concurrent reads and writes corrupting ValueTree
    - Corrupt preferences file crashing on load
    - Missing preferences directory preventing save
*/

#include "core/GlobalPreferences.h"

#include <juce_core/juce_core.h>

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace oscil;

class GlobalPreferencesTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Delete existing preferences file so tests start from a clean state.
        // GlobalPreferences constructor calls load(), which would read stale data.
        auto prefsFile = getPreferencesFilePath();
        if (prefsFile.existsAsFile())
            prefsFile.deleteFile();

        prefs_ = std::make_unique<GlobalPreferences>();
    }

    void TearDown() override
    {
        prefs_.reset();

        // Clean up preferences file written by tests
        auto prefsFile = getPreferencesFilePath();
        if (prefsFile.existsAsFile())
            prefsFile.deleteFile();
    }

    static juce::File getPreferencesFilePath()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Oscil")
            .getChildFile("preferences.xml");
    }

    std::unique_ptr<GlobalPreferences> prefs_;
};

// ============================================================================
// Default Values
// ============================================================================

TEST_F(GlobalPreferencesTest, DefaultThemeIsDarkProfessional)
{
    // Bug caught: default returning empty string instead of "Dark Professional"
    EXPECT_EQ(prefs_->getDefaultTheme(), juce::String("Dark Professional"));
}

TEST_F(GlobalPreferencesTest, DefaultColumnLayoutIsOne) { EXPECT_EQ(prefs_->getDefaultColumnLayout(), 1); }

TEST_F(GlobalPreferencesTest, DefaultShowStatusBarIsTrue) { EXPECT_TRUE(prefs_->getShowStatusBar()); }

TEST_F(GlobalPreferencesTest, DefaultReducedMotionIsFalse) { EXPECT_FALSE(prefs_->getReducedMotion()); }

TEST_F(GlobalPreferencesTest, DefaultUIAudioFeedbackIsFalse) { EXPECT_FALSE(prefs_->getUIAudioFeedback()); }

TEST_F(GlobalPreferencesTest, DefaultTooltipsEnabledIsTrue) { EXPECT_TRUE(prefs_->getTooltipsEnabled()); }

TEST_F(GlobalPreferencesTest, DefaultSidebarWidthIs280) { EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 280); }

// ============================================================================
// Set-Then-Get Round Trip
// ============================================================================

TEST_F(GlobalPreferencesTest, SetAndGetTheme)
{
    prefs_->setDefaultTheme("Neon Green");
    EXPECT_EQ(prefs_->getDefaultTheme(), juce::String("Neon Green"));
}

TEST_F(GlobalPreferencesTest, SetAndGetColumnLayout)
{
    prefs_->setDefaultColumnLayout(3);
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 3);
}

TEST_F(GlobalPreferencesTest, SetAndGetShowStatusBar)
{
    prefs_->setShowStatusBar(false);
    EXPECT_FALSE(prefs_->getShowStatusBar());

    prefs_->setShowStatusBar(true);
    EXPECT_TRUE(prefs_->getShowStatusBar());
}

TEST_F(GlobalPreferencesTest, SetAndGetReducedMotion)
{
    prefs_->setReducedMotion(true);
    EXPECT_TRUE(prefs_->getReducedMotion());
}

TEST_F(GlobalPreferencesTest, SetAndGetUIAudioFeedback)
{
    prefs_->setUIAudioFeedback(true);
    EXPECT_TRUE(prefs_->getUIAudioFeedback());
}

TEST_F(GlobalPreferencesTest, SetAndGetTooltipsEnabled)
{
    prefs_->setTooltipsEnabled(false);
    EXPECT_FALSE(prefs_->getTooltipsEnabled());
}

TEST_F(GlobalPreferencesTest, SetAndGetSidebarWidth)
{
    prefs_->setDefaultSidebarWidth(450);
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 450);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(GlobalPreferencesTest, EmptyThemeNameRoundTrip)
{
    // Bug caught: empty string stored as juce::var() instead of ""
    prefs_->setDefaultTheme("");
    EXPECT_EQ(prefs_->getDefaultTheme(), juce::String(""));
}

TEST_F(GlobalPreferencesTest, SpecialCharactersInThemeName)
{
    // Bug caught: XML special characters corrupting preferences file
    prefs_->setDefaultTheme("Theme <with> \"quotes\" & 'apostrophes'");
    EXPECT_EQ(prefs_->getDefaultTheme(), juce::String("Theme <with> \"quotes\" & 'apostrophes'"));
}

TEST_F(GlobalPreferencesTest, NegativeColumnLayout)
{
    // GlobalPreferences stores raw int — caller is responsible for clamping.
    // Test verifies no crash or silent corruption.
    prefs_->setDefaultColumnLayout(-5);
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), -5);
}

TEST_F(GlobalPreferencesTest, ZeroSidebarWidth)
{
    prefs_->setDefaultSidebarWidth(0);
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 0);
}

TEST_F(GlobalPreferencesTest, NegativeSidebarWidth)
{
    prefs_->setDefaultSidebarWidth(-100);
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), -100);
}

TEST_F(GlobalPreferencesTest, VeryLargeSidebarWidth)
{
    prefs_->setDefaultSidebarWidth(100000);
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 100000);
}

TEST_F(GlobalPreferencesTest, PreferencesFilePathIsValid)
{
    auto file = prefs_->getPreferencesFile();
    EXPECT_TRUE(file.getFullPathName().isNotEmpty());
    EXPECT_TRUE(file.getFullPathName().contains("Oscil"));
    EXPECT_TRUE(file.getFullPathName().contains("preferences.xml"));
}

// ============================================================================
// Multiple Set Operations Preserve Latest Value
// ============================================================================

TEST_F(GlobalPreferencesTest, RapidSetOverwritesCorrectly)
{
    // Bug caught: race between save and subsequent set causing stale values
    for (int i = 0; i < 100; ++i)
    {
        prefs_->setDefaultColumnLayout(i);
    }
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 99);
}

TEST_F(GlobalPreferencesTest, InterleavedSetsDontCorrupt)
{
    // Bug caught: setting different properties interleaved causes cross-property corruption
    prefs_->setDefaultTheme("Theme A");
    prefs_->setDefaultColumnLayout(2);
    prefs_->setShowStatusBar(false);
    prefs_->setDefaultTheme("Theme B");
    prefs_->setDefaultColumnLayout(3);

    EXPECT_EQ(prefs_->getDefaultTheme(), juce::String("Theme B"));
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 3);
    EXPECT_FALSE(prefs_->getShowStatusBar());
}

// ============================================================================
// Concurrent Access Safety
// ============================================================================

TEST_F(GlobalPreferencesTest, ConcurrentReadsDoNotCorruptDuringWrite)
{
    // Bug caught: mutex_ not held correctly, causing torn reads or ValueTree corruption.
    // GlobalPreferences holds the mutex during disk I/O, so writer throughput is low.
    // Instead of timing-based concurrency, we interleave explicit reads and writes
    // from two threads to verify correctness.

    // Pre-set to known state
    prefs_->setDefaultColumnLayout(5);
    prefs_->setDefaultTheme("ConcurrentTest");

    std::atomic<bool> writerDone{false};
    std::atomic<int> readErrors{0};

    // Writer: do a small number of writes (each involves disk I/O)
    std::thread writer([&]() {
        for (int i = 0; i < 10; ++i)
        {
            prefs_->setDefaultColumnLayout(i % 10);
        }
        writerDone.store(true, std::memory_order_release);
    });

    // Reader: continuously read while writer is active
    std::thread reader([&]() {
        while (!writerDone.load(std::memory_order_acquire))
        {
            int cols = prefs_->getDefaultColumnLayout();
            if (cols < 0 || cols >= 10)
                readErrors.fetch_add(1, std::memory_order_relaxed);
        }
    });

    writer.join();
    reader.join();

    EXPECT_EQ(readErrors.load(), 0) << "Reader saw invalid column layout during concurrent writes";

    // Final state must be deterministic (last write wins)
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 9);
}

// ============================================================================
// Persistence Round-Trip via Save/Load
// ============================================================================

TEST_F(GlobalPreferencesTest, SaveAndLoadPreservesAllSettings)
{
    // Set non-default values for every property
    prefs_->setDefaultTheme("Custom Theme");
    prefs_->setDefaultColumnLayout(2);
    prefs_->setShowStatusBar(false);
    prefs_->setReducedMotion(true);
    prefs_->setUIAudioFeedback(true);
    prefs_->setTooltipsEnabled(false);
    prefs_->setDefaultSidebarWidth(350);

    // Force save (setters already call saveUnlocked)
    prefs_->save();

    // Create new instance — constructor calls load()
    auto prefs2 = std::make_unique<GlobalPreferences>();

    EXPECT_EQ(prefs2->getDefaultTheme(), juce::String("Custom Theme"));
    EXPECT_EQ(prefs2->getDefaultColumnLayout(), 2);
    EXPECT_FALSE(prefs2->getShowStatusBar());
    EXPECT_TRUE(prefs2->getReducedMotion());
    EXPECT_TRUE(prefs2->getUIAudioFeedback());
    EXPECT_FALSE(prefs2->getTooltipsEnabled());
    EXPECT_EQ(prefs2->getDefaultSidebarWidth(), 350);
}
