
#include <gtest/gtest.h>
#include "OscilTestFixtures.h"
#include "tools/PluginEditor_Adapters.h"
#include "core/dsp/TimingEngine.h"

using namespace oscil;
using namespace oscil::test;

class TimingControlsIntegrationTest : public OscilPluginTestFixture
{
};

TEST_F(TimingControlsIntegrationTest, TimingModePropagation)
{
    createEditor();
    
    // Create the adapter directly to test logic
    TimingSidebarListenerAdapter adapter(*editor);
    auto& timingEngine = processor->getTimingEngine();
    
    // Initial state check
    timingEngine.setTimingMode(TimingMode::TIME);
    EXPECT_EQ(timingEngine.getConfig().timingMode, TimingMode::TIME);
    
    // Simulate change to Melodic
    adapter.timingModeChanged(TimingMode::MELODIC);
    
    // Verify engine updated
    EXPECT_EQ(timingEngine.getConfig().timingMode, TimingMode::MELODIC);
    
    // Simulate change back to TimeBased
    adapter.timingModeChanged(TimingMode::TIME);
    EXPECT_EQ(timingEngine.getConfig().timingMode, TimingMode::TIME);
}

TEST_F(TimingControlsIntegrationTest, NoteIntervalPropagation)
{
    createEditor();
    TimingSidebarListenerAdapter adapter(*editor);
    auto& timingEngine = processor->getTimingEngine();
    
    // Initial
    timingEngine.setNoteIntervalFromEntity(NoteInterval::QUARTER);
    
    // Change to 1/8
    adapter.noteIntervalChanged(NoteInterval::EIGHTH);
    // Note: getConfig().noteInterval returns EngineNoteInterval.
    // We need to compare properly or use getNoteIntervalAsEntity()
    EXPECT_EQ(timingEngine.getNoteIntervalAsEntity(), NoteInterval::EIGHTH);
    
    // Change to 1/16
    adapter.noteIntervalChanged(NoteInterval::SIXTEENTH);
    EXPECT_EQ(timingEngine.getNoteIntervalAsEntity(), NoteInterval::SIXTEENTH);
}

TEST_F(TimingControlsIntegrationTest, TimeIntervalPropagation)
{
    createEditor();
    TimingSidebarListenerAdapter adapter(*editor);
    auto& timingEngine = processor->getTimingEngine();
    
    // Change to 50ms
    adapter.timeIntervalChanged(50.0f);
    EXPECT_EQ(timingEngine.getConfig().timeIntervalMs, 50.0f);
    
    // Change to 100ms
    adapter.timeIntervalChanged(100.0f);
    EXPECT_EQ(timingEngine.getConfig().timeIntervalMs, 100.0f);
}

TEST_F(TimingControlsIntegrationTest, BPMPropagation)
{
    createEditor();
    TimingSidebarListenerAdapter adapter(*editor);
    auto& timingEngine = processor->getTimingEngine();
    
    // Change BPM
    adapter.bpmChanged(140.0f);
    EXPECT_DOUBLE_EQ(timingEngine.getConfig().internalBPM, 140.0);
    
    adapter.bpmChanged(120.0f);
    EXPECT_DOUBLE_EQ(timingEngine.getConfig().internalBPM, 120.0);
}

TEST_F(TimingControlsIntegrationTest, WaveformModePropagation)
{
    createEditor();
    TimingSidebarListenerAdapter adapter(*editor);
    auto& timingEngine = processor->getTimingEngine();

    // Test FreeRunning
    adapter.waveformModeChanged(WaveformMode::FreeRunning);
    EXPECT_EQ(timingEngine.getConfig().triggerMode, WaveformTriggerMode::None);

    // Test RestartOnNote
    adapter.waveformModeChanged(WaveformMode::RestartOnNote);
    EXPECT_EQ(timingEngine.getConfig().triggerMode, WaveformTriggerMode::Midi);
}

TEST_F(TimingControlsIntegrationTest, OptionsPropagation)
{
    createEditor();
    // Create the adapter directly to test logic
    OptionsSidebarListenerAdapter adapter(*editor);
    auto& state = processor->getState();

    // Test GPU Toggle
    adapter.gpuRenderingChanged(false);
    EXPECT_FALSE(editor->getProcessor().getState().isGpuRenderingEnabled()); // Checking state via processor as editor delegates
    adapter.gpuRenderingChanged(true);
    EXPECT_TRUE(editor->getProcessor().getState().isGpuRenderingEnabled());

    // Test Layout
    adapter.layoutChanged(2);
    EXPECT_EQ(state.getLayoutManager().getColumnLayout(), ColumnLayout::Double);

    // Test Theme
    adapter.themeChanged("Light Mode");
    EXPECT_EQ(processor->getThemeService().getCurrentTheme().name, "Light Mode");

    // Test Quality
    adapter.qualityPresetChanged(QualityPreset::High);
    EXPECT_EQ(state.getCaptureQualityConfig().qualityPreset, QualityPreset::High);
}
