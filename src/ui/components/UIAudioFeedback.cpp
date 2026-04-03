/*
    Oscil - UI Audio Feedback Implementation
*/

#include "ui/components/UIAudioFeedback.h"

#include <atomic>
#include <cmath>

namespace oscil
{

// Simple sound player using AudioSourcePlayer
// Uses juce::SpinLock with tryLock in audio callback to avoid blocking
class UIAudioFeedback::SoundPlayer : public juce::AudioSource
{
public:
    SoundPlayer() = default;

    void playBuffer(const juce::AudioBuffer<float>& buffer, float volume)
    {
        const juce::SpinLock::ScopedLockType lock(mutex_);

        // Copy buffer for playback
        playbackBuffer_.makeCopyOf(buffer);
        playbackPosition_ = 0;
        volume_ = volume;
        isPlaying_.store(true, std::memory_order_release);
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        (void) samplesPerBlockExpected;
        (void) sampleRate;
    }

    void releaseResources() override
    {
        const juce::SpinLock::ScopedLockType lock(mutex_);
        isPlaying_.store(false, std::memory_order_release);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        // Use tryLock to avoid blocking the audio thread - drop frame if locked
        const juce::SpinLock::ScopedTryLockType lock(mutex_);

        bufferToFill.clearActiveBufferRegion();

        if (!lock.isLocked())
            return; // Drop this frame rather than block audio thread

        if (!isPlaying_.load(std::memory_order_acquire) || playbackBuffer_.getNumSamples() == 0)
            return;

        // Guard against empty playback buffer (would cause negative array index)
        if (playbackBuffer_.getNumChannels() == 0)
        {
            isPlaying_.store(false, std::memory_order_release);
            return;
        }

        int const samplesToPlay =
            std::min(bufferToFill.numSamples, playbackBuffer_.getNumSamples() - playbackPosition_);

        if (samplesToPlay <= 0)
        {
            isPlaying_.store(false, std::memory_order_release);
            return;
        }

        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            int const sourceChannel = std::min(channel, playbackBuffer_.getNumChannels() - 1);

            bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample, playbackBuffer_, sourceChannel,
                                          playbackPosition_, samplesToPlay);

            bufferToFill.buffer->applyGain(channel, bufferToFill.startSample, samplesToPlay, volume_);
        }

        playbackPosition_ += samplesToPlay;

        if (playbackPosition_ >= playbackBuffer_.getNumSamples())
            isPlaying_.store(false, std::memory_order_release);
    }

private:
    juce::AudioBuffer<float> playbackBuffer_;
    int playbackPosition_ = 0;
    float volume_ = 1.0f;
    std::atomic<bool> isPlaying_{false};
    juce::SpinLock mutex_;
};

UIAudioFeedback::UIAudioFeedback() : player_(std::make_unique<SoundPlayer>())
{
    // Initialize all sound types as enabled
    soundEnabled_[SoundType::Click] = true;
    soundEnabled_[SoundType::Toggle] = true;
    soundEnabled_[SoundType::SliderSnap] = true;
    soundEnabled_[SoundType::Error] = true;
    soundEnabled_[SoundType::Success] = true;
    soundEnabled_[SoundType::Focus] = false; // Disabled by default
    soundEnabled_[SoundType::Hover] = false; // Disabled by default
    soundEnabled_[SoundType::Notification] = true;

    generateSounds();
}

UIAudioFeedback::~UIAudioFeedback() { shutdown(); }

void UIAudioFeedback::setEnabled(bool enabled)
{
    std::scoped_lock const lock(mutex_);
    enabled_ = enabled;
}

