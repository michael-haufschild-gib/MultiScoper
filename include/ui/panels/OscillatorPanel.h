/*
    Oscil - Oscillator Panel Header
    Panel for configuring individual oscillator settings
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "core/InstanceRegistry.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/TestId.h"
#include "ui/panels/OscillatorPresenter.h"
#include <functional>

namespace oscil
{

// Forward declarations
class ColorPickerComponent;
class SourceSelectorComponent;

/**
 * Panel for configuring an individual oscillator.
 * Provides controls for source, processing mode, color, visibility, and deletion.
 */
class OscillatorPanel : public juce::Component
{
public:
    explicit OscillatorPanel(const Oscillator& oscillator);
    ~OscillatorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Get the oscillator ID this panel controls
     */
    OscillatorId getOscillatorId() const;

    /**
     * Update the oscillator data
     */
    void setOscillator(const Oscillator& oscillator);

    /**
     * Get current oscillator configuration
     */
    Oscillator getOscillator() const;

    /**
     * Set callback for when oscillator configuration changes
     */
    void onOscillatorChanged(std::function<void(const Oscillator&)> callback)
    {
        oscillatorChangedCallback_ = std::move(callback);
    }

    /**
     * Set callback for when delete is requested
     */
    void onDeleteRequested(std::function<void(const OscillatorId&)> callback)
    {
        deleteRequestedCallback_ = std::move(callback);
    }

    /**
     * Set callback for when visibility toggle is requested
     */
    void onVisibilityToggled(std::function<void(const OscillatorId&, bool)> callback)
    {
        visibilityToggledCallback_ = std::move(callback);
    }

    // Height constants
    static constexpr int COLLAPSED_HEIGHT = 32;
    static constexpr int EXPANDED_HEIGHT = 180;

    /**
     * Check if the panel is expanded
     */
    bool isExpanded() const { return expanded_; }

    /**
     * Set expanded state
     */
    void setExpanded(bool expanded);

private:
    void setupPresenterCallbacks();
    void updateUi();

    void handleSourceChange(const SourceId& sourceId);
    void handleProcessingModeChange();
    void handleColourChange(juce::Colour colour);
    void handleVisibilityToggle();
    void handleDeleteClick();
    void handleExpandToggle();

    std::unique_ptr<OscillatorPresenter> presenter_;
    bool expanded_ = false;

    // Header controls (always visible)
    std::unique_ptr<juce::Label> nameLabel_;
    std::unique_ptr<OscilToggle> visibilityToggle_;
    std::unique_ptr<OscilButton> expandButton_;
    std::unique_ptr<OscilButton> deleteButton_;

    // Expanded controls
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<SourceSelectorComponent> sourceSelector_;
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<OscilDropdown> processingModeSelector_;
    std::unique_ptr<juce::Label> colourLabel_;
    std::unique_ptr<ColorPickerComponent> colorPicker_;

    // Callbacks
    std::function<void(const Oscillator&)> oscillatorChangedCallback_;
    std::function<void(const OscillatorId&)> deleteRequestedCallback_;
    std::function<void(const OscillatorId&, bool)> visibilityToggledCallback_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorPanel)
};

} // namespace oscil
