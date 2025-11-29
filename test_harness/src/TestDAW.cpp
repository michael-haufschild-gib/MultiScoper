/*
    Oscil Test Harness - Test DAW Implementation
*/

#include "TestDAW.h"

namespace oscil::test
{

TestDAW::TestDAW()
{
}

TestDAW::~TestDAW()
{
    stop();
    hideAllEditors();
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
        constexpr float trackFrequencies[] = { 0.5f, 0.125f, 3.0f };
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
        return tracks_[index].get();
    return nullptr;
}

const TestTrack* TestDAW::getTrack(int index) const
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[index].get();
    return nullptr;
}

std::vector<TestTrack*> TestDAW::getTracks()
{
    std::vector<TestTrack*> result;
    for (auto& track : tracks_)
    {
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

void TestDAW::timerCallback()
{
    if (running_)
    {
        processAudioBlock();
    }
}

void TestDAW::processAudioBlock()
{
    // Process all tracks
    for (auto& track : tracks_)
    {
        track->processBlock();
    }
}

} // namespace oscil::test
