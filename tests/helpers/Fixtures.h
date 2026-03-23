/*
    Oscil - Common Test Fixtures
    Reusable test fixture base classes for reducing setup boilerplate
*/

#pragma once

#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Source.h"
#include "core/dsp/TimingEngine.h"

#include "AudioBufferBuilder.h"
#include "OscillatorBuilder.h"
#include "SourceBuilder.h"
#include "StateBuilder.h"
#include "TestSignals.h"
#include "TimingConfigBuilder.h"

#include <gtest/gtest.h>
#include <memory>

namespace oscil::test
{

/**
 * Base fixture for DSP-related tests
 * Provides a TimingEngine instance with default configuration
 */
class DSPTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset to default state
        EngineTimingConfig defaultConfig;
        engine.setConfig(defaultConfig);
        engine.setSampleRate(44100.0);
    }

    void TearDown() override
    {
        // Cleanup if needed
    }

    /**
     * Create a default audio buffer for testing
     */
    juce::AudioBuffer<float> createTestBuffer(int numSamples = 512)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withSilence().build();
    }

    /**
     * Create a buffer with a sine wave
     */
    juce::AudioBuffer<float> createSineBuffer(int numSamples, float frequency, float amplitude = 1.0f)
    {
        return AudioBufferBuilder()
            .withChannels(2)
            .withSamples(numSamples)
            .withSineWave(frequency, amplitude, 44100.0f)
            .build();
    }

    TimingEngine engine;
};

/**
 * Base fixture for Source-related tests
 * Provides Source, SourceId, and InstanceId objects
 */
class SourceTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sourceId = SourceId::generate();
        instanceId = InstanceId::generate();
        source = std::make_unique<Source>(sourceId);
    }

    void TearDown() override { source.reset(); }

    /**
     * Create an additional source with auto-generated ID
     */
    std::unique_ptr<Source> createSource() { return SourceBuilder().buildUnique(); }

    /**
     * Create a source with specific state
     */
    std::unique_ptr<Source> createSourceWithState(SourceState state)
    {
        return SourceBuilder().withState(state).buildUnique();
    }

    /**
     * Create a source with owner
     */
    std::unique_ptr<Source> createSourceWithOwner(const InstanceId& owner)
    {
        return SourceBuilder().withOwner(owner).withState(SourceState::ACTIVE).buildUnique();
    }

    SourceId sourceId;
    InstanceId instanceId;
    std::unique_ptr<Source> source;
};

/**
 * Base fixture for Oscillator-related tests
 * Provides Oscillator instances
 */
class OscillatorTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sourceId = SourceId::generate();
        oscillator = std::make_unique<Oscillator>();
    }

    void TearDown() override { oscillator.reset(); }

    /**
     * Create an additional oscillator with auto-generated ID
     */
    std::unique_ptr<Oscillator> createOscillator() { return OscillatorBuilder().buildUnique(); }

    /**
     * Create an oscillator with specific processing mode
     */
    std::unique_ptr<Oscillator> createOscillatorWithMode(ProcessingMode mode)
    {
        return OscillatorBuilder().withProcessingMode(mode).buildUnique();
    }

    /**
     * Create an oscillator attached to a source
     */
    std::unique_ptr<Oscillator> createOscillatorWithSource(const SourceId& sid)
    {
        return OscillatorBuilder().withSourceId(sid).buildUnique();
    }

    SourceId sourceId;
    std::unique_ptr<Oscillator> oscillator;
};

/**
 * Base fixture for State persistence tests
 * Provides OscilState instances
 */
class StateTestFixture : public ::testing::Test
{
protected:
    void SetUp() override { state = std::make_unique<OscilState>(); }

    void TearDown() override { state.reset(); }

    /**
     * Create a state with oscillators
     */
    std::unique_ptr<OscilState> createStateWithOscillators(int count)
    {
        auto builder = StateBuilder();

        for (int i = 0; i < count; ++i)
        {
            auto osc = OscillatorBuilder()
                           .withName(juce::String("Oscillator ") + juce::String(i + 1))
                           .withOrderIndex(i)
                           .build();
            builder.withOscillator(osc);
        }

        return builder.buildUnique();
    }

    /**
     * Serialize and deserialize state (round-trip test helper)
     */
    std::unique_ptr<OscilState> roundTripState(OscilState& original)
    {
        juce::String xml = original.toXmlString();
        auto restored = std::make_unique<OscilState>();
        (void) restored->fromXmlString(xml);
        return restored;
    }

    std::unique_ptr<OscilState> state;
};

/**
 * Base fixture for tests requiring multiple components
 * Combines Source, Oscillator, and State management
 */
class IntegrationTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        state = std::make_unique<OscilState>();
        sourceId = SourceId::generate();
        instanceId = InstanceId::generate();

        source = SourceBuilder().withId(sourceId).withOwner(instanceId).withState(SourceState::ACTIVE).buildUnique();

        oscillator = OscillatorBuilder().withSourceId(sourceId).withName("Test Oscillator").buildUnique();
    }

    void TearDown() override
    {
        oscillator.reset();
        source.reset();
        state.reset();
    }

    /**
     * Add the oscillator to the state
     */
    void addOscillatorToState() { state->addOscillator(*oscillator); }

    std::unique_ptr<OscilState> state;
    SourceId sourceId;
    InstanceId instanceId;
    std::unique_ptr<Source> source;
    std::unique_ptr<Oscillator> oscillator;
};

/**
 * Base fixture for timing and synchronization tests
 * Provides TimingEngine and HostTimingInfo
 */
class TimingSyncTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine.setSampleRate(44100.0);

        // Set default host info
        hostInfo.bpm = 120.0;
        hostInfo.sampleRate = 44100.0;
        hostInfo.isPlaying = false;
        hostInfo.ppqPosition = 0.0;
    }

    void TearDown() override
    {
        // Cleanup if needed
    }

    /**
     * Update engine with current host info
     */
    void updateEngineWithHostInfo()
    {
        juce::AudioPlayHead::PositionInfo posInfo;
        posInfo.setBpm(hostInfo.bpm);
        posInfo.setPpqPosition(hostInfo.ppqPosition);
        posInfo.setIsPlaying(hostInfo.isPlaying);

        engine.updateHostInfo(posInfo);
    }

    /**
     * Set BPM and update engine
     */
    void setBPM(double bpm)
    {
        hostInfo.bpm = bpm;
        updateEngineWithHostInfo();
    }

    /**
     * Set playing state and update engine
     */
    void setPlaying(bool playing)
    {
        hostInfo.isPlaying = playing;
        updateEngineWithHostInfo();
    }

    TimingEngine engine;
    HostTimingInfo hostInfo;
};

} // namespace oscil::test
