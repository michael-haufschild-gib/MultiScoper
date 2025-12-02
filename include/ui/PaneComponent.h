/*
    Oscil - Pane Component Header
    Container component for oscillator waveforms
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "WaveformComponent.h"
#include "ui/components/TestId.h"
#include <vector>
#include <memory>
#include <functional>

namespace oscil
{

class OscilPluginProcessor;

/**
 * Component that contains one or more oscillator waveforms stacked vertically.
 * Supports drag-and-drop reordering.
 */
class PaneComponent : public juce::Component,
                       public juce::DragAndDropTarget,
                       public TestIdSupport
{
public:
    PaneComponent(OscilPluginProcessor& processor, const PaneId& paneId);
    ~PaneComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse events for drag initiation
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    // DragAndDropTarget interface
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
    size_t getOscillatorCount() const { return waveforms_.size(); }

    /**
     * Set callback for when pane is reordered
     */
    void onPaneReordered(std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)> callback)
    {
        paneReorderedCallback_ = std::move(callback);
    }

    /**
     * Get pane index for drag-and-drop
     */
    void setPaneIndex(int index);
    int getPaneIndex() const { return paneIndex_; }

    // Display options - apply to all waveforms in this pane
    void setShowGrid(bool enabled);
    void setGridConfig(const GridConfiguration& config);
    void setAutoScale(bool enabled);
    void setHoldDisplay(bool enabled);
    void setGainDb(float dB);
    void setDisplaySamples(int samples);

    /**
     * Highlight a specific oscillator (or clear highlight if oscillatorId is invalid)
     */
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Drag-and-drop identifier
    static constexpr const char* PANE_DRAG_ID = "OscilPane";

    // Test access - for automated testing only
    WaveformComponent* getWaveformAt(size_t index) const {
        return (index < waveforms_.size()) ? waveforms_[index].waveform.get() : nullptr;
    }
    const Oscillator* getOscillatorAt(size_t index) const {
        return (index < waveforms_.size()) ? &waveforms_[index].oscillator : nullptr;
    }

private:
    void updateLayout();
    bool isInDragZone(const juce::Point<int>& position) const;

    OscilPluginProcessor& processor_;
    PaneId paneId_;
    int paneIndex_ = 0;

    struct OscillatorEntry
    {
        Oscillator oscillator;
        std::unique_ptr<WaveformComponent> waveform;
    };

    std::vector<OscillatorEntry> waveforms_;

    // Drag state
    bool isDragOver_ = false;
    juce::Point<int> dragStartPos_;

    // Callbacks
    std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)> paneReorderedCallback_;

    static constexpr int HEADER_HEIGHT = 24;
    static constexpr int PADDING = 4;
    static constexpr int DRAG_THRESHOLD = 5;
    
    // Drag handle styling
    static constexpr int DRAG_HANDLE_WIDTH = 20;
    static constexpr int DRAG_HANDLE_LINE_WIDTH = 10;
    static constexpr int DRAG_HANDLE_LINE_HEIGHT = 2;
    static constexpr int DRAG_HANDLE_LINE_SPACING = 4;
    static constexpr int DRAG_HANDLE_LEFT_MARGIN = 4;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneComponent)
};

} // namespace oscil
