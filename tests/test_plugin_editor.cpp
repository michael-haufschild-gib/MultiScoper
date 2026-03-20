/*
    Oscil - Plugin Editor Tests
    Tests for the main plugin editor and its refactored components
*/

#include <gtest/gtest.h>
#include "OscilTestFixtures.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/managers/PerformanceMetricsController.h"
#include "ui/layout/sections/TimingSidebarSection.h"
#include "ui/layout/sections/OptionsSection.h"
#include "ui/components/OscilToggle.h"
#include "tools/PluginEditor_Adapters.h"
#include <limits>

using namespace oscil;
using namespace oscil::test;

namespace
{
int parseCountLabel(const juce::String& labelText)
{
    auto digits = labelText.retainCharacters("0123456789");
    return digits.isNotEmpty() ? digits.getIntValue() : -1;
}

juce::Label* findOscillatorCountLabel(StatusBarComponent* statusBar)
{
    if (!statusBar)
        return nullptr;

    for (auto* child : statusBar->getChildren())
    {
        auto* label = dynamic_cast<juce::Label*>(child);
        if (!label)
            continue;

        const auto text = label->getText();
        if (text.startsWithIgnoreCase("Osc:"))
            return label;
    }

    return nullptr;
}

juce::Label* findMemoryUsageLabel(StatusBarComponent* statusBar)
{
    if (!statusBar)
        return nullptr;

    for (auto* child : statusBar->getChildren())
    {
        auto* label = dynamic_cast<juce::Label*>(child);
        if (!label)
            continue;

        const auto text = label->getText();
        if (text.startsWithIgnoreCase("Mem:"))
            return label;
    }

    return nullptr;
}

TimingSidebarSection* findTimingSection(juce::Component& root)
{
    if (auto* section = dynamic_cast<TimingSidebarSection*>(&root))
        return section;

    for (auto* child : root.getChildren())
    {
        if (child == nullptr)
            continue;

        if (auto* section = findTimingSection(*child))
            return section;
    }

    return nullptr;
}

OptionsSection* findOptionsSection(juce::Component& root)
{
    if (auto* section = dynamic_cast<OptionsSection*>(&root))
        return section;

    for (auto* child : root.getChildren())
    {
        if (child == nullptr)
            continue;

        if (auto* section = findOptionsSection(*child))
            return section;
    }

    return nullptr;
}

juce::Label* findTimingBpmDisplayLabel(TimingSidebarSection& timingSection)
{
    for (auto* child : timingSection.getChildren())
    {
        if (auto* label = dynamic_cast<juce::Label*>(child))
        {
            if (label->getText().containsChar('.'))
                return label;
        }
    }

    return nullptr;
}

OscilToggle* findToggleByLabel(juce::Component& root, const juce::String& label)
{
    if (auto* toggle = dynamic_cast<OscilToggle*>(&root))
    {
        if (toggle->getLabel() == label)
            return toggle;
    }

    for (auto* child : root.getChildren())
    {
        if (child == nullptr)
            continue;

        if (auto* toggle = findToggleByLabel(*child, label))
            return toggle;
    }

    return nullptr;
}

const Oscillator* findOscillatorById(const std::vector<Oscillator>& oscillators, const OscillatorId& id)
{
    for (const auto& oscillator : oscillators)
    {
        if (oscillator.getId() == id)
            return &oscillator;
    }

    return nullptr;
}
} // namespace

/**
 * Plugin Editor tests using the standardized test fixture.
 * Uses real singletons because many UI components bypass DI.
 */
class PluginEditorTest : public OscilPluginTestFixture
{
};

TEST_F(PluginEditorTest, Construction)
{
    createEditor();
    EXPECT_NE(editor, nullptr);
    EXPECT_GT(editor->getWidth(), 0);
    EXPECT_GT(editor->getHeight(), 0);
}

TEST_F(PluginEditorTest, InitialLayout)
{
    createEditor();
    editor->setSize(800, 600);
    editor->resized();

    // Check if pane components are created
    const auto& panes = editor->getPaneComponents();
    // Verify we can call the method without crashing
    EXPECT_NO_THROW(editor->getPaneComponents());
}

TEST_F(PluginEditorTest, GpuRenderingToggle)
{
    createEditor();

    editor->setGpuRenderingEnabled(true);
    pumpMessageQueue(50);
    EXPECT_GT(editor->getWidth(), 0);

    editor->setGpuRenderingEnabled(false);
    pumpMessageQueue(50);
    EXPECT_GT(editor->getWidth(), 0);
}

TEST_F(PluginEditorTest, ResizeHandling)
{
    createEditor();

    // Test various sizes
    editor->setSize(1200, 800);
    pumpMessageQueue(20);
    EXPECT_EQ(editor->getWidth(), 1200);
    EXPECT_EQ(editor->getHeight(), 800);

    editor->setSize(600, 400);
    pumpMessageQueue(20);
    EXPECT_EQ(editor->getWidth(), 600);
    EXPECT_EQ(editor->getHeight(), 400);
}

