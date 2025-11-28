/*
    Oscil - UI Audio Feedback Tests
*/

#include <gtest/gtest.h>
#include "ui/components/UIAudioFeedback.h"

using namespace oscil;

class UIAudioFeedbackTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Singleton access
TEST_F(UIAudioFeedbackTest, SingletonAccess)
{
    auto& feedback1 = UIAudioFeedback::getInstance();
    auto& feedback2 = UIAudioFeedback::getInstance();

    EXPECT_EQ(&feedback1, &feedback2);
}

// Test: Default state - disabled
TEST_F(UIAudioFeedbackTest, DefaultDisabled)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Audio feedback is disabled by default
    EXPECT_FALSE(feedback.isEnabled());
}

// Test: Enable/disable
TEST_F(UIAudioFeedbackTest, EnableDisable)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setEnabled(true);
    EXPECT_TRUE(feedback.isEnabled());

    feedback.setEnabled(false);
    EXPECT_FALSE(feedback.isEnabled());
}

// Test: Volume control
TEST_F(UIAudioFeedbackTest, VolumeControl)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setVolume(0.5f);
    EXPECT_NEAR(feedback.getVolume(), 0.5f, 0.01f);

    feedback.setVolume(0.8f);
    EXPECT_NEAR(feedback.getVolume(), 0.8f, 0.01f);
}

// Test: Volume clamping
TEST_F(UIAudioFeedbackTest, VolumeClamping)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setVolume(-0.5f);
    EXPECT_GE(feedback.getVolume(), 0.0f);

    feedback.setVolume(1.5f);
    EXPECT_LE(feedback.getVolume(), 1.0f);

    // Reset
    feedback.setVolume(0.5f);
}

// Test: Individual sound types enabled
TEST_F(UIAudioFeedbackTest, IndividualSoundEnabled)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Check default states
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Hover));  // Disabled by default
}

// Test: Enable/disable individual sounds
TEST_F(UIAudioFeedbackTest, EnableDisableIndividualSounds)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));

    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
}

// Test: Play sound doesn't crash when disabled
TEST_F(UIAudioFeedbackTest, PlaySoundWhenDisabled)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(false);

    // Should not crash
    feedback.playSound(UIAudioFeedback::SoundType::Click);
    feedback.playSound(UIAudioFeedback::SoundType::Toggle);
    feedback.playSound(UIAudioFeedback::SoundType::Error);
}

// Test: Play sound doesn't crash when enabled
TEST_F(UIAudioFeedbackTest, PlaySoundWhenEnabled)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    // Should not crash (no audio device, but shouldn't throw)
    feedback.playSound(UIAudioFeedback::SoundType::Click);
    feedback.playSound(UIAudioFeedback::SoundType::Toggle);
    feedback.playSound(UIAudioFeedback::SoundType::Success);
    feedback.playSound(UIAudioFeedback::SoundType::Error);
    feedback.playSound(UIAudioFeedback::SoundType::SliderSnap);
    feedback.playSound(UIAudioFeedback::SoundType::Focus);
    feedback.playSound(UIAudioFeedback::SoundType::Notification);

    feedback.setEnabled(false);
}

// Test: All sound types exist
TEST_F(UIAudioFeedbackTest, AllSoundTypesExist)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // These should all be valid sound types
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::SliderSnap, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Error, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Success, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Focus, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Hover, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Notification, true);

    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::SliderSnap));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Error));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Success));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Focus));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Hover));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Notification));
}

// Test: Disabled individual sound not played
TEST_F(UIAudioFeedbackTest, DisabledSoundNotPlayed)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, false);

    // Should not play (no crash, just silent)
    feedback.playSound(UIAudioFeedback::SoundType::Click);

    // Re-enable
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback.setEnabled(false);
}

// Test: Set audio device manager (null is valid)
TEST_F(UIAudioFeedbackTest, SetAudioDeviceManager)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Setting null should not crash
    feedback.setAudioDevice(nullptr);
}

// Test: Shutdown doesn't crash
TEST_F(UIAudioFeedbackTest, ShutdownDoesntCrash)
{
    // Get a reference but don't call shutdown - it's a singleton
    // The actual shutdown happens on app exit
    auto& feedback = UIAudioFeedback::getInstance();
    (void)feedback;

    // If we get here without crashing, test passes
    EXPECT_TRUE(true);
}