void UIAudioFeedback::setVolume(float volume)
{
    std::scoped_lock const lock(mutex_);
    volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void UIAudioFeedback::playSound(SoundType type)
{
    std::scoped_lock const lock(mutex_);

    if (!enabled_ || !soundEnabled_[type])
        return;

    auto it = sounds_.find(type);
    if (it != sounds_.end() && player_)
    {
        player_->playBuffer(it->second, volume_);
    }
}

void UIAudioFeedback::setSoundEnabled(SoundType type, bool enabled)
{
    std::scoped_lock const lock(mutex_);
    soundEnabled_[type] = enabled;
}

bool UIAudioFeedback::isSoundEnabled(SoundType type) const
{
    auto it = soundEnabled_.find(type);
    return it != soundEnabled_.end() ? it->second : false;
}

void UIAudioFeedback::setAudioDevice(juce::AudioDeviceManager* deviceManager)
{
    std::scoped_lock const lock(mutex_);
    deviceManager_ = deviceManager;

    if (deviceManager_ && deviceManager_->getCurrentAudioDevice())
    {
        sampleRate_ = static_cast<float>(deviceManager_->getCurrentAudioDevice()->getCurrentSampleRate());
        generateSounds(); // Regenerate at correct sample rate
    }
}

void UIAudioFeedback::shutdown()
{
    std::scoped_lock const lock(mutex_);
    player_.reset();
    sounds_.clear();
}

void UIAudioFeedback::generateSounds()
{
    sounds_[SoundType::Click] = generateClickSound();
    sounds_[SoundType::Toggle] = generateToggleSound();
    sounds_[SoundType::SliderSnap] = generateSliderSnapSound();
    sounds_[SoundType::Error] = generateErrorSound();
    sounds_[SoundType::Success] = generateSuccessSound();
    sounds_[SoundType::Focus] = generateFocusSound();
    sounds_[SoundType::Hover] = generateHoverSound();
    sounds_[SoundType::Notification] = generateNotificationSound();
}

juce::AudioBuffer<float> UIAudioFeedback::generateClickSound() const
{
    // Short percussive click - 20ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.02f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    // Noise burst with quick decay
    juce::Random random;
    for (int i = 0; i < numSamples; ++i)
    {
        float const noise = (random.nextFloat() * 2.0f) - 1.0f;
        float const envelope = std::exp(-static_cast<float>(i) / (static_cast<float>(numSamples) * 0.1f));
        data[i] = noise * envelope * 0.3f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateToggleSound() const
{
    // Soft "pop" sound - 50ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.05f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    float const freq = 880.0f; // A5
    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const envelope = std::exp(-t * 60.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.2f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateSliderSnapSound() const
{
    // Short tick - 10ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.01f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    float const freq = 1200.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const envelope = std::exp(-t * 200.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.15f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateErrorSound() const
{
    // Two-tone descending - 150ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.15f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    int const halfSamples = numSamples / 2;
    float const freq1 = 400.0f;
    float const freq2 = 300.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const freq = (i < halfSamples) ? freq1 : freq2;
        float const localT = (i < halfSamples) ? t : (t - (static_cast<float>(halfSamples) / sampleRate_));
        float const envelope = std::exp(-localT * 15.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.25f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateSuccessSound() const
{
    // Two-tone ascending - 150ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.15f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    int const halfSamples = numSamples / 2;
    float const freq1 = 523.0f; // C5
    float const freq2 = 659.0f; // E5

    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const freq = (i < halfSamples) ? freq1 : freq2;
        float const envelope = std::exp(-(static_cast<float>(i % halfSamples) / sampleRate_) * 15.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.2f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateFocusSound() const
{
    // Very soft tick - 5ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.005f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    float const freq = 2000.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const envelope = std::exp(-t * 400.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.1f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateHoverSound() const
{
    // Barely audible - 3ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.003f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    float const freq = 3000.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const envelope = std::exp(-t * 500.0f);
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t) * envelope * 0.05f;
    }

    return buffer;
}

juce::AudioBuffer<float> UIAudioFeedback::generateNotificationSound() const
{
    // Gentle chime - 300ms
    int const numSamples = static_cast<int>(sampleRate_ * 0.3f);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto* data = buffer.getWritePointer(0);

    // C major chord
    float const freqs[] = {523.0f, 659.0f, 784.0f}; // C5, E5, G5

    for (int i = 0; i < numSamples; ++i)
    {
        float const t = static_cast<float>(i) / sampleRate_;
        float const envelope = std::exp(-t * 5.0f);

        float sample = 0.0f;
        for (float const freq : freqs)
        {
            sample += std::sin(2.0f * juce::MathConstants<float>::pi * freq * t);
        }

        data[i] = sample / 3.0f * envelope * 0.15f;
    }

    return buffer;
}

void UIAudioFeedback::applyEnvelope(juce::AudioBuffer<float>& buffer, float attackMs, float releaseMs) const
{
    int const numSamples = buffer.getNumSamples();
    int const attackSamples = static_cast<int>((attackMs / 1000.0f) * sampleRate_);
    int const releaseSamples = static_cast<int>((releaseMs / 1000.0f) * sampleRate_);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float envelope = 1.0f;

            if (i < attackSamples)
                envelope = static_cast<float>(i) / static_cast<float>(attackSamples);
            else if (i > numSamples - releaseSamples)
                envelope = static_cast<float>(numSamples - i) / static_cast<float>(releaseSamples);

            data[i] *= envelope;
        }
    }
}

} // namespace oscil
