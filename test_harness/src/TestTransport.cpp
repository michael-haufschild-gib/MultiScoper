/*
    Oscil Test Harness - Transport Implementation
*/

#include "TestTransport.h"

namespace oscil::test
{

TestTransport::TestTransport() {}

void TestTransport::prepare(double sampleRate) { sampleRate_ = sampleRate; }

void TestTransport::play() { playing_.store(true); }

void TestTransport::stop() { playing_.store(false); }

void TestTransport::setBpm(double bpm) { bpm_.store(juce::jlimit(20.0, 300.0, bpm)); }

void TestTransport::setPositionSamples(int64_t samples) { positionSamples_.store(std::max(int64_t(0), samples)); }

double TestTransport::getPositionSeconds() const
{
    // Guard against division by zero when sampleRate is 0
    return sampleRate_ > 0.0 ? static_cast<double>(positionSamples_.load()) / sampleRate_ : 0.0;
}

double TestTransport::getPositionBeats() const
{
    double seconds = getPositionSeconds();
    double beatsPerSecond = bpm_.load() / 60.0;
    return seconds * beatsPerSecond;
}

void TestTransport::advancePosition(int numSamples)
{
    if (playing_.load())
    {
        positionSamples_.fetch_add(numSamples);
    }
}

double TestTransport::getSamplesPerBeat() const
{
    // Guard against division by zero when bpm is 0
    double bpm = bpm_.load();
    return bpm > 0.0 ? (60.0 / bpm) * sampleRate_ : 0.0;
}

juce::AudioPlayHead::PositionInfo TestTransport::getPositionInfo() const
{
    juce::AudioPlayHead::PositionInfo info;

    info.setBpm(bpm_.load());
    info.setTimeInSamples(positionSamples_.load());
    info.setTimeInSeconds(getPositionSeconds());
    info.setPpqPosition(getPositionBeats());
    info.setIsPlaying(playing_.load());
    info.setIsRecording(false);
    info.setIsLooping(false);

    // Time signature 4/4
    info.setTimeSignature(juce::AudioPlayHead::TimeSignature{4, 4});

    // Calculate bar position
    double beats = getPositionBeats();
    int bar = static_cast<int>(beats / 4.0) + 1;
    info.setPpqPositionOfLastBarStart((bar - 1) * 4.0);

    return info;
}

} // namespace oscil::test
