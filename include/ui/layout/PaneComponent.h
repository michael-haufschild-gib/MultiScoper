/*
    Oscil - Pane Component Header
    Container component for oscillator waveforms
*/

#pragma once

#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/ServiceContext.h"
#include "ui/components/TestId.h"
#include "ui/layout/pane/PaneBody.h"
#include "ui/layout/pane/PaneHeader.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace oscil
{

class IAudioDataProvider;
class ShaderRegistry;

/**
 * Component that contains one or more oscillator waveforms stacked vertically.
 * Supports drag-and-drop reordering.
 *
 * Structure:
 * - PaneHeader: Drag handle, name, badges, action bar, close button
 * - PaneBody: WaveformStack, CrosshairOverlay, StatsOverlay
 */
class PaneComponent
    : public juce::Component
    , public juce::DragAndDropTarget
    , public TestIdSupport
{
public:
    /// Construct a pane bound to the given data provider and assigned the given pane ID.
    PaneComponent(IAudioDataProvider& dataProvider, ServiceContext& context, const PaneId& paneId);
    ~PaneComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Accept oscillator drag-drop sources for reassignment (DragAndDropTarget).
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    /**
     * Add an oscillator to this pane
     */
    void addOscillator(const Oscillator& oscillator);

    /**
     * Remove an oscillator from this pane
     */
    void removeOscillator(const OscillatorId& oscillatorId);

    /**
     * Clear all oscillators
     */
    void clearOscillators();

    /**
     * Update the capture buffer for an oscillator when its source changes
     * @param oscillatorId The oscillator to update
     * @param newSourceId The new source to bind to
     */
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);

    /**
     * Update an oscillator's properties (mode, visibility, etc)
     * @param oscillatorId The oscillator to update
     * @param mode The new processing mode
     * @param visible Whether the waveform should be visible
     */
    void updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible);

    /**
     * Update an oscillator's name
     * @param oscillatorId The oscillator to update
     * @param name The new name
     */
    void updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name);

    /**
     * Update an oscillator's color
     * @param oscillatorId The oscillator to update
     * @param color The new color
     */
    void updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color);

    /**
     * Update all properties of an oscillator (used when config popup changes are made)
     * @param oscillator The oscillator with updated properties
     */
    void updateOscillatorFull(const Oscillator& oscillator);

    /**
     * Get the pane ID
     */
    PaneId getPaneId() const { return paneId_; }

    /**
     * Get the number of oscillators in this pane
     */
    size_t getOscillatorCount() const;

    /**
     * Set callback for when pane is reordered
     */
    void onPaneReordered(std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)> callback)
    {
        paneReorderedCallback_ = std::move(callback);
    }

    /**
     * Set callback for when pane close is requested
     */
    void onPaneCloseRequested(std::function<void(const PaneId& paneId)> callback)
    {
        paneCloseCallback_ = std::move(callback);
    }

    /**
     * Set callback for when pane name is changed
     */
    void onPaneNameChanged(std::function<void(const PaneId& paneId, const juce::String& newName)> callback)
    {
        paneNameChangedCallback_ = std::move(callback);
    }

    /**
     * Set the pane name
     */
    void setPaneName(const juce::String& name);

    /**
     * Get the pane name
     */
    juce::String getPaneName() const;

    /**
     * Get pane index for drag-and-drop
     */
    void setPaneIndex(int index);
    int getPaneIndex() const { return paneIndex_; }

    // Display options - apply to all waveforms in this pane
    void setShowGrid(bool enabled);
    void setGridConfig(const GridConfiguration& config);
    void setAutoScale(bool enabled);

    /**
     * Toggle hold/pause display for this pane
     */
    void toggleHoldDisplay();

    /**
     * Set hold display directly
     */
    void setHoldDisplay(bool enabled);

    /**
     * Check if pane is currently held/paused
     */
    bool isHoldDisplayEnabled() const { return isHeld_; }

    void setGainDb(float dB);
    void setDisplaySamples(int samples);
    void setSampleRate(int sampleRate);
    void requestWaveformRestartAtTimestamp(int64_t timelineSampleTimestamp);

    /**
     * Highlight a specific oscillator (or clear highlight if oscillatorId is invalid)
     */
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Drag-and-drop identifier
    static constexpr const char* PANE_DRAG_ID = "OscilPane";

    /// Get the waveform component at the given stack index (test access only).
    WaveformComponent* getWaveformAt(size_t index) const;
    /// Get the oscillator model at the given stack index (test access only).
    const Oscillator* getOscillatorAt(size_t index) const;

    // Component access for testing
    PaneHeader* getHeader() { return header_.get(); }
    PaneBody* getBody() { return body_.get(); }

private:
    void handleHeaderAction(PaneAction action, bool state);
    void handleDragStarted(const juce::MouseEvent& event);
    void updateHeaderBadge();

    IAudioDataProvider& dataProvider_;
    IThemeService& themeService_;
    ShaderRegistry& shaderRegistry_;
    PaneId paneId_;
    int paneIndex_ = 0;
    juce::String paneName_;

    // Sub-components
    std::unique_ptr<PaneHeader> header_;
    std::unique_ptr<PaneBody> body_;

    // Drag state
    bool isDragOver_ = false;
    juce::Point<int> dragStartPos_;

    // Pane state
    bool isHeld_ = false;

    // Callbacks
    std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)> paneReorderedCallback_;
    std::function<void(const PaneId& paneId)> paneCloseCallback_;
    std::function<void(const PaneId& paneId, const juce::String& newName)> paneNameChangedCallback_;

    static constexpr int DRAG_THRESHOLD = 5;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneComponent)
};

} // namespace oscil