TEST_F(PluginEditorTest, SidebarToggle)
{
    createEditor();

    // Toggle sidebar should not crash
    EXPECT_NO_THROW(editor->toggleSidebar());
    pumpMessageQueue(50);

    EXPECT_NO_THROW(editor->toggleSidebar());
    pumpMessageQueue(50);
}

TEST_F(PluginEditorTest, DisplayOptionsDoNotCrash)
{
    createEditor();

    // All display options should be callable without crashing
    EXPECT_NO_THROW(editor->setShowGridForAllPanes(true));
    EXPECT_NO_THROW(editor->setAutoScaleForAllPanes(true));
    EXPECT_NO_THROW(editor->setGainDbForAllPanes(6.0f));
    EXPECT_NO_THROW(editor->setDisplaySamplesForAllPanes(2048));

    pumpMessageQueue(50);
}

TEST_F(PluginEditorTest, RefreshPanelsDoesNotCrash)
{
    createEditor();

    // Multiple refreshes should be stable
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_NO_THROW(editor->refreshPanels());
        pumpMessageQueue(50);
    }
}

TEST_F(PluginEditorTest, StatusBarOscillatorCountUpdatesPromptlyAfterAdd)
{
    StatusBarComponent statusBar(processor->getThemeService());
    PerformanceMetricsController metricsController(*processor, statusBar);

    auto* countLabel = findOscillatorCountLabel(&statusBar);
    ASSERT_NE(countLabel, nullptr);

    const auto initialCount = static_cast<int>(processor->getState().getOscillators().size());
    metricsController.update();
    EXPECT_EQ(parseCountLabel(countLabel->getText()), initialCount);

    Oscillator addedOscillator;
    addedOscillator.setName("Added from test");
    processor->getState().addOscillator(addedOscillator);
    const auto expectedCount = static_cast<int>(processor->getState().getOscillators().size());
    EXPECT_EQ(expectedCount, initialCount + 1);

    // Regression check: second update should publish the latest count immediately.
    metricsController.update();
    EXPECT_EQ(parseCountLabel(countLabel->getText()), expectedCount);
}

TEST_F(PluginEditorTest, StatusBarMemoryUsageUpdatesPromptlyAfterAdd)
{
    StatusBarComponent statusBar(processor->getThemeService());
    PerformanceMetricsController metricsController(*processor, statusBar);

    auto* memoryLabel = findMemoryUsageLabel(&statusBar);
    ASSERT_NE(memoryLabel, nullptr);

    metricsController.update();
    const auto initialMemoryText = memoryLabel->getText();

    Oscillator addedOscillator;
    addedOscillator.setName("Memory increment");
    processor->getState().addOscillator(addedOscillator);

    metricsController.update();
    const auto updatedMemoryText = memoryLabel->getText();

    EXPECT_NE(updatedMemoryText, initialMemoryText);
}

TEST_F(PluginEditorTest, DenseSingleColumnLayoutMaintainsMinimumPaneHeight)
{
    createEditor();
    editor->setSize(1200, 800);
    editor->resized();
    pumpMessageQueue(50);

    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();

    state.getOscillators().clear();
    layoutManager.clear();
    layoutManager.setColumnLayout(ColumnLayout::Triple);

    std::vector<PaneId> paneIds;
    paneIds.reserve(5);

    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        pane.setName("Pane " + juce::String(i + 1));
        layoutManager.addPane(pane);
        paneIds.push_back(pane.getId());
    }

    for (int i = 0; i < static_cast<int>(paneIds.size()); ++i)
    {
        layoutManager.movePaneToColumn(paneIds[static_cast<size_t>(i)], 0, i);
    }

    ASSERT_EQ(layoutManager.getPaneCountInColumn(0), 5);

    editor->refreshPanels();
    pumpMessageQueue(100);

    const auto& paneComponents = editor->getPaneComponents();
    ASSERT_EQ(paneComponents.size(), 5);

    int minPaneHeight = std::numeric_limits<int>::max();
    for (const auto& paneComponent : paneComponents)
    {
        minPaneHeight = std::min(minPaneHeight, paneComponent->getHeight());
    }

    // Regression: concentrated panes in one column should still get the minimum practical pane height.
    EXPECT_GE(minPaneHeight, 190);
}

TEST_F(PluginEditorTest, TimingAdapterHostBpmCallbackUpdatesTimingSectionDisplay)
{
    createEditor();

    auto* timingSection = findTimingSection(*editor);
    ASSERT_NE(timingSection, nullptr);

    timingSection->setTimingMode(TimingMode::MELODIC);
    timingSection->setHostSyncEnabled(true);
    pumpMessageQueue(50);

    auto* bpmLabel = findTimingBpmDisplayLabel(*timingSection);
    ASSERT_NE(bpmLabel, nullptr);

    TimingEngineListenerAdapter adapter(*editor);
    adapter.hostBPMChanged(137.0f);
    pumpMessageQueue(50);

    EXPECT_EQ(bpmLabel->getText(), juce::String(137.0f, 1));
}

