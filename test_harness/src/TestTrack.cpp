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

TestTrack::TestTrack(int trackIndex, const juce::String& name, TestTransport& transport,
                     IInstanceRegistry* overrideRegistry)
    : trackIndex_(trackIndex)
    , name_(name)
    , transport_(transport)
    , overrideRegistry_(overrideRegistry)
{
    auto& factory = PluginFactory::getInstance();

    if (overrideRegistry_ != nullptr)
    {
        // Isolation mode: wire this processor to a caller-owned registry
        // so it does NOT share cross-instance state with the factory
        // singleton's registry. All other services (theme, shaders, presets,
        // memory budget) still come from the factory — those are process-
        // global in a real DAW too; only the registry is per-process under
        // AU sandboxing.
        PluginProcessorConfig const config{.instanceRegistry = *overrideRegistry_,
                                           .themeService = factory.getThemeManager(),
                                           .shaderRegistry = factory.getShaderRegistry(),
                                           .presetManager = factory.getPresetManager(),
                                           .memoryBudgetManager = factory.getMemoryBudgetManager()};
        processor_ = std::make_unique<OscilPluginProcessor>(config);
    }
    else
    {
        // Default path: use the factory's shared registry (normal harness
        // behaviour + production default).
        processor_ = std::unique_ptr<OscilPluginProcessor>(
            static_cast<OscilPluginProcessor*>(factory.createPluginProcessor().release()));
    }
    processor_->setPlayHead(this);

    // Note: sourceId_ is retrieved in prepare() after prepareToPlay() is called,
    // because the processor registers its source during prepareToPlay()
}

IInstanceRegistry& TestTrack::getInstanceRegistry() const
{
    if (overrideRegistry_ != nullptr)
        return *overrideRegistry_;
    return PluginFactory::getInstance().getInstanceRegistry();
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

    // Prepare audio buffer.  juce::AudioBuffer::setSize reallocates when the
    // requested size differs from the current size, so re-prepare with a new
    // block size (sample-rate / buffer-size change mid-session) transparently
    // resizes the scratch buffer to match.  On the first call the buffer has
    // zero channels and we default to stereo; on re-prepare we keep whatever
    // channel count setChannelLayout left behind so layout changes are not
    // silently reverted by a later prepare().
    const int currentChannels = audioBuffer_.getNumChannels();
    const int channels = currentChannels > 0 ? currentChannels : 2;
    audioBuffer_.setSize(channels, samplesPerBlock, /*keepExistingContent=*/false,
                         /*clearExtraSpace=*/true, /*avoidReallocating=*/false);

    // Prepare plugin processor
    processor_->prepareToPlay(sampleRate, samplesPerBlock);

    // Retrieve sourceId after prepareToPlay() registers the source with InstanceRegistry
    {
        std::lock_guard<std::mutex> lock(sourceIdMutex_);
        sourceId_ = processor_->getSourceId();
    }
}

void TestTrack::processBlock()
{
    // Clear and generate test audio
    audioBuffer_.clear();
    audioGenerator_.generateBlock(audioBuffer_);

    // Process through plugin
    processor_->processBlock(audioBuffer_, midiBuffer_);

    // NOTE: transport_.advancePosition is NOT called here. The DAW dispatcher
    // (TestDAW::processAudioTick) advances the shared transport exactly once
    // per tick — calling it per-track would (a) race across parallel pool
    // jobs and (b) advance N× per block, which no real DAW does.
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

bool TestTrack::detachEditor()
{
    // Only meaningful when both window and editor exist and the editor is
    // currently parented.  Detaching without destroying fires
    // parentHierarchyChanged(nullptr) on the editor — the production code path
    // that real hosts drive when the user collapses a plugin window.
    if (editorWindow_ == nullptr || editor_ == nullptr)
        return false;

    if (editor_->getParentComponent() == nullptr)
        return false; // already detached

    editorWindow_->clearContentComponent();
    return true;
}

bool TestTrack::reattachEditor()
{
    if (editor_ == nullptr)
        return false;

    // If the window was destroyed (e.g., hideEditor was called), create a
    // fresh one so reattach still succeeds.  Matches the DAW behaviour of
    // rehydrating the frame around an existing editor.
    if (editorWindow_ == nullptr)
    {
        editorWindow_ = std::make_unique<EditorWindow>(name_ + " - Oscil", [this]() { hideEditor(); });
        editorWindow_->setUsingNativeTitleBar(true);
        editorWindow_->setResizable(true, false);
    }

    if (editor_->getParentComponent() == editorWindow_.get())
        return false; // already attached

    editorWindow_->setContentNonOwned(editor_.get(), true);
    editorWindow_->centreWithSize(editorWindow_->getWidth(), editorWindow_->getHeight());
    editorWindow_->setVisible(true);
    return true;
}

bool TestTrack::setChannelLayout(const juce::String& layout)
{
    if (processor_ == nullptr)
        return false;

    juce::AudioChannelSet channelSet;
    int channels = 0;
    if (layout == "mono")
    {
        channelSet = juce::AudioChannelSet::mono();
        channels = 1;
    }
    else if (layout == "stereo")
    {
        channelSet = juce::AudioChannelSet::stereo();
        channels = 2;
    }
    else
    {
        return false; // invalid argument
    }

    juce::AudioProcessor::BusesLayout newLayout;
    newLayout.inputBuses.add(channelSet);
    newLayout.outputBuses.add(channelSet);

    // setBusesLayout invokes isBusesLayoutSupported; if the processor rejects
    // the request we leave the scratch buffer untouched — that mirrors what a
    // real host sees when renegotiation is refused.
    if (!processor_->setBusesLayout(newLayout))
        return false;

    // Resize scratch buffer to match the new channel count.  Keep the block
    // size, do not retain content (a layout change is a barrier, not a
    // continuous stream).
    audioBuffer_.setSize(channels, samplesPerBlock_, /*keepExistingContent=*/false,
                         /*clearExtraSpace=*/true, /*avoidReallocating=*/false);
    return true;
}

juce::Optional<juce::AudioPlayHead::PositionInfo> TestTrack::getPosition() const
{
    return transport_.getPositionInfo();
}

} // namespace oscil::test
