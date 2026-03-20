/*
    Oscil - UI Audio Feedback Tests
*/

#include <gtest/gtest.h>
#include "ui/components/UIAudioFeedback.h"

using namespace oscil;

class UIAudioFeedbackTest : public ::testing::Test
{
protected:
    std::unique_ptr<UIAudioFeedback> feedback_;

    void SetUp() override
    {
        feedback_ = std::make_unique<UIAudioFeedback>();
    }

    void TearDown() override
    {
        feedback_.reset();
    }
};

// Test: Default state - disabled
TEST_F(UIAudioFeedbackTest, DefaultDisabled)
{
    // Audio feedback is disabled by default
    EXPECT_FALSE(feedback_->isEnabled());
}

// Test: Enable/disable
TEST_F(UIAudioFeedbackTest, EnableDisable)
{
    feedback_->setEnabled(true);
    EXPECT_TRUE(feedback_->isEnabled());

    feedback_->setEnabled(false);
    EXPECT_FALSE(feedback_->isEnabled());
}

// Test: Volume control
TEST_F(UIAudioFeedbackTest, VolumeControl)
{
    feedback_->setVolume(0.5f);
    EXPECT_NEAR(feedback_->getVolume(), 0.5f, 0.01f);

    feedback_->setVolume(0.8f);
    EXPECT_NEAR(feedback_->getVolume(), 0.8f, 0.01f);
}

// Test: Volume clamping
TEST_F(UIAudioFeedbackTest, VolumeClamping)
{
    feedback_->setVolume(-0.5f);
    EXPECT_GE(feedback_->getVolume(), 0.0f);

    feedback_->setVolume(1.5f);
    EXPECT_LE(feedback_->getVolume(), 1.0f);

    // Reset
    feedback_->setVolume(0.5f);
}

// Test: Individual sound types enabled
TEST_F(UIAudioFeedbackTest, IndividualSoundEnabled)
{
    // Check default states
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));  // Disabled by default
}

// Test: Enable/disable individual sounds
TEST_F(UIAudioFeedbackTest, EnableDisableIndividualSounds)
{
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));

    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
}

// Test: Play sound doesn't crash when disabled
TEST_F(UIAudioFeedbackTest, PlaySoundWhenDisabled)
{
    feedback_->setEnabled(false);

    feedback_->playSound(UIAudioFeedback::SoundType::Click);
    feedback_->playSound(UIAudioFeedback::SoundType::Toggle);
    feedback_->playSound(UIAudioFeedback::SoundType::Error);

    // State should remain disabled after playing sounds
    EXPECT_FALSE(feedback_->isEnabled());
}

// Test: Play sound doesn't crash when enabled
TEST_F(UIAudioFeedbackTest, PlaySoundWhenEnabled)
{
    feedback_->setEnabled(true);

    feedback_->playSound(UIAudioFeedback::SoundType::Click);
    feedback_->playSound(UIAudioFeedback::SoundType::Toggle);
    feedback_->playSound(UIAudioFeedback::SoundType::Success);
    feedback_->playSound(UIAudioFeedback::SoundType::Error);
    feedback_->playSound(UIAudioFeedback::SoundType::SliderSnap);
    feedback_->playSound(UIAudioFeedback::SoundType::Focus);
    feedback_->playSound(UIAudioFeedback::SoundType::Notification);

    // System should remain enabled and functional after playing all types
    EXPECT_TRUE(feedback_->isEnabled());
    feedback_->setEnabled(false);
}

// Test: All sound types exist
TEST_F(UIAudioFeedbackTest, AllSoundTypesExist)
{
    // These should all be valid sound types
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::SliderSnap, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Error, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Success, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Focus, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Hover, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Notification, true);

    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::SliderSnap));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Error));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Success));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Focus));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Notification));
}

// Test: Disabled individual sound not played
TEST_F(UIAudioFeedbackTest, DisabledSoundNotPlayed)
{
    feedback_->setEnabled(true);

    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));

    feedback_->playSound(UIAudioFeedback::SoundType::Click);

    // Sound type should still be disabled after playback attempt
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));

    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback_->setEnabled(false);
}

// Test: Set audio device manager (null is valid)
TEST_F(UIAudioFeedbackTest, SetAudioDeviceManager)
{
    feedback_->setAudioDevice(nullptr);

    // After setting null device, system should still be in a consistent state
    EXPECT_FALSE(feedback_->isEnabled());
    EXPECT_FLOAT_EQ(feedback_->getVolume(), 0.5f);
}