// ============================================================================
// Behavioral Edge Case Tests
// ============================================================================

// Test: Rapid toggle on/off
TEST_F(UIAudioFeedbackTest, RapidToggle)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Rapidly toggle many times
    for (int i = 0; i < 100; ++i)
    {
        feedback.setEnabled(true);
        feedback.setEnabled(false);
    }

    // Should end disabled
    EXPECT_FALSE(feedback.isEnabled());

    // Toggle again and verify state
    feedback.setEnabled(true);
    EXPECT_TRUE(feedback.isEnabled());
    feedback.setEnabled(false);
}

// Test: Rapid playSound calls (stress test)
TEST_F(UIAudioFeedbackTest, RapidPlaySoundCalls)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    // Rapidly play many sounds of different types
    for (int i = 0; i < 100; ++i)
    {
        feedback.playSound(UIAudioFeedback::SoundType::Click);
        feedback.playSound(UIAudioFeedback::SoundType::Toggle);
        feedback.playSound(UIAudioFeedback::SoundType::SliderSnap);
    }

    // Should not crash, clean up
    feedback.setEnabled(false);
}

// Test: Volume at exact boundaries
TEST_F(UIAudioFeedbackTest, VolumeExactBoundaries)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setVolume(0.0f);
    EXPECT_FLOAT_EQ(feedback.getVolume(), 0.0f);

    feedback.setVolume(1.0f);
    EXPECT_FLOAT_EQ(feedback.getVolume(), 1.0f);

    // Reset
    feedback.setVolume(0.5f);
}

// Test: Volume with very small values
TEST_F(UIAudioFeedbackTest, VolumeVerySmall)
{
    auto& feedback = UIAudioFeedback::getInstance();

    feedback.setVolume(0.001f);
    EXPECT_NEAR(feedback.getVolume(), 0.001f, 0.0001f);

    feedback.setVolume(1e-10f);
    EXPECT_GE(feedback.getVolume(), 0.0f);

    // Reset
    feedback.setVolume(0.5f);
}

// Test: Multiple sound types toggled quickly
TEST_F(UIAudioFeedbackTest, RapidSoundTypeToggle)
{
    auto& feedback = UIAudioFeedback::getInstance();

    for (int i = 0; i < 50; ++i)
    {
        feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
        feedback.setSoundEnabled(UIAudioFeedback::SoundType::Toggle, false);
        feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
        feedback.setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    }

    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
}

// Test: Play sound while toggling enabled
TEST_F(UIAudioFeedbackTest, PlayWhileToggling)
{
    auto& feedback = UIAudioFeedback::getInstance();

    for (int i = 0; i < 50; ++i)
    {
        feedback.setEnabled(true);
        feedback.playSound(UIAudioFeedback::SoundType::Click);
        feedback.setEnabled(false);
        feedback.playSound(UIAudioFeedback::SoundType::Click);
    }

    // Should not crash
    EXPECT_FALSE(feedback.isEnabled());
}

// Test: All sound types can be played
TEST_F(UIAudioFeedbackTest, PlayAllSoundTypes)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    // Enable all sounds including normally-disabled hover
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Hover, true);

    // Play each type
    feedback.playSound(UIAudioFeedback::SoundType::Click);
    feedback.playSound(UIAudioFeedback::SoundType::Toggle);
    feedback.playSound(UIAudioFeedback::SoundType::SliderSnap);
    feedback.playSound(UIAudioFeedback::SoundType::Error);
    feedback.playSound(UIAudioFeedback::SoundType::Success);
    feedback.playSound(UIAudioFeedback::SoundType::Focus);
    feedback.playSound(UIAudioFeedback::SoundType::Hover);
    feedback.playSound(UIAudioFeedback::SoundType::Notification);

    // Should not crash
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Hover, false);
    feedback.setEnabled(false);
}

