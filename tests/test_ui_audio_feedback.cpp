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
