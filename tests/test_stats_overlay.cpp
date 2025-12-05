/*
    Oscil - Stats Overlay Tests
*/

#include <gtest/gtest.h>
#include "ui/layout/pane/overlays/StatsOverlay.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class StatsOverlayTest : public ::testing::Test
{
protected:
    ThemeManager themeManager;
    StatsOverlay overlay{themeManager};
};

TEST_F(StatsOverlayTest, InitialState)
{
    // Should be empty initially
    EXPECT_EQ(overlay.getDisplayedText(), "");
}

TEST_F(StatsOverlayTest, UpdateStatsWithSingleOscillator)
{
    std::vector<OscillatorStats> stats;
    OscillatorStats os;
    os.name = "Osc1";
    os.left.rmsDb = -6.0f;
    os.right.rmsDb = -6.0f;
    os.left.peakDb = -1.0f;
    stats.push_back(os);
    
    overlay.updateStats(stats);
    
    juce::String text = overlay.getDisplayedText();
    
    // Check for key strings
    EXPECT_TRUE(text.contains("Osc1"));
    EXPECT_TRUE(text.contains("-6.0 dB"));
    EXPECT_TRUE(text.contains("-1.0 dB"));
}

TEST_F(StatsOverlayTest, UpdateStatsWithMultipleOscillators)
{
    std::vector<OscillatorStats> stats;
    
    OscillatorStats os1;
    os1.name = "Osc1";
    os1.left.rmsDb = -10.0f;
    stats.push_back(os1);
    
    OscillatorStats os2;
    os2.name = "Osc2";
    os2.left.rmsDb = -20.0f;
    stats.push_back(os2);
    
    overlay.updateStats(stats);
    
    juce::String text = overlay.getDisplayedText();
    
    // Check headers
    EXPECT_TRUE(text.contains("Osc1"));
    EXPECT_TRUE(text.contains("Osc2"));
    
    // Check values
    EXPECT_TRUE(text.contains("-10.0 dB"));
    EXPECT_TRUE(text.contains("-20.0 dB"));
}

TEST_F(StatsOverlayTest, ResetCallback)
{
    bool resetCalled = false;
    overlay.onResetStats = [&]() { resetCalled = true; };
    
    // Find and click the reset button
    // The button is a child component. Since we don't have easy access to internal children via public API except by index,
    // we can simulate it if we exposed the button or finding it.
    // StatsOverlay has 2 children: TextEditor and Button.
    
    auto* button = dynamic_cast<OscilButton*>(overlay.getChildComponent(1)); // 0 is TextEditor, 1 is Button usually
    // But to be safe, iterate
    for (auto* child : overlay.getChildren())
    {
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            btn->triggerClick();
            break;
        }
    }
    
    EXPECT_TRUE(resetCalled);
}

TEST_F(StatsOverlayTest, FormattingChecks)
{
    std::vector<OscillatorStats> stats;
    OscillatorStats os;
    os.name = "Test";
    
    // Check -inf formatting
    os.left.rmsDb = -100.0f; 
    
    // Check % formatting
    os.left.dcOffset = 0.5f; // 50%
    
    // Check ms formatting
    os.left.attackTimeMs = 150.0f;
    
    stats.push_back(os);
    overlay.updateStats(stats);
    
    juce::String text = overlay.getDisplayedText();
    EXPECT_TRUE(text.contains("-inf"));
    EXPECT_TRUE(text.contains("50.0%"));
    EXPECT_TRUE(text.contains("150 ms"));
}
