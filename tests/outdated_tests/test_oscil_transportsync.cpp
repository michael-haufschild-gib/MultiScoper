/*
    Oscil - Transport Sync Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTransportSync.h"

using namespace oscil;

class OscilTransportSyncTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilTransportSyncTest, DefaultConstruction)
{
    OscilTransportSync transport;

    EXPECT_NEAR(transport.getCurrentBPM(), 120.0, 0.01);
    EXPECT_TRUE(transport.isSyncEnabled());
    EXPECT_EQ(transport.getNumerator(), 4);
    EXPECT_EQ(transport.getDenominator(), 4);
}

// Test: Set manual BPM
TEST_F(OscilTransportSyncTest, SetManualBPM)
{
    OscilTransportSync transport;
    transport.setSyncEnabled(false);

    transport.setManualBPM(140.0);
    EXPECT_NEAR(transport.getManualBPM(), 140.0, 0.01);
    EXPECT_NEAR(transport.getCurrentBPM(), 140.0, 0.01);
}

// Test: BPM range clamping
TEST_F(OscilTransportSyncTest, BPMRangeClamping)
{
    OscilTransportSync transport;
    transport.setBPMRange(60.0, 200.0);

    transport.setManualBPM(300.0);
    EXPECT_LE(transport.getManualBPM(), 200.0);

    transport.setManualBPM(30.0);
    EXPECT_GE(transport.getManualBPM(), 60.0);
}

// Test: Set BPM range
TEST_F(OscilTransportSyncTest, SetBPMRange)
{
    OscilTransportSync transport;

    transport.setBPMRange(40.0, 250.0);
    EXPECT_NEAR(transport.getMinBPM(), 40.0, 0.01);
    EXPECT_NEAR(transport.getMaxBPM(), 250.0, 0.01);
}

// Test: Sync enabled/disabled
TEST_F(OscilTransportSyncTest, SyncEnabled)
{
    OscilTransportSync transport;

    EXPECT_TRUE(transport.isSyncEnabled());

    transport.setSyncEnabled(false);
    EXPECT_FALSE(transport.isSyncEnabled());

    transport.setSyncEnabled(true);
    EXPECT_TRUE(transport.isSyncEnabled());
}

// Test: Show time signature
TEST_F(OscilTransportSyncTest, ShowTimeSignature)
{
    OscilTransportSync transport;

    transport.setShowTimeSignature(true);
    EXPECT_TRUE(transport.getShowTimeSignature());

    transport.setShowTimeSignature(false);
    EXPECT_FALSE(transport.getShowTimeSignature());
}

// Test: Show beat indicator
TEST_F(OscilTransportSyncTest, ShowBeatIndicator)
{
    OscilTransportSync transport;

    transport.setShowBeatIndicator(true);
    EXPECT_TRUE(transport.getShowBeatIndicator());

    transport.setShowBeatIndicator(false);
    EXPECT_FALSE(transport.getShowBeatIndicator());
}

// Test: Compact mode
TEST_F(OscilTransportSyncTest, CompactMode)
{
    OscilTransportSync transport;

    transport.setCompactMode(true);
    EXPECT_TRUE(transport.isCompactMode());

    transport.setCompactMode(false);
    EXPECT_FALSE(transport.isCompactMode());
}

// Test: Sync changed callback
TEST_F(OscilTransportSyncTest, SyncChangedCallback)
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

// Test: Manual BPM changed callback
TEST_F(OscilTransportSyncTest, ManualBPMChangedCallback)
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

// Test: Preferred size - compact vs normal
TEST_F(OscilTransportSyncTest, PreferredSizeCompactVsNormal)
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

// Test: Preferred size with options
TEST_F(OscilTransportSyncTest, PreferredSizeWithOptions)
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

// Test: Update transport info
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

// Test: Current BPM depends on sync
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

// Test: Theme changes
TEST_F(OscilTransportSyncTest, ThemeChanges)
{
    OscilTransportSync transport;
    transport.setManualBPM(130.0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    transport.themeChanged(newTheme);

    // BPM should be preserved
    EXPECT_NEAR(transport.getManualBPM(), 130.0, 0.01);
}

// Test: Accessibility
TEST_F(OscilTransportSyncTest, Accessibility)
{
    OscilTransportSync transport;

    auto* handler = transport.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}

// Test: Focus handling
TEST_F(OscilTransportSyncTest, FocusHandling)
{
    OscilTransportSync transport;
    // Transport should accept keyboard focus for BPM adjustment
    EXPECT_TRUE(transport.getWantsKeyboardFocus());
}