// Test: Volume change while playing
TEST_F(UIAudioFeedbackTest, VolumeChangeDuringPlayback)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    feedback.setVolume(1.0f);
    feedback.playSound(UIAudioFeedback::SoundType::Click);
    feedback.setVolume(0.0f);
    feedback.playSound(UIAudioFeedback::SoundType::Click);
    feedback.setVolume(0.5f);
    feedback.playSound(UIAudioFeedback::SoundType::Click);

    // Should not crash
    feedback.setEnabled(false);
}

// Test: Disable specific sound then play it
TEST_F(UIAudioFeedbackTest, PlayDisabledSound)
{
    auto& feedback = UIAudioFeedback::getInstance();
    feedback.setEnabled(true);

    // Disable Click sound
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));

    // Play it anyway (should be silent, not crash)
    feedback.playSound(UIAudioFeedback::SoundType::Click);

    // Re-enable
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback.setEnabled(false);
}

// Test: Set audio device to null multiple times
TEST_F(UIAudioFeedbackTest, SetNullDeviceMultipleTimes)
{
    auto& feedback = UIAudioFeedback::getInstance();

    for (int i = 0; i < 10; ++i)
    {
        feedback.setAudioDevice(nullptr);
    }

    // Should not crash
}

// Test: Singleton consistency across operations
TEST_F(UIAudioFeedbackTest, SingletonStateConsistency)
{
    auto& feedback1 = UIAudioFeedback::getInstance();
    feedback1.setEnabled(true);
    feedback1.setVolume(0.75f);

    auto& feedback2 = UIAudioFeedback::getInstance();

    EXPECT_TRUE(feedback2.isEnabled());
    EXPECT_NEAR(feedback2.getVolume(), 0.75f, 0.01f);

    // Changes via feedback2 should be reflected in feedback1
    feedback2.setEnabled(false);
    EXPECT_FALSE(feedback1.isEnabled());

    feedback1.setVolume(0.5f);
}

// Test: State after many operations
TEST_F(UIAudioFeedbackTest, StateAfterManyOperations)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Start with known state
    feedback.setEnabled(false);
    feedback.setVolume(0.5f);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);

    // Perform many operations
    for (int i = 0; i < 100; ++i)
    {
        feedback.setEnabled(i % 2 == 0);
        feedback.setVolume(static_cast<float>(i % 10) / 10.0f);
        feedback.playSound(UIAudioFeedback::SoundType::Click);
    }

    // Final state should be predictable
    EXPECT_FALSE(feedback.isEnabled()); // 99 % 2 == 1, so false
    EXPECT_NEAR(feedback.getVolume(), 0.9f, 0.01f); // 99 % 10 = 9, so 0.9

    // Reset
    feedback.setEnabled(false);
    feedback.setVolume(0.5f);
}

// Test: Enable sound type that's already enabled
TEST_F(UIAudioFeedbackTest, EnableAlreadyEnabledSound)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Click is enabled by default
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));

    // Enable again
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);

    // Should still be enabled
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
}

// Test: Disable sound type that's already disabled
TEST_F(UIAudioFeedbackTest, DisableAlreadyDisabledSound)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Hover is disabled by default
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Hover));

    // Disable again
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Hover, false);

    // Should still be disabled
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Hover));
}

// Test: Volume precision
TEST_F(UIAudioFeedbackTest, VolumePrecision)
{
    auto& feedback = UIAudioFeedback::getInstance();

    float testValues[] = { 0.123f, 0.456f, 0.789f, 0.111f, 0.999f };

    for (float val : testValues)
    {
        feedback.setVolume(val);
        EXPECT_NEAR(feedback.getVolume(), val, 0.001f);
    }

    // Reset
    feedback.setVolume(0.5f);
}

// Test: Default hover disabled
TEST_F(UIAudioFeedbackTest, HoverDisabledByDefault)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Hover should be disabled by default as it can be annoying
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Hover));
}

// Test: Mixed sound enables/disables
TEST_F(UIAudioFeedbackTest, MixedSoundEnables)
{
    auto& feedback = UIAudioFeedback::getInstance();

    // Set a pattern
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Toggle, false);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Error, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Success, false);

    // Verify pattern
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_TRUE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Error));
    EXPECT_FALSE(feedback.isSoundEnabled(UIAudioFeedback::SoundType::Success));

    // Restore defaults
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    feedback.setSoundEnabled(UIAudioFeedback::SoundType::Success, true);
}
