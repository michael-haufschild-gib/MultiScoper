/*
    Oscil Test Harness - Test DAW Implementation
*/

#include "TestDAW.h"

namespace oscil::test
{

TestDAW::TestDAW() {}

TestDAW::~TestDAW()
{
    stop();
    hideAllEditors();
    // Clear in reverse order to match real DAW teardown
    for (int i = static_cast<int>(tracks_.size()) - 1; i >= 0; --i)
        tracks_[static_cast<size_t>(i)].reset();
    tracks_.clear();
}

void TestDAW::initialize(int numTracks)
{
    if (initialized_)
        return;

    // Prepare transport
    transport_.prepare(sampleRate_);

    // Create tracks
    for (int i = 0; i < numTracks; ++i)
    {
        juce::String trackName = "Track " + juce::String(i + 1);
        auto track = std::make_unique<TestTrack>(i, trackName, transport_);
        track->prepare(sampleRate_, bufferSize_);

        // Set default audio - each track gets a different LFO frequency for visual testing
        constexpr float trackFrequencies[] = {2.0f, 6.0f, 3.0f};
        float freq = (i < 3) ? trackFrequencies[i] : 0.5f;
        track->getAudioGenerator().setFrequency(freq);
        track->getAudioGenerator().setWaveform(Waveform::Sine);
        track->getAudioGenerator().setAmplitude(0.5f);

        tracks_.push_back(std::move(track));
    }

    initialized_ = true;
}

void TestDAW::start()
{
    if (!initialized_)
        initialize();

    transport_.play();
    running_ = true;
    startTimer(TIMER_INTERVAL_MS);
}

void TestDAW::stop()
{
    stopTimer();
    running_ = false;
    transport_.stop();
}

TestTrack* TestDAW::getTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

const TestTrack* TestDAW::getTrack(int index) const
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

std::vector<TestTrack*> TestDAW::getTracks()
{
    std::vector<TestTrack*> result;
    for (auto& track : tracks_)
    {
        if (track != nullptr)
            result.push_back(track.get());
    }
    return result;
}

void TestDAW::showTrackEditor(int trackIndex)
{
    if (auto* track = getTrack(trackIndex))
    {
        track->showEditor();
    }
}

void TestDAW::hideAllEditors()
{
    for (auto& track : tracks_)
    {
        if (track != nullptr)
            track->hideEditor();
    }
}

juce::AudioProcessorEditor* TestDAW::getPrimaryEditor()
{
    if (!tracks_.empty())
    {
        return tracks_[0]->getEditor();
    }
    return nullptr;
}

int TestDAW::addTrack(const juce::String& name)
{
    int index = static_cast<int>(tracks_.size());
    juce::String trackName = name.isEmpty() ? ("Track " + juce::String(index + 1)) : name;
    auto track = std::make_unique<TestTrack>(index, trackName, transport_);
    track->prepare(sampleRate_, bufferSize_);
    track->getAudioGenerator().setFrequency(440.0f);
    track->getAudioGenerator().setWaveform(Waveform::Sine);
    track->getAudioGenerator().setAmplitude(0.5f);
    tracks_.push_back(std::move(track));
    return index;
}

bool TestDAW::removeTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size()))
        return false;
    if (tracks_[static_cast<size_t>(trackIndex)] == nullptr)
        return false;

    tracks_[static_cast<size_t>(trackIndex)]->hideEditor();
    tracks_[static_cast<size_t>(trackIndex)].reset();
    return true;
}

void TestDAW::timerCallback()
{
    if (running_)
    {
        processAudioBlock();
    }
}

void TestDAW::processAudioBlock()
{
    // Process all tracks (skip null entries from removed tracks)
    for (auto& track : tracks_)
    {
        if (track != nullptr)
            track->processBlock();
    }
}

} // namespace oscil::test
