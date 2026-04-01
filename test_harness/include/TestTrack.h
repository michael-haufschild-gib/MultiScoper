/*
    Oscil Test Harness - Test Track
    Represents a single DAW track with plugin instance
*/

#pragma once

#include "core/InstanceRegistry.h"

#include "TestAudioGenerator.h"
#include "TestTransport.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace oscil::test
{

/**
 * Represents a single DAW track with:
 * - OscilPluginProcessor instance
 * - Audio generator for test signals
 * - Unique source ID registered with InstanceRegistry
 */
class TestTrack : public juce::AudioPlayHead
{
public:
    /**
     * Create a test track with given name
     */
    TestTrack(int trackIndex, const juce::String& name, TestTransport& transport);
    ~TestTrack() override;

    /**
     * Prepare for playback
     */
    void prepare(double sampleRate, int samplesPerBlock);

    /**
     * Process a block of audio through the plugin
     */
    void processBlock();

    /**
     * Get the plugin processor
     */
    OscilPluginProcessor& getProcessor() { return *processor_; }
    const OscilPluginProcessor& getProcessor() const { return *processor_; }

    /**
     * Get the audio generator
     */
    TestAudioGenerator& getAudioGenerator() { return audioGenerator_; }

    /**
     * Get the source ID for this track
     */
    const SourceId& getSourceId() const { return sourceId_; }

    /**
     * Get track name
     */
    const juce::String& getName() const { return name_; }

    /**
     * Get track index
     */
    int getTrackIndex() const { return trackIndex_; }

    /**
     * Create and show the plugin editor window
     */
    void showEditor();

    /**
     * Hide the plugin editor window
     */
    void hideEditor();

    /**
     * Check if editor is visible
     */
    bool isEditorVisible() const;

    /**
     * Get the editor component (may be null)
     */
    juce::AudioProcessorEditor* getEditor() { return editor_.get(); }

    /**
     * Get the editor window (may be null)
     */
    juce::DocumentWindow* getEditorWindow() { return editorWindow_.get(); }

    // AudioPlayHead implementation
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override;

private:
    int trackIndex_;
    juce::String name_;
    SourceId sourceId_;
    TestTransport& transport_;

    std::unique_ptr<OscilPluginProcessor> processor_;
    std::unique_ptr<juce::AudioProcessorEditor> editor_;
    std::unique_ptr<juce::DocumentWindow> editorWindow_;

    TestAudioGenerator audioGenerator_;
    juce::AudioBuffer<float> audioBuffer_;
    juce::MidiBuffer midiBuffer_;

    double sampleRate_ = 44100.0;
    int samplesPerBlock_ = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestTrack)
};

} // namespace oscil::test
