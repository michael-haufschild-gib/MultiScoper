#include <gtest/gtest.h>
#include "core/PerformanceConfig.h"
#include "core/AudioConfig.h"
#include "core/HostInfo.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_processors/juce_audio_processors.h>

using namespace oscil;

// --- PerformanceConfig Tests ---

TEST(PerformanceConfigTest, Clamping) {
    PerformanceConfig config;
    
    juce::ValueTree vt("Test");
    vt.setProperty("refreshRate", 10.0f, nullptr); // Too low (min 15)
    vt.setProperty("cpuWarningThreshold", 100.0f, nullptr); // Too high (max 20)
    
    config.fromValueTree(vt);
    
    // Should be clamped
    EXPECT_FLOAT_EQ(config.refreshRate, 15.0f);
    EXPECT_FLOAT_EQ(config.cpuWarningThreshold, 20.0f);
}

TEST(PerformanceConfigTest, QualityHeuristics) {
    PerformanceConfig config;
    config.qualityMode = QualityMode::HIGHEST;
    
    EXPECT_EQ(config.suggestQualityForCpu(5.0f), QualityMode::HIGHEST);
    EXPECT_EQ(config.suggestQualityForCpu(15.0f), QualityMode::MAXIMUM); // Critical
    EXPECT_EQ(config.suggestQualityForCpu(10.0f), QualityMode::HIGH);    // Warning
}

TEST(PerformanceConfigTest, Serialization) {
    PerformanceConfig original;
    original.refreshRate = 120.0f;
    original.qualityMode = QualityMode::HIGHEST;
    original.statusBarVisible = false;
    
    juce::ValueTree vt = original.toValueTree();
    
    PerformanceConfig restored;
    restored.fromValueTree(vt);
    
    EXPECT_FLOAT_EQ(restored.refreshRate, original.refreshRate);
    EXPECT_EQ(restored.qualityMode, original.qualityMode);
    EXPECT_EQ(restored.statusBarVisible, original.statusBarVisible);
}

// --- AudioConfig Tests ---

TEST(AudioConfigTest, Validation) {
    AudioConfig config;
    config.sampleRate = 44100.0f;
    config.bufferSize = 512;
    config.channelCount = 2;
    EXPECT_TRUE(config.isValid());
    
    config.sampleRate = 100.0f; // Invalid
    EXPECT_FALSE(config.isValidSampleRate());
    EXPECT_FALSE(config.isValid());
    
    config.sampleRate = 44100.0f;
    config.bufferSize = 10; // Invalid
    EXPECT_FALSE(config.isValidBufferSize());
    
    config.bufferSize = 512;
    config.channelCount = 0; // Invalid
    EXPECT_FALSE(config.isValidChannelCount());
}

TEST(AudioConfigTest, Helpers) {
    AudioConfig config;
    config.sampleRate = 1000.0f; // Simple math
    config.bufferSize = 100;
    
    EXPECT_DOUBLE_EQ(config.getBufferDurationMs(), 100.0); // 100 samples @ 1000Hz = 0.1s = 100ms
    EXPECT_DOUBLE_EQ(config.getBufferDurationSeconds(), 0.1);
    
    EXPECT_EQ(config.msToSamples(100.0), 100);
    EXPECT_DOUBLE_EQ(config.samplesToMs(100), 100.0);
}

// --- HostInfo Tests ---

TEST(HostInfoTest, BPMHelpers) {
    HostInfo host;
    host.bpm = 120.0f;
    
    // 120 BPM = 2 beats per sec = 500ms per beat
    EXPECT_DOUBLE_EQ(host.getMsPerBeat(), 500.0);
    
    host.timeSignature.numerator = 4;
    // 4 beats per bar -> 2000ms per bar
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0);
}

TEST(HostInfoTest, PositionHelpers) {
    HostInfo host;
    host.timeSignature.numerator = 4;
    host.ppqPosition = 4.5; // Bar 1 (start at 0), Beat 0.5
    
    EXPECT_DOUBLE_EQ(host.getBarPosition(), 1.125); // 4.5 / 4
    EXPECT_DOUBLE_EQ(host.getBeatInBar(), 0.5); // 4.5 % 4
}
