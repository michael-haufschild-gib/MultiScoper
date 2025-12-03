/*
    Oscil - Oscillator Panel Implementation
    Panel for configuring individual oscillator settings
*/

#include "ui/panels/OscillatorPanel.h"
#include "ui/theme/ColorPickerComponent.h"
#include "ui/panels/SourceSelectorComponent.h"
#include "ui/theme/ThemeManager.h"
#include "ui/components/ListItemIcons.h"

namespace oscil
{

OscillatorPanel::OscillatorPanel(const Oscillator& oscillator)
    : presenter_(std::make_unique<OscillatorPresenter>(oscillator))
{
    // Header controls
    nameLabel_ = std::make_unique<juce::Label>("name", presenter_->getName().isEmpty() ? "Oscillator" : presenter_->getName());
    nameLabel_->setEditable(true);
    nameLabel_->onTextChange = [this]()
    {
        presenter_->setName(nameLabel_->getText());
    };
    addAndMakeVisible(*nameLabel_);

    visibilityToggle_ = std::make_unique<OscilToggle>();
    visibilityToggle_->setValue(presenter_->isVisible(), false);
    visibilityToggle_->onValueChanged = [this](bool /*value*/) { handleVisibilityToggle(); };
    addAndMakeVisible(*visibilityToggle_);

    expandButton_ = std::make_unique<OscilButton>("...");
    expandButton_->setVariant(ButtonVariant::Ghost);
    expandButton_->onClick = [this]() { handleExpandToggle(); };
    addAndMakeVisible(*expandButton_);

    deleteButton_ = std::make_unique<OscilButton>("");
    deleteButton_->setVariant(ButtonVariant::Danger);
    deleteButton_->setIconPath(ListItemIcons::createTrashIcon(20.0f));
    deleteButton_->setTooltip("Delete Oscillator");
    deleteButton_->onClick = [this]() { handleDeleteClick(); };
    addAndMakeVisible(*deleteButton_);

    // Expanded controls (initially hidden)
    sourceLabel_ = std::make_unique<juce::Label>("", "Source:");
    addChildComponent(*sourceLabel_);

    sourceSelector_ = std::make_unique<SourceSelectorComponent>();
    sourceSelector_->setSelectedSourceId(presenter_->getSourceId());
    sourceSelector_->onSelectionChanged([this](const SourceId& sid) { handleSourceChange(sid); });
    addChildComponent(*sourceSelector_);

    modeLabel_ = std::make_unique<juce::Label>("", "Mode:");
    addChildComponent(*modeLabel_);

    processingModeSelector_ = std::make_unique<OscilDropdown>();
    processingModeSelector_->addItem("Full Stereo");
    processingModeSelector_->addItem("Mono");
    processingModeSelector_->addItem("Mid");
    processingModeSelector_->addItem("Side");
    processingModeSelector_->addItem("Left");
    processingModeSelector_->addItem("Right");
    // ProcessingMode enum is 0-based, OscilDropdown is also 0-based
    processingModeSelector_->setSelectedIndex(static_cast<int>(presenter_->getProcessingMode()), false);
    processingModeSelector_->onSelectionChanged = [this](int /*index*/) { handleProcessingModeChange(); };
    addChildComponent(*processingModeSelector_);

    colourLabel_ = std::make_unique<juce::Label>("", "Colour:");
    addChildComponent(*colourLabel_);

    colorPicker_ = std::make_unique<ColorPickerComponent>();
    colorPicker_->setColour(presenter_->getColour().withAlpha(presenter_->getOpacity()));
    colorPicker_->onColourChanged([this](juce::Colour c) { handleColourChange(c); });
    addChildComponent(*colorPicker_);

    setupPresenterCallbacks();
}

OscillatorPanel::~OscillatorPanel() = default;

void OscillatorPanel::setupPresenterCallbacks()
{
    presenter_->setOnOscillatorChanged([this](const Oscillator& osc) {
        // Update UI elements if they differ (avoid loops)
        // But mostly we just forward the notification
        if (oscillatorChangedCallback_)
        {
            oscillatorChangedCallback_(osc);
        }
        repaint();
    });

    presenter_->setOnDeleteRequested([this](const OscillatorId& id) {
        if (deleteRequestedCallback_)
        {
            deleteRequestedCallback_(id);
        }
    });

    presenter_->setOnVisibilityToggled([this](const OscillatorId& id, bool visible) {
        if (visibilityToggledCallback_)
        {
            visibilityToggledCallback_(id, visible);
        }
    });
}

void OscillatorPanel::updateUi()
{
    if (!presenter_) return;

    auto name = presenter_->getName();
    nameLabel_->setText(name.isEmpty() ? "Oscillator" : name, juce::dontSendNotification);
    
    visibilityToggle_->setValue(presenter_->isVisible(), false);
    
    sourceSelector_->setSelectedSourceId(presenter_->getSourceId());
    
    processingModeSelector_->setSelectedIndex(static_cast<int>(presenter_->getProcessingMode()), false);
    
    colorPicker_->setColour(presenter_->getColour().withAlpha(presenter_->getOpacity()));

    repaint();
}

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
    if (presenter_)
    {
        auto colorBar = getLocalBounds().removeFromLeft(4);
        g.setColour(presenter_->getColour().withAlpha(presenter_->getOpacity()));
        g.fillRoundedRectangle(colorBar.toFloat(), 2.0f);
    }
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

OscillatorId OscillatorPanel::getOscillatorId() const
{
    return presenter_->getOscillatorId();
}

void OscillatorPanel::setOscillator(const Oscillator& oscillator)
{
    presenter_ = std::make_unique<OscillatorPresenter>(oscillator);
    setupPresenterCallbacks();
    updateUi();
}

Oscillator OscillatorPanel::getOscillator() const
{
    return presenter_->getOscillator();
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

        expandButton_->setText(expanded_ ? "-" : "...");

        // Notify parent to resize
        if (auto* parent = getParentComponent())
        {
            parent->resized();
        }
    }
}

void OscillatorPanel::handleSourceChange(const SourceId& sourceId)
{
    presenter_->setSourceId(sourceId);
}

void OscillatorPanel::handleProcessingModeChange()
{
    int selectedIndex = processingModeSelector_->getSelectedIndex();
    if (selectedIndex >= 0 && selectedIndex <= 5)
    {
        presenter_->setProcessingMode(static_cast<ProcessingMode>(selectedIndex));
    }
}

void OscillatorPanel::handleColourChange(juce::Colour colour)
{
    presenter_->setColour(colour.withAlpha(1.0f));
    presenter_->setOpacity(colour.getFloatAlpha());
    repaint();
}

void OscillatorPanel::handleVisibilityToggle()
{
    presenter_->setVisible(visibilityToggle_->getValue());
}

void OscillatorPanel::handleDeleteClick()
{
    presenter_->requestDelete();
}

void OscillatorPanel::handleExpandToggle()
{
    setExpanded(!expanded_);
}

} // namespace oscil
