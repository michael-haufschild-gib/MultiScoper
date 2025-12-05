/*
    Oscil - UI Audio Feedback
    Optional sound effects for UI interactions
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>
#include <map>

namespace oscil
{

/**
 * UI Audio Feedback System
 *
 * Provides optional audio feedback for UI interactions.
 * Features:
 * - Multiple sound types (click, toggle, error, success)
 * - Volume control
 * - Enable/disable toggle
 * - Minimal CPU footprint
 * - Thread-safe playback
 */
class UIAudioFeedback
{
public:
    enum class SoundType
    {
        Click,          // Button clicks
        Toggle,         // Toggle/checkbox changes
        SliderSnap,     // Magnetic snap on sliders
        Error,          // Validation errors
        Success,        // Successful actions
        Focus,          // Focus changes
        Hover,          // Hover effects (usually disabled)
        Notification    // Alerts/notifications
    };

    UIAudioFeedback();
    ~UIAudioFeedback();

    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Volume control (0.0 - 1.0)
    void setVolume(float volume);
    float getVolume() const { return volume_; }

    // Play sounds
    void playSound(SoundType type);

    // Individual sound enables
    void setSoundEnabled(SoundType type, bool enabled);
    bool isSoundEnabled(SoundType type) const;

    // Audio device setup
    void setAudioDevice(juce::AudioDeviceManager* deviceManager);

    // Cleanup
    void shutdown();

    UIAudioFeedback(const UIAudioFeedback&) = delete;
    UIAudioFeedback& operator=(const UIAudioFeedback&) = delete;

private:

    // Generate synthesized sounds
    void generateSounds();
    juce::AudioBuffer<float> generateClickSound();
    juce::AudioBuffer<float> generateToggleSound();
    juce::AudioBuffer<float> generateSliderSnapSound();
    juce::AudioBuffer<float> generateErrorSound();
    juce::AudioBuffer<float> generateSuccessSound();
    juce::AudioBuffer<float> generateFocusSound();
    juce::AudioBuffer<float> generateHoverSound();
    juce::AudioBuffer<float> generateNotificationSound();

    // Apply envelope
    void applyEnvelope(juce::AudioBuffer<float>& buffer, float attackMs, float releaseMs);

    // Internal playback
    class SoundPlayer;
    std::unique_ptr<SoundPlayer> player_;

    // Sound buffers
    std::map<SoundType, juce::AudioBuffer<float>> sounds_;
    std::map<SoundType, bool> soundEnabled_;

    // State
    bool enabled_ = false;  // Disabled by default
    float volume_ = 0.5f;
    float sampleRate_ = 44100.0f;

    juce::AudioDeviceManager* deviceManager_ = nullptr;

    std::mutex mutex_;
};

} // namespace oscil
