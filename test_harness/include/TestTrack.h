/*
    MultiScoper Test Harness - Test Track
    Represents a single DAW track with plugin instance
*/

#pragma once

#include "core/InstanceRegistry.h"

#include "TestAudioGenerator.h"
#include "TestTransport.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <mutex>

namespace multiscoper::test
{

/**
 * Represents a single DAW track with:
 * - MultiScoperPluginProcessor instance
 * - Audio generator for test signals
 * - Unique source ID registered with InstanceRegistry
 */
class TestTrack : public juce::AudioPlayHead
{
public:
    /**
     * Create a test track with given name.
     *
     * If `overrideRegistry` is non-null, the plugin processor is constructed
     * with that registry injected instead of the PluginFactory singleton's
     * shared one. Used to simulate Logic AU sandbox isolation, where each
     * plugin lives in its own process and cannot see other instances'
     * sources. Ownership of the registry remains with the caller (TestDAW).
     */
    TestTrack(int trackIndex, const juce::String& name, TestTransport& transport,
              IInstanceRegistry* overrideRegistry = nullptr);
    ~TestTrack() override;

    /// Returns the `IInstanceRegistry` this track's processor was wired up
    /// with. Matches `PluginFactory::getInstance().getInstanceRegistry()`
    /// under normal operation, or a per-track registry under isolation mode.
    IInstanceRegistry& getInstanceRegistry() const;

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
    MultiScoperPluginProcessor& getProcessor() { return *processor_; }
    const MultiScoperPluginProcessor& getProcessor() const { return *processor_; }

    /**
     * Get the audio generator
     */
    TestAudioGenerator& getAudioGenerator() { return audioGenerator_; }

    /**
     * Get the source ID for this track.
     *
     * Thread-safety: callers from any thread are serialized by
     * ``sourceIdMutex_``. The processor's source ID can be queried
     * concurrently from HTTP handler threads during registry scans, so the
     * cache read-modify-write is guarded even though most callers are
     * single-threaded.
     */
    SourceId getSourceId() const
    {
        std::lock_guard<std::mutex> lock(sourceIdMutex_);
        if (!sourceId_.isValid() && processor_ != nullptr)
        {
            auto live = processor_->getSourceId();
            if (live.isValid())
                sourceId_ = live;
        }
        return sourceId_;
    }

    /**
     * Drop the cached SourceId and re-read it from the processor.
     *
     * The cache is populated once in ``prepare()`` after the first
     * registration. setStateInformation may replace the processor's
     * SourceId mid-session (via ``reRegisterUnderPersistedTrackIdentifier``
     * when a persisted trackIdentifier arrives that differs from the one
     * assigned by prepareToPlay). Callers that drive state restore must
     * invoke this so subsequent ``getSourceId()`` reads return the
     * post-restore value rather than the stale prepareToPlay value.
     */
    void refreshSourceIdFromProcessor()
    {
        std::lock_guard<std::mutex> lock(sourceIdMutex_);
        if (processor_ != nullptr)
            sourceId_ = processor_->getSourceId();
    }

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
     * Detach the editor component from its window without destroying it.
     *
     * Unlike hideEditor(), which clears the content component and resets
     * editor_, this call only removes the editor from the window — both
     * editor_ and editorWindow_ remain alive.  This reproduces the
     * parentHierarchyChanged(null-parent) transition that real DAWs
     * (Logic, Cubase) trigger when the user collapses a plugin window
     * without destroying the plugin.
     *
     * Returns true if an editor was previously attached and is now detached.
     */
    bool detachEditor();

    /**
     * Reattach a previously-detached editor to its window.
     *
     * Pairs with detachEditor(): puts the existing editor_ back into the
     * existing editorWindow_ (or returns false if either is missing).  This
     * exercises the parentHierarchyChanged(non-null-parent) path without
     * going through createEditor() again.
     *
     * Returns true if an editor was reattached.
     */
    bool reattachEditor();

    /**
     * Get the editor component (may be null)
     */
    juce::AudioProcessorEditor* getEditor() { return editor_.get(); }

    /**
     * Get the editor window (may be null)
     */
    juce::DocumentWindow* getEditorWindow() { return editorWindow_.get(); }

    /**
     * Apply a new channel layout by reconfiguring the processor's bus layout.
     *
     * ``layout`` must be "mono" or "stereo".  If the processor rejects the
     * layout (isBusesLayoutSupported returns false), the audio buffer keeps
     * its existing channel count and this method returns false — matching the
     * contract a real host observes when setBusesLayout refuses a request.
     *
     * Returns true on success, false on rejection / invalid argument.
     */
    bool setChannelLayout(const juce::String& layout);

    // AudioPlayHead implementation
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override;

private:
    int trackIndex_;
    juce::String name_;
    mutable std::mutex sourceIdMutex_;
    mutable SourceId sourceId_;
    TestTransport& transport_;

    std::unique_ptr<MultiScoperPluginProcessor> processor_;
    // Non-owning pointer to the registry the processor was constructed with.
    // Null if we used the factory default (which is the common path).
    IInstanceRegistry* overrideRegistry_{nullptr};
    std::unique_ptr<juce::AudioProcessorEditor> editor_;
    std::unique_ptr<juce::DocumentWindow> editorWindow_;

    TestAudioGenerator audioGenerator_;
    juce::AudioBuffer<float> audioBuffer_;
    juce::MidiBuffer midiBuffer_;

    double sampleRate_ = 44100.0;
    int samplesPerBlock_ = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestTrack)
};

} // namespace multiscoper::test
