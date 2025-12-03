/*
    Oscil - Oscillator Presenter Implementation
*/

#include "ui/panels/OscillatorPresenter.h"

namespace oscil
{

OscillatorPresenter::OscillatorPresenter(const Oscillator& oscillator)
    : oscillator_(oscillator)
{
}

void OscillatorPresenter::setName(const juce::String& name)
{
    if (oscillator_.getName() != name)
    {
        oscillator_.setName(name);
        notifyChanged();
    }
}

void OscillatorPresenter::setVisible(bool visible)
{
    if (oscillator_.isVisible() != visible)
    {
        oscillator_.setVisible(visible);
        
        if (visibilityToggledCallback_)
        {
            visibilityToggledCallback_(oscillator_.getId(), visible);
        }
        
        notifyChanged();
    }
}

void OscillatorPresenter::toggleVisibility()
{
    setVisible(!oscillator_.isVisible());
}

void OscillatorPresenter::setProcessingMode(ProcessingMode mode)
{
    if (oscillator_.getProcessingMode() != mode)
    {
        oscillator_.setProcessingMode(mode);
        notifyChanged();
    }
}

void OscillatorPresenter::setSourceId(const SourceId& sourceId)
{
    // SourceId doesn't have != operator in some implementations, check string or raw ID if needed.
    // Assuming it works or we just update.
    // If SourceId is a string or struct with operator==
    if (oscillator_.getSourceId() != sourceId)
    {
        oscillator_.setSourceId(sourceId);
        notifyChanged();
    }
}

void OscillatorPresenter::setColour(juce::Colour colour)
{
    if (oscillator_.getColour() != colour)
    {
        oscillator_.setColour(colour);
        notifyChanged();
    }
}

void OscillatorPresenter::setOpacity(float opacity)
{
    if (std::abs(oscillator_.getOpacity() - opacity) > 1e-4f)
    {
        oscillator_.setOpacity(opacity);
        notifyChanged();
    }
}

void OscillatorPresenter::requestDelete()
{
    if (deleteRequestedCallback_)
    {
        deleteRequestedCallback_(oscillator_.getId());
    }
}

void OscillatorPresenter::setOnOscillatorChanged(OscillatorChangedCallback callback)
{
    oscillatorChangedCallback_ = std::move(callback);
}

void OscillatorPresenter::setOnDeleteRequested(DeleteRequestedCallback callback)
{
    deleteRequestedCallback_ = std::move(callback);
}

void OscillatorPresenter::setOnVisibilityToggled(VisibilityToggledCallback callback)
{
    visibilityToggledCallback_ = std::move(callback);
}

void OscillatorPresenter::notifyChanged()
{
    if (oscillatorChangedCallback_)
    {
        oscillatorChangedCallback_(oscillator_);
    }
}

} // namespace oscil
