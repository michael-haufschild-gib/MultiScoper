/*
    Oscil - Oscillator Presenter
    Handles logic for oscillator configuration and state management.
    Separates logic from View (OscillatorPanel).
*/

#pragma once

#include "core/Oscillator.h"
#include <functional>
#include <juce_graphics/juce_graphics.h> // For juce::Colour

namespace oscil
{

class OscillatorPresenter
{
public:
    explicit OscillatorPresenter(const Oscillator& oscillator);
    virtual ~OscillatorPresenter() = default;

    // Modifiers
    void setName(const juce::String& name);
    void setVisible(bool visible);
    void setProcessingMode(ProcessingMode mode);
    void setSourceId(const SourceId& sourceId);
    void setColour(juce::Colour colour);
    void setOpacity(float opacity);
    
    // Actions
    void toggleVisibility();
    void requestDelete();

    // Accessors
    const Oscillator& getOscillator() const { return oscillator_; }
    OscillatorId getOscillatorId() const { return oscillator_.getId(); }
    SourceId getSourceId() const { return oscillator_.getSourceId(); }
    ProcessingMode getProcessingMode() const { return oscillator_.getProcessingMode(); }
    juce::Colour getColour() const { return oscillator_.getColour(); }
    float getOpacity() const { return oscillator_.getOpacity(); }
    bool isVisible() const { return oscillator_.isVisible(); }
    juce::String getName() const { return oscillator_.getName(); }

    // Callbacks
    using OscillatorChangedCallback = std::function<void(const Oscillator&)>;
    using DeleteRequestedCallback = std::function<void(const OscillatorId&)>;
    using VisibilityToggledCallback = std::function<void(const OscillatorId&, bool)>;

    void setOnOscillatorChanged(OscillatorChangedCallback callback);
    void setOnDeleteRequested(DeleteRequestedCallback callback);
    void setOnVisibilityToggled(VisibilityToggledCallback callback);

private:
    Oscillator oscillator_;

    OscillatorChangedCallback oscillatorChangedCallback_;
    DeleteRequestedCallback deleteRequestedCallback_;
    VisibilityToggledCallback visibilityToggledCallback_;

    void notifyChanged();
};

} // namespace oscil