// Test: Shutdown doesn't crash
TEST_F(UIAudioFeedbackTest, ShutdownDoesntCrash)
{
    // Call shutdown explicitly
    feedback_->shutdown();

    // If we get here without crashing, test passes
    EXPECT_TRUE(true);
}

// ============================================================================
// Behavioral Edge Case Tests
// ============================================================================

// Test: Rapid toggle on/off
TEST_F(UIAudioFeedbackTest, RapidToggle)
{
    // Rapidly toggle many times
    for (int i = 0; i < 100; ++i)
    {
        feedback_->setEnabled(true);
        feedback_->setEnabled(false);
    }

    // Should end disabled
    EXPECT_FALSE(feedback_->isEnabled());

    // Toggle again and verify state
    feedback_->setEnabled(true);
    EXPECT_TRUE(feedback_->isEnabled());
    feedback_->setEnabled(false);
}

// Test: Rapid playSound calls (stress test)
TEST_F(UIAudioFeedbackTest, RapidPlaySoundCalls)
{
    feedback_->setEnabled(true);

    for (int i = 0; i < 100; ++i)
    {
        feedback_->playSound(UIAudioFeedback::SoundType::Click);
        feedback_->playSound(UIAudioFeedback::SoundType::Toggle);
        feedback_->playSound(UIAudioFeedback::SoundType::SliderSnap);
    }

    // After 300 rapid calls, system should still report enabled and valid volume
    EXPECT_TRUE(feedback_->isEnabled());
    EXPECT_GE(feedback_->getVolume(), 0.0f);
    feedback_->setEnabled(false);
}

// Test: Volume at exact boundaries
TEST_F(UIAudioFeedbackTest, VolumeExactBoundaries)
{
    feedback_->setVolume(0.0f);
    EXPECT_FLOAT_EQ(feedback_->getVolume(), 0.0f);

    feedback_->setVolume(1.0f);
    EXPECT_FLOAT_EQ(feedback_->getVolume(), 1.0f);

    // Reset
    feedback_->setVolume(0.5f);
}

// Test: Volume with very small values
TEST_F(UIAudioFeedbackTest, VolumeVerySmall)
{
    feedback_->setVolume(0.001f);
    EXPECT_NEAR(feedback_->getVolume(), 0.001f, 0.0001f);

    feedback_->setVolume(1e-10f);
    EXPECT_GE(feedback_->getVolume(), 0.0f);

    // Reset
    feedback_->setVolume(0.5f);
}

// Test: Multiple sound types toggled quickly
TEST_F(UIAudioFeedbackTest, RapidSoundTypeToggle)
{
    for (int i = 0; i < 50; ++i)
    {
        feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
        feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Toggle, false);
        feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
        feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    }

    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
}

// Test: Play sound while toggling enabled
TEST_F(UIAudioFeedbackTest, PlayWhileToggling)
{
    for (int i = 0; i < 50; ++i)
    {
        feedback_->setEnabled(true);
        feedback_->playSound(UIAudioFeedback::SoundType::Click);
        feedback_->setEnabled(false);
        feedback_->playSound(UIAudioFeedback::SoundType::Click);
    }

    // Should not crash
    EXPECT_FALSE(feedback_->isEnabled());
}

// Test: All sound types can be played
TEST_F(UIAudioFeedbackTest, PlayAllSoundTypes)
{
    feedback_->setEnabled(true);

    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Hover, true);

    feedback_->playSound(UIAudioFeedback::SoundType::Click);
    feedback_->playSound(UIAudioFeedback::SoundType::Toggle);
    feedback_->playSound(UIAudioFeedback::SoundType::SliderSnap);
    feedback_->playSound(UIAudioFeedback::SoundType::Error);
    feedback_->playSound(UIAudioFeedback::SoundType::Success);
    feedback_->playSound(UIAudioFeedback::SoundType::Focus);
    feedback_->playSound(UIAudioFeedback::SoundType::Hover);
    feedback_->playSound(UIAudioFeedback::SoundType::Notification);

    // All sound types should still be enabled after playback
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Hover, false);
    feedback_->setEnabled(false);
}

