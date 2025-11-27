/*
    Oscil - Oscillator Panel Implementation
    Panel for configuring individual oscillator settings
*/

#include "ui/OscillatorPanel.h"
#include "ui/ColorPickerComponent.h"
#include "ui/SourceSelectorComponent.h"
#include "ui/ThemeManager.h"

namespace oscil
{

OscillatorPanel::OscillatorPanel(const Oscillator& oscillator)
    : oscillatorId_(oscillator.getId())
    , sourceId_(oscillator.getSourceId())
    , processingMode_(oscillator.getProcessingMode())
    , colour_(oscillator.getColour())
    , opacity_(oscillator.getOpacity())
    , visible_(oscillator.isVisible())
    , name_(oscillator.getName())
{
    // Header controls
    nameLabel_ = std::make_unique<juce::Label>("name", name_.isEmpty() ? "Oscillator" : name_);
    nameLabel_->setEditable(true);
    nameLabel_->onTextChange = [this]()
    {
        name_ = nameLabel_->getText();
        notifyOscillatorChanged();
    };
    addAndMakeVisible(*nameLabel_);

    visibilityToggle_ = std::make_unique<juce::ToggleButton>();
    visibilityToggle_->setToggleState(visible_, juce::dontSendNotification);
    visibilityToggle_->setButtonText(visible_ ? "V" : "H");
    visibilityToggle_->onClick = [this]() { handleVisibilityToggle(); };
    addAndMakeVisible(*visibilityToggle_);

    expandButton_ = std::make_unique<juce::TextButton>("...");
    expandButton_->onClick = [this]() { handleExpandToggle(); };
    addAndMakeVisible(*expandButton_);

    deleteButton_ = std::make_unique<juce::TextButton>("X");
    deleteButton_->onClick = [this]() { handleDeleteClick(); };
    addAndMakeVisible(*deleteButton_);

    // Expanded controls (initially hidden)
    sourceLabel_ = std::make_unique<juce::Label>("", "Source:");
    addChildComponent(*sourceLabel_);

    sourceSelector_ = std::make_unique<SourceSelectorComponent>();
    sourceSelector_->setSelectedSourceId(sourceId_);
    sourceSelector_->onSelectionChanged([this](const SourceId& sid) { handleSourceChange(sid); });
    addChildComponent(*sourceSelector_);

    modeLabel_ = std::make_unique<juce::Label>("", "Mode:");
    addChildComponent(*modeLabel_);

    processingModeSelector_ = std::make_unique<juce::ComboBox>("mode");
    processingModeSelector_->addItem("Full Stereo", 1);
    processingModeSelector_->addItem("Mono", 2);
    processingModeSelector_->addItem("Mid", 3);
    processingModeSelector_->addItem("Side", 4);
    processingModeSelector_->addItem("Left", 5);
    processingModeSelector_->addItem("Right", 6);
    processingModeSelector_->setSelectedId(static_cast<int>(processingMode_) + 1, juce::dontSendNotification);
    processingModeSelector_->onChange = [this]() { handleProcessingModeChange(); };
    addChildComponent(*processingModeSelector_);

    colourLabel_ = std::make_unique<juce::Label>("", "Colour:");
    addChildComponent(*colourLabel_);

    colorPicker_ = std::make_unique<ColorPickerComponent>();
    colorPicker_->setColour(colour_.withAlpha(opacity_));
    colorPicker_->onColourChanged([this](juce::Colour c) { handleColourChange(c); });
    addChildComponent(*colorPicker_);
}

OscillatorPanel::~OscillatorPanel() = default;

void OscillatorPanel::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Draw background
    g.setColour(theme.backgroundSecondary);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Draw border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);

    // Draw color indicator on left side
    auto colorBar = getLocalBounds().removeFromLeft(4);
    g.setColour(colour_.withAlpha(opacity_));
    g.fillRoundedRectangle(colorBar.toFloat(), 2.0f);
}

void OscillatorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Header row
    auto headerRow = bounds.removeFromTop(24);

    // Left side: visibility toggle and name
    visibilityToggle_->setBounds(headerRow.removeFromLeft(24));
    headerRow.removeFromLeft(4);

    // Right side: expand and delete buttons
    deleteButton_->setBounds(headerRow.removeFromRight(24));
    headerRow.removeFromRight(4);
    expandButton_->setBounds(headerRow.removeFromRight(30));
    headerRow.removeFromRight(4);

    // Name takes remaining space
    nameLabel_->setBounds(headerRow);

    if (expanded_)
    {
        bounds.removeFromTop(8); // Spacing

        const int rowHeight = 24;
        const int labelWidth = 60;
        const int spacing = 4;

        // Source selector row
        auto sourceRow = bounds.removeFromTop(rowHeight);
        sourceLabel_->setBounds(sourceRow.removeFromLeft(labelWidth));
        sourceSelector_->setBounds(sourceRow);
        bounds.removeFromTop(spacing);

        // Processing mode row
        auto modeRow = bounds.removeFromTop(rowHeight);
        modeLabel_->setBounds(modeRow.removeFromLeft(labelWidth));
        processingModeSelector_->setBounds(modeRow);
        bounds.removeFromTop(spacing);

        // Color picker row
        auto colourRow = bounds.removeFromTop(rowHeight);
        colourLabel_->setBounds(colourRow.removeFromLeft(labelWidth));
        bounds.removeFromTop(spacing);

        // Color picker takes remaining height
        colorPicker_->setBounds(bounds);
    }
}

void OscillatorPanel::setOscillator(const Oscillator& oscillator)
{
    oscillatorId_ = oscillator.getId();
    sourceId_ = oscillator.getSourceId();
    processingMode_ = oscillator.getProcessingMode();
    colour_ = oscillator.getColour();
    opacity_ = oscillator.getOpacity();
    visible_ = oscillator.isVisible();
    name_ = oscillator.getName();

    // Update UI
    nameLabel_->setText(name_.isEmpty() ? "Oscillator" : name_, juce::dontSendNotification);
    visibilityToggle_->setToggleState(visible_, juce::dontSendNotification);
    visibilityToggle_->setButtonText(visible_ ? "V" : "H");
    sourceSelector_->setSelectedSourceId(sourceId_);
    processingModeSelector_->setSelectedId(static_cast<int>(processingMode_) + 1, juce::dontSendNotification);
    colorPicker_->setColour(colour_.withAlpha(opacity_));

    repaint();
}

Oscillator OscillatorPanel::getOscillator() const
{
    Oscillator osc;
    // Note: We can't set the ID directly as it's generated, but we store it
    // The caller should use the ID from getOscillatorId()
    osc.setSourceId(sourceId_);
    osc.setProcessingMode(processingMode_);
    osc.setColour(colour_);
    osc.setOpacity(opacity_);
    osc.setVisible(visible_);
    osc.setName(name_);
    return osc;
}

void OscillatorPanel::setExpanded(bool expanded)
{
    if (expanded_ != expanded)
    {
        expanded_ = expanded;

        // Show/hide expanded controls
        sourceLabel_->setVisible(expanded_);
        sourceSelector_->setVisible(expanded_);
        modeLabel_->setVisible(expanded_);
        processingModeSelector_->setVisible(expanded_);
        colourLabel_->setVisible(expanded_);
        colorPicker_->setVisible(expanded_);

        expandButton_->setButtonText(expanded_ ? "-" : "...");

        // Notify parent to resize
        if (auto* parent = getParentComponent())
        {
            parent->resized();
        }
    }
}

void OscillatorPanel::notifyOscillatorChanged()
{
    if (oscillatorChangedCallback_)
    {
        Oscillator osc = getOscillator();
        oscillatorChangedCallback_(osc);
    }
}

void OscillatorPanel::handleSourceChange(const SourceId& sourceId)
{
    sourceId_ = sourceId;
    notifyOscillatorChanged();
}

void OscillatorPanel::handleProcessingModeChange()
{
    int selectedId = processingModeSelector_->getSelectedId();
    if (selectedId >= 1 && selectedId <= 6)
    {
        processingMode_ = static_cast<ProcessingMode>(selectedId - 1);
        notifyOscillatorChanged();
    }
}

void OscillatorPanel::handleColourChange(juce::Colour colour)
{
    colour_ = colour.withAlpha(1.0f);
    opacity_ = colour.getFloatAlpha();
    repaint();
    notifyOscillatorChanged();
}

void OscillatorPanel::handleVisibilityToggle()
{
    visible_ = visibilityToggle_->getToggleState();
    visibilityToggle_->setButtonText(visible_ ? "V" : "H");

    if (visibilityToggledCallback_)
    {
        visibilityToggledCallback_(oscillatorId_, visible_);
    }

    notifyOscillatorChanged();
}

void OscillatorPanel::handleDeleteClick()
{
    if (deleteRequestedCallback_)
    {
        deleteRequestedCallback_(oscillatorId_);
    }
}

void OscillatorPanel::handleExpandToggle()
{
    setExpanded(!expanded_);
}

} // namespace oscil
