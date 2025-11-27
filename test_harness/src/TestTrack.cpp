/*
    Oscil Test Harness - Test Track Implementation
*/

#include "TestTrack.h"
#include "ui/PluginEditor.h"

namespace oscil::test
{

TestTrack::TestTrack(int trackIndex, const juce::String& name, TestTransport& transport)
    : trackIndex_(trackIndex)
    , name_(name)
    , transport_(transport)
{
    // Create the plugin processor
    processor_ = std::make_unique<OscilPluginProcessor>();
    processor_->setPlayHead(this);

    // The processor automatically registers itself in its constructor,
    // so we retrieve the sourceId from it
    sourceId_ = processor_->getSourceId();
}

TestTrack::~TestTrack()
{
    hideEditor();

    // The processor automatically unregisters itself in its destructor
    processor_.reset();
}

void TestTrack::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;

    // Prepare audio generator
    audioGenerator_.prepare(sampleRate);

    // Prepare audio buffer
    audioBuffer_.setSize(2, samplesPerBlock);

    // Prepare plugin processor
    processor_->prepareToPlay(sampleRate, samplesPerBlock);
}

void TestTrack::processBlock()
{
    // Clear and generate test audio
    audioBuffer_.clear();
    audioGenerator_.generateBlock(audioBuffer_);

    // Process through plugin
    processor_->processBlock(audioBuffer_, midiBuffer_);

    // Advance transport
    transport_.advancePosition(audioBuffer_.getNumSamples());
}

void TestTrack::showEditor()
{
    if (editor_ != nullptr)
        return;

    // Create editor
    editor_.reset(processor_->createEditor());

    if (editor_ != nullptr)
    {
        // Create window to host the editor
        editorWindow_ = std::make_unique<juce::DocumentWindow>(
            name_ + " - Oscil",
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons);

        editorWindow_->setUsingNativeTitleBar(true);
        editorWindow_->setContentOwned(editor_.release(), true);
        editorWindow_->setResizable(true, false);
        editorWindow_->centreWithSize(
            editorWindow_->getWidth(),
            editorWindow_->getHeight());
        editorWindow_->setVisible(true);

        // Get editor back from window
        editor_.reset(static_cast<juce::AudioProcessorEditor*>(
            editorWindow_->getContentComponent()));
    }
}

void TestTrack::hideEditor()
{
    if (editorWindow_ != nullptr)
    {
        editorWindow_->setVisible(false);
        editor_.release(); // Window owns it
        editorWindow_.reset();
    }
    editor_.reset();
}

bool TestTrack::isEditorVisible() const
{
    return editorWindow_ != nullptr && editorWindow_->isVisible();
}

juce::Optional<juce::AudioPlayHead::PositionInfo> TestTrack::getPosition() const
{
    return transport_.getPositionInfo();
}

} // namespace oscil::test