TEST_F(PluginEditorTest, TimingSectionChangesPropagateToTimingEngine)
{
    createEditor();

    auto* timingSection = findTimingSection(*editor);
    ASSERT_NE(timingSection, nullptr);

    timingSection->setTimingMode(TimingMode::MELODIC);
    timingSection->setNoteInterval(NoteInterval::HALF);
    timingSection->setInternalBPM(97.5f);
    pumpMessageQueue(20);

    auto timingConfig = processor->getTimingEngine().toEntityConfig();
    auto engineConfig = processor->getTimingEngine().getConfig();
    EXPECT_EQ(timingConfig.timingMode, TimingMode::MELODIC);
    EXPECT_EQ(timingConfig.noteInterval, NoteInterval::HALF);
    EXPECT_NEAR(engineConfig.internalBPM, 97.5f, 0.05f);
}

TEST_F(PluginEditorTest, TimingSectionInitializesWaveformModeFromTimingEngineState)
{
    processor->getTimingEngine().setWaveformTriggerMode(WaveformTriggerMode::Midi);
    processor->getTimingEngine().setSyncToPlayhead(false);

    createEditor();

    auto* timingSection = findTimingSection(*editor);
    ASSERT_NE(timingSection, nullptr);
    EXPECT_EQ(timingSection->getWaveformMode(), WaveformMode::RestartOnNote);
}

TEST_F(PluginEditorTest, TimerDoesNotDrainTriggersWhenRestartModeIsInactive)
{
    createEditor();

    auto& timingEngine = processor->getTimingEngine();
    timingEngine.setSyncToPlayhead(false);
    timingEngine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);

    timingEngine.requestManualTrigger();
    pumpMessageQueue(80);

    EXPECT_TRUE(timingEngine.checkAndClearTrigger());
}

TEST_F(PluginEditorTest, OptionsShowGridToggleUpdatesState)
{
    createEditor();

    auto* optionsSection = findOptionsSection(*editor);
    ASSERT_NE(optionsSection, nullptr);

    auto* showGridToggle = findToggleByLabel(*optionsSection, "Show Grid");
    ASSERT_NE(showGridToggle, nullptr);

    processor->getState().setShowGridEnabled(true);
    showGridToggle->setValue(false, false);
    pumpMessageQueue(20);

    EXPECT_FALSE(processor->getState().isShowGridEnabled());
}

TEST_F(PluginEditorTest, DisconnectedSourceRemainsDisconnectedWhenSourceListChanges)
{
    createEditor();

    auto* controller = editor->getOscillatorPanelController();
    ASSERT_NE(controller, nullptr);

    auto oscillators = processor->getState().getOscillators();
    ASSERT_FALSE(oscillators.empty());

    const auto oscillatorId = oscillators.front().getId();
    controller->updateOscillatorSource(oscillatorId, SourceId::noSource());
    pumpMessageQueue(60);

    auto afterDisconnect = processor->getState().getOscillators();
    const auto* disconnectedOsc = findOscillatorById(afterDisconnect, oscillatorId);
    ASSERT_NE(disconnectedOsc, nullptr);
    ASSERT_TRUE(disconnectedOsc->getSourceId().isNoSource());

    auto externalBuffer = std::make_shared<SharedCaptureBuffer>();
    auto externalSourceId = getRegistry().registerInstance(
        "external_disconnect_regression_track",
        externalBuffer,
        "External Disconnect Regression",
        2,
        44100.0);
    ASSERT_TRUE(externalSourceId.isValid());

    pumpMessageQueue(80);

    auto afterSourceUpdate = processor->getState().getOscillators();
    const auto* updatedOsc = findOscillatorById(afterSourceUpdate, oscillatorId);
    ASSERT_NE(updatedOsc, nullptr);
    EXPECT_TRUE(updatedOsc->getSourceId().isNoSource());

    getRegistry().unregisterInstance(externalSourceId);
    pumpMessageQueue(40);
}

TEST_F(PluginEditorTest, ConstructionReconcilesLegacyInvalidSourceToOwnTrack)
{
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();

    state.getOscillators().clear();
    layoutManager.clear();

    Pane pane;
    pane.setName("Legacy Pane");
    layoutManager.addPane(pane);

    Oscillator staleOscillator;
    staleOscillator.setPaneId(pane.getId());
    staleOscillator.setName("Legacy Oscillator");
    staleOscillator.setSourceId(SourceId::invalid());
    const auto staleOscillatorId = staleOscillator.getId();
    state.addOscillator(staleOscillator);

    ASSERT_TRUE(processor->getSourceId().isValid());

    createEditor();
    pumpMessageQueue(80);

    auto postConstructionOscillators = state.getOscillators();
    const auto* reconciledOscillator = findOscillatorById(postConstructionOscillators, staleOscillatorId);
    ASSERT_NE(reconciledOscillator, nullptr);
    EXPECT_EQ(reconciledOscillator->getSourceId(), processor->getSourceId());
}
