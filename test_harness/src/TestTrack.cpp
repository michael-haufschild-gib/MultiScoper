/*
    Oscil Test Harness - Test Track Implementation
*/

#include "TestTrack.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace oscil::test
{

namespace
{

/**
 * DocumentWindow subclass that hides itself (and notifies the owner) when
 * the close button is pressed, and forwards minimize/maximize to the OS.
 */
class EditorWindow : public juce::DocumentWindow
{
public:
    EditorWindow(const juce::String& title, std::function<void()> onClose)
        : DocumentWindow(title, juce::Colours::darkgrey, DocumentWindow::allButtons)
        , onClose_(std::move(onClose))
    {
    }

    void closeButtonPressed() override
    {
        if (onClose_)
            onClose_();
    }

private:
    std::function<void()> onClose_;
};

} // namespace

TestTrack::TestTrack(int trackIndex, const juce::String& name, TestTransport& transport)
    : trackIndex_(trackIndex)
    , name_(name)
    , transport_(transport)
{
    // Create the plugin processor via PluginFactory (composition root)
    auto& factory = PluginFactory::getInstance();
    processor_ = std::unique_ptr<OscilPluginProcessor>(
        static_cast<OscilPluginProcessor*>(factory.createPluginProcessor().release()));
    processor_->setPlayHead(this);

    // Note: sourceId_ is retrieved in prepare() after prepareToPlay() is called,
    // because the processor registers its source during prepareToPlay()
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

    // Retrieve sourceId after prepareToPlay() registers the source with InstanceRegistry
    sourceId_ = processor_->getSourceId();
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
    if (editorWindow_ != nullptr)
        return;

    auto* rawEditor = processor_->createEditor();
    if (rawEditor == nullptr)
        return;

    // Window takes ownership of the editor component (setContentOwned with deleteWhenRemoved=true).
    // We store a non-owning pointer in editor_ for external access; the window manages its lifetime.
    editor_.release();
    editor_.reset(rawEditor);

    editorWindow_ = std::make_unique<EditorWindow>(name_ + " - Oscil", [this]() { hideEditor(); });

    editorWindow_->setUsingNativeTitleBar(true);
    editorWindow_->setContentNonOwned(editor_.get(), true);
    editorWindow_->setResizable(true, false);
    editorWindow_->centreWithSize(editorWindow_->getWidth(), editorWindow_->getHeight());
    editorWindow_->setVisible(true);
}

void TestTrack::hideEditor()
{
    if (editorWindow_ != nullptr)
    {
        editorWindow_->clearContentComponent(); // Detach without deleting (we own it)
        editorWindow_.reset();
    }
    editor_.reset();
}

bool TestTrack::isEditorVisible() const { return editorWindow_ != nullptr && editorWindow_->isVisible(); }

juce::Optional<juce::AudioPlayHead::PositionInfo> TestTrack::getPosition() const
{
    return transport_.getPositionInfo();
}

} // namespace oscil::test