// Test: Volume change while playing
TEST_F(UIAudioFeedbackTest, VolumeChangeDuringPlayback)
{
    feedback_->setEnabled(true);

    feedback_->setVolume(1.0f);
    feedback_->playSound(UIAudioFeedback::SoundType::Click);
    feedback_->setVolume(0.0f);
    feedback_->playSound(UIAudioFeedback::SoundType::Click);
    feedback_->setVolume(0.5f);
    feedback_->playSound(UIAudioFeedback::SoundType::Click);

    // Volume should reflect the last set value
    EXPECT_FLOAT_EQ(feedback_->getVolume(), 0.5f);
    feedback_->setEnabled(false);
}

// Test: Disable specific sound then play it
TEST_F(UIAudioFeedbackTest, PlayDisabledSound)
{
    feedback_->setEnabled(true);

    // Disable Click sound
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, false);
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));

    // Play it anyway (should be silent, not crash)
    feedback_->playSound(UIAudioFeedback::SoundType::Click);

    // Re-enable
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback_->setEnabled(false);
}

// Test: Set audio device to null multiple times
TEST_F(UIAudioFeedbackTest, SetNullDeviceMultipleTimes)
{
    for (int i = 0; i < 10; ++i)
    {
        feedback_->setAudioDevice(nullptr);
    }

    // System should remain in a consistent state
    EXPECT_GE(feedback_->getVolume(), 0.0f);
    EXPECT_LE(feedback_->getVolume(), 1.0f);
}

// Test: State after many operations
TEST_F(UIAudioFeedbackTest, StateAfterManyOperations)
{
    // Start with known state
    feedback_->setEnabled(false);
    feedback_->setVolume(0.5f);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);

    // Perform many operations
    for (int i = 0; i < 100; ++i)
    {
        feedback_->setEnabled(i % 2 == 0);
        feedback_->setVolume(static_cast<float>(i % 10) / 10.0f);
        feedback_->playSound(UIAudioFeedback::SoundType::Click);
    }

    // Final state should be predictable
    EXPECT_FALSE(feedback_->isEnabled()); // 99 % 2 == 1, so false
    EXPECT_NEAR(feedback_->getVolume(), 0.9f, 0.01f); // 99 % 10 = 9, so 0.9
}

// Test: Enable sound type that's already enabled
TEST_F(UIAudioFeedbackTest, EnableAlreadyEnabledSound)
{
    // Click is enabled by default
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));

    // Enable again
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);

    // Should still be enabled
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
}

// Test: Disable sound type that's already disabled
TEST_F(UIAudioFeedbackTest, DisableAlreadyDisabledSound)
{
    // Hover is disabled by default
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));

    // Disable again
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Hover, false);

    // Should still be disabled
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));
}

// Test: Volume precision
TEST_F(UIAudioFeedbackTest, VolumePrecision)
{
    float testValues[] = { 0.123f, 0.456f, 0.789f, 0.111f, 0.999f };

    for (float val : testValues)
    {
        feedback_->setVolume(val);
        EXPECT_NEAR(feedback_->getVolume(), val, 0.001f);
    }

    // Reset
    feedback_->setVolume(0.5f);
}

// Test: Default hover disabled
TEST_F(UIAudioFeedbackTest, HoverDisabledByDefault)
{
    // Hover should be disabled by default as it can be annoying
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Hover));
}

// Test: Mixed sound enables/disables
TEST_F(UIAudioFeedbackTest, MixedSoundEnables)
{
    // Set a pattern
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Click, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Toggle, false);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Error, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Success, false);

    // Verify pattern
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Click));
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Toggle));
    EXPECT_TRUE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Error));
    EXPECT_FALSE(feedback_->isSoundEnabled(UIAudioFeedback::SoundType::Success));

    // Restore defaults
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Toggle, true);
    feedback_->setSoundEnabled(UIAudioFeedback::SoundType::Success, true);
}

// Test: Multiple instances are independent
TEST_F(UIAudioFeedbackTest, MultipleInstancesIndependent)
{
    auto feedback2 = std::make_unique<UIAudioFeedback>();

    feedback_->setEnabled(true);
    feedback_->setVolume(0.75f);

    // Second instance should have its own state
    EXPECT_FALSE(feedback2->isEnabled());
    EXPECT_FLOAT_EQ(feedback2->getVolume(), 0.5f);  // Default volume

    // Changes to one shouldn't affect the other
    feedback2->setEnabled(true);
    feedback2->setVolume(0.25f);

    EXPECT_TRUE(feedback_->isEnabled());
    EXPECT_NEAR(feedback_->getVolume(), 0.75f, 0.01f);
}
