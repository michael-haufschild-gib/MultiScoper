/*
    Oscil - Transport Sync Component Tests
    Tests for OscilTransportSync UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTransportSync.h"

using namespace oscil;

class OscilTransportSyncTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, DefaultConstruction)
{
    OscilTransportSync transport;

    EXPECT_NEAR(transport.getCurrentBPM(), 120.0, 0.01);
    EXPECT_TRUE(transport.isSyncEnabled());
    EXPECT_EQ(transport.getNumerator(), 4);
    EXPECT_EQ(transport.getDenominator(), 4);
}

TEST_F(OscilTransportSyncTest, ConstructionWithTestId)
{
    OscilTransportSync transport("transport-1");

    EXPECT_NEAR(transport.getCurrentBPM(), 120.0, 0.01);
}

// =============================================================================
// Manual BPM Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, SetManualBPM)
{
    OscilTransportSync transport;
    transport.setSyncEnabled(false);

    transport.setManualBPM(140.0);
    EXPECT_NEAR(transport.getManualBPM(), 140.0, 0.01);
    EXPECT_NEAR(transport.getCurrentBPM(), 140.0, 0.01);
}

TEST_F(OscilTransportSyncTest, SetBPMRange)
{
    OscilTransportSync transport;

    transport.setBPMRange(40.0, 250.0);
    EXPECT_NEAR(transport.getMinBPM(), 40.0, 0.01);
    EXPECT_NEAR(transport.getMaxBPM(), 250.0, 0.01);
}

TEST_F(OscilTransportSyncTest, BPMRangeClamping)
{
    OscilTransportSync transport;
    transport.setBPMRange(60.0, 200.0);

    transport.setManualBPM(300.0);
    EXPECT_LE(transport.getManualBPM(), 200.0);

    transport.setManualBPM(30.0);
    EXPECT_GE(transport.getManualBPM(), 60.0);
}

// =============================================================================
// Sync Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, DefaultSyncEnabled)
{
    OscilTransportSync transport;

    EXPECT_TRUE(transport.isSyncEnabled());
}

TEST_F(OscilTransportSyncTest, SetSyncEnabled)
{
    OscilTransportSync transport;

    transport.setSyncEnabled(false);
    EXPECT_FALSE(transport.isSyncEnabled());

    transport.setSyncEnabled(true);
    EXPECT_TRUE(transport.isSyncEnabled());
}

TEST_F(OscilTransportSyncTest, CurrentBPMDependsOnSync)
{
    OscilTransportSync transport;

    juce::AudioPlayHead::PositionInfo info;
    info.setBpm(180.0);
    transport.updateTransportInfo(info);

    transport.setManualBPM(100.0);

    transport.setSyncEnabled(true);
    EXPECT_NEAR(transport.getCurrentBPM(), 180.0, 0.01);

    transport.setSyncEnabled(false);
    EXPECT_NEAR(transport.getCurrentBPM(), 100.0, 0.01);
}

// =============================================================================
// Display Options Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, SetShowTimeSignature)
{
    OscilTransportSync transport;

    transport.setShowTimeSignature(true);
    EXPECT_TRUE(transport.getShowTimeSignature());

    transport.setShowTimeSignature(false);
    EXPECT_FALSE(transport.getShowTimeSignature());
}

TEST_F(OscilTransportSyncTest, SetShowBeatIndicator)
{
    OscilTransportSync transport;

    transport.setShowBeatIndicator(true);
    EXPECT_TRUE(transport.getShowBeatIndicator());

    transport.setShowBeatIndicator(false);
    EXPECT_FALSE(transport.getShowBeatIndicator());
}

TEST_F(OscilTransportSyncTest, SetCompactMode)
{
    OscilTransportSync transport;

    transport.setCompactMode(true);
    EXPECT_TRUE(transport.isCompactMode());

    transport.setCompactMode(false);
    EXPECT_FALSE(transport.isCompactMode());
}

// =============================================================================
// Transport Info Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, UpdateTransportInfo)
{
    OscilTransportSync transport;

    juce::AudioPlayHead::PositionInfo info;
    info.setBpm(145.0);
    info.setTimeSignature(juce::AudioPlayHead::TimeSignature{3, 4});
    info.setIsPlaying(true);

    transport.updateTransportInfo(info);

    // When synced, should use host BPM
    EXPECT_NEAR(transport.getCurrentBPM(), 145.0, 0.01);
    EXPECT_EQ(transport.getNumerator(), 3);
    EXPECT_EQ(transport.getDenominator(), 4);
    EXPECT_TRUE(transport.isHostPlaying());
}

TEST_F(OscilTransportSyncTest, HostNotPlaying)
{
    OscilTransportSync transport;

    juce::AudioPlayHead::PositionInfo info;
    info.setBpm(120.0);
    info.setIsPlaying(false);

    transport.updateTransportInfo(info);

    EXPECT_FALSE(transport.isHostPlaying());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, OnSyncChangedCallback)
{
    OscilTransportSync transport;

    int changeCount = 0;
    bool lastState = true;

    transport.onSyncChanged = [&changeCount, &lastState](bool enabled) {
        changeCount++;
        lastState = enabled;
    };

    transport.setSyncEnabled(false);
    EXPECT_EQ(changeCount, 1);
    EXPECT_FALSE(lastState);
}

TEST_F(OscilTransportSyncTest, OnManualBPMChangedCallback)
{
    OscilTransportSync transport;
    transport.setSyncEnabled(false);

    int changeCount = 0;
    double lastBPM = 0.0;

    transport.onManualBPMChanged = [&changeCount, &lastBPM](double bpm) {
        changeCount++;
        lastBPM = bpm;
    };

    transport.setManualBPM(150.0);
    EXPECT_EQ(changeCount, 1);
    EXPECT_NEAR(lastBPM, 150.0, 0.01);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, PreferredWidthPositive)
{
    OscilTransportSync transport;

    EXPECT_GT(transport.getPreferredWidth(), 0);
}

TEST_F(OscilTransportSyncTest, PreferredHeightPositive)
{
    OscilTransportSync transport;

    EXPECT_GT(transport.getPreferredHeight(), 0);
}

TEST_F(OscilTransportSyncTest, CompactModeReducesSize)
{
    OscilTransportSync transport;

    transport.setCompactMode(false);
    int normalWidth = transport.getPreferredWidth();
    int normalHeight = transport.getPreferredHeight();

    transport.setCompactMode(true);
    int compactWidth = transport.getPreferredWidth();
    int compactHeight = transport.getPreferredHeight();

    EXPECT_LT(compactWidth, normalWidth);
    EXPECT_LT(compactHeight, normalHeight);
}

TEST_F(OscilTransportSyncTest, DisplayOptionsAffectSize)
{
    OscilTransportSync transport;
    transport.setCompactMode(false);

    transport.setShowTimeSignature(false);
    transport.setShowBeatIndicator(false);
    int minimalWidth = transport.getPreferredWidth();

    transport.setShowTimeSignature(true);
    transport.setShowBeatIndicator(true);
    int fullWidth = transport.getPreferredWidth();

    EXPECT_GT(fullWidth, minimalWidth);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, ThemeChangeDoesNotThrow)
{
    OscilTransportSync transport;
    transport.setManualBPM(130.0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    transport.themeChanged(newTheme);

    // BPM should be preserved
    EXPECT_NEAR(transport.getManualBPM(), 130.0, 0.01);
}

TEST_F(OscilTransportSyncTest, ThemeChangePreservesDisplayOptions)
{
    OscilTransportSync transport;
    transport.setShowTimeSignature(false);
    transport.setCompactMode(true);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    transport.themeChanged(newTheme);

    EXPECT_FALSE(transport.getShowTimeSignature());
    EXPECT_TRUE(transport.isCompactMode());
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilTransportSyncTest, DoesNotWantKeyboardFocusDirectly)
{
    OscilTransportSync transport;

    // TransportSync handles interaction through mouse events, not keyboard focus
    EXPECT_FALSE(transport.getWantsKeyboardFocus());
}
