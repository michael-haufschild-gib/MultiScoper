/*
    Oscil - Oscillator Configuration Dialog Implementation
    Modal dialog for comprehensive oscillator configuration
*/

#include "ui/dialogs/OscillatorConfigDialog.h"

#include "core/OscilState.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/panels/SourceSelectorComponent.h"
#include "ui/theme/IThemeService.h"

#include "rendering/VisualConfiguration.h"

namespace oscil
{

OscillatorConfigDialog::OscillatorConfigDialog(IThemeService& themeService, IInstanceRegistry& instanceRegistry)
    : themeService_(themeService)
    , instanceRegistry_(instanceRegistry)
{
    OSCIL_REGISTER_TEST_ID("configPopup");
    setupComponents();
    themeService_.addListener(this);
}

OscillatorConfigDialog::~OscillatorConfigDialog()
{
    OSCIL_UNREGISTER_CHILD_TEST_ID("configPopup_sourceDropdown");
    OSCIL_UNREGISTER_CHILD_TEST_ID("configPopup_modeSelector");
    themeService_.removeListener(this);
}

void OscillatorConfigDialog::setupSourceAndMode()
{
    nameEditor_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Text, "configPopup_nameField");
    nameEditor_->setPlaceholder("Oscillator Name");
    nameEditor_->onReturnPressed = [this]() { handleNameEdit(); };
    nameEditor_->onTextChanged = [this](const juce::String&) { handleNameEdit(); };
    addAndMakeVisible(*nameEditor_);

    sourceLabel_ = std::make_unique<juce::Label>("", "Source");
    addAndMakeVisible(*sourceLabel_);

    sourceSelector_ = std::make_unique<SourceSelectorComponent>(themeService_, instanceRegistry_);
    sourceSelector_->onSelectionChanged([this](const SourceId& id) { handleSourceChange(id); });
    addAndMakeVisible(*sourceSelector_);
    OSCIL_REGISTER_CHILD_TEST_ID(*sourceSelector_, "configPopup_sourceDropdown");

    modeLabel_ = std::make_unique<juce::Label>("", "Processing Mode");
    addAndMakeVisible(*modeLabel_);

    modeButtons_ = std::make_unique<SegmentedButtonBar>(themeService_);
    modeButtons_->setMinButtonWidth(40);
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(16),
                                    static_cast<int>(ProcessingMode::FullStereo), "configPopup_modeSelector_stereo");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(16), static_cast<int>(ProcessingMode::Mono),
                                    "configPopup_modeSelector_mono");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMidIcon(16), static_cast<int>(ProcessingMode::Mid),
                                    "configPopup_modeSelector_mid");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createSideIcon(16), static_cast<int>(ProcessingMode::Side),
                                    "configPopup_modeSelector_side");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createLeftIcon(16), static_cast<int>(ProcessingMode::Left),
                                    "configPopup_modeSelector_left");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createRightIcon(16), static_cast<int>(ProcessingMode::Right),
                                    "configPopup_modeSelector_right");
    modeButtons_->onSelectionChanged = [this](int id) { handleProcessingModeChange(id); };
    addAndMakeVisible(*modeButtons_);
    OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, "configPopup_modeSelector");
}

void OscillatorConfigDialog::setupAppearanceControls()
{
    colorLabel_ = std::make_unique<juce::Label>("", "Color");
    addAndMakeVisible(*colorLabel_);

    colorSwatches_ = std::make_unique<OscilColorSwatches>(themeService_, "configPopup_colorPicker");
    colorSwatches_->setColors(getDefaultColors());
    colorSwatches_->onColorSelected = [this](int, juce::Colour color) { handleColorSelect(color); };
    addAndMakeVisible(*colorSwatches_);

    visualPresetLabel_ = std::make_unique<juce::Label>("", "Visual Preset");
    addAndMakeVisible(*visualPresetLabel_);

    visualPresetDropdown_ = std::make_unique<OscilDropdown>(themeService_, "", "configPopup_visualPresetDropdown");
    for (const auto& preset : VisualConfiguration::getAvailablePresets())
        visualPresetDropdown_->addItem(preset.second, preset.first);
    visualPresetDropdown_->onSelectionChanged = [this](int) { handleVisualPresetChange(); };
    addAndMakeVisible(*visualPresetDropdown_);

    lineWidthSlider_ = std::make_unique<OscilSlider>(themeService_, "configPopup_lineWidthSlider");
    lineWidthSlider_->setLabel("Line Width");
    lineWidthSlider_->setRange(Oscillator::MIN_LINE_WIDTH, Oscillator::MAX_LINE_WIDTH);
    lineWidthSlider_->setStep(0.1);
    lineWidthSlider_->setValue(Oscillator::DEFAULT_LINE_WIDTH, false);
    lineWidthSlider_->setSuffix(" px");
    lineWidthSlider_->onValueChanged = [this](double) { handleLineWidthChange(); };
    addAndMakeVisible(*lineWidthSlider_);

    opacitySlider_ = std::make_unique<OscilSlider>(themeService_, "configPopup_opacitySlider");
    opacitySlider_->setLabel("Opacity");
    opacitySlider_->setRange(0.0, 100.0);
    opacitySlider_->setStep(1.0);
    opacitySlider_->setValue(100.0, false);
    opacitySlider_->setSuffix(" %");
    opacitySlider_->onValueChanged = [this](double) { handleOpacityChange(); };
    addAndMakeVisible(*opacitySlider_);
}

void OscillatorConfigDialog::setupPaneAndFooter()
{
    paneLabel_ = std::make_unique<juce::Label>("", "Pane");
    addAndMakeVisible(*paneLabel_);

    paneSelectorComponent_ = std::make_unique<PaneSelectorComponent>(themeService_, false, "configPopup_paneSelector");
    paneSelectorComponent_->onSelectionChanged = [this](const PaneId&, bool) { handlePaneChange(); };
    addAndMakeVisible(*paneSelectorComponent_);

    footerCloseButton_ = std::make_unique<OscilButton>(themeService_, "Close", "configPopup_closeBtn");
    footerCloseButton_->setVariant(ButtonVariant::Secondary);
    footerCloseButton_->onClick = [this]() { handleClose(); };
    addAndMakeVisible(*footerCloseButton_);
}

void OscillatorConfigDialog::setupComponents()
{
    setupSourceAndMode();
    setupAppearanceControls();
    setupPaneAndFooter();
    setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
    themeChanged(themeService_.getCurrentTheme());
}

void OscillatorConfigDialog::paint(juce::Graphics&)
{
    // No custom painting - OscilModal handles the window frame
}

void OscillatorConfigDialog::resized()
{
    auto bounds = getLocalBounds().reduced(PADDING);

    // Name editor at top
    nameEditor_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SPACING_LARGE);

    // Source section
    sourceLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    sourceSelector_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SPACING_LARGE);

    // Processing mode section
    modeLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    modeButtons_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SPACING_LARGE);

    // Color section
    colorLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    colorSwatches_->setBounds(bounds.removeFromTop(COLOR_PICKER_HEIGHT));
    bounds.removeFromTop(SPACING_LARGE);

    // Visual preset dropdown
    visualPresetLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    visualPresetDropdown_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SPACING_MEDIUM);

    // Sliders
    lineWidthSlider_->setBounds(bounds.removeFromTop(SLIDER_ROW_HEIGHT));
    bounds.removeFromTop(SPACING_SMALL);

    opacitySlider_->setBounds(bounds.removeFromTop(SLIDER_ROW_HEIGHT));
    bounds.removeFromTop(SPACING_MEDIUM);

    // Pane selector
    paneLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    paneSelectorComponent_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SPACING_SECTION);

    // Footer: Close button (centered or right aligned)
    auto footerRow = bounds.removeFromBottom(FOOTER_HEIGHT);
    footerCloseButton_->setBounds(footerRow.withSizeKeepingCentre(100, FOOTER_HEIGHT));
}

void OscillatorConfigDialog::themeChanged(const ColorTheme& newTheme)
{
    auto styleLabel = [&newTheme](juce::Label* label) {
        label->setColour(juce::Label::textColourId, newTheme.textSecondary);
        label->setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL));
    };

    styleLabel(sourceLabel_.get());
    styleLabel(modeLabel_.get());
    styleLabel(colorLabel_.get());
    styleLabel(visualPresetLabel_.get());
    styleLabel(paneLabel_.get());

    repaint();
}

void OscillatorConfigDialog::showForOscillator(const Oscillator& oscillator)
{
    updateFromOscillator(oscillator);
    nameEditor_->grabKeyboardFocus();
}

void OscillatorConfigDialog::updateFromOscillator(const Oscillator& oscillator)
{
    oscillatorId_ = oscillator.getId();
    sourceId_ = oscillator.getSourceId();
    processingMode_ = oscillator.getProcessingMode();
    colour_ = oscillator.getColour();
    opacity_ = oscillator.getOpacity();
    visible_ = oscillator.isVisible();
    name_ = oscillator.getName();
    lineWidth_ = oscillator.getLineWidth();
    paneId_ = oscillator.getPaneId();
    orderIndex_ = oscillator.getOrderIndex();
    visualPresetId_ = oscillator.getVisualPresetId();
    visualOverrides_ = oscillator.getVisualOverrides().createCopy();

    // Load base config from preset
    VisualConfiguration config = VisualConfiguration::getPreset(visualPresetId_);

    // Update controls
    nameEditor_->setText(name_, false);
    sourceSelector_->setSelectedSourceId(sourceId_);
    modeButtons_->setSelectedId(static_cast<int>(processingMode_));
    lineWidthSlider_->setValue(lineWidth_, false);
    opacitySlider_->setValue(opacity_ * 100.0, false);

    colorSwatches_->setSelectedColor(colour_);

    auto availablePresets = VisualConfiguration::getAvailablePresets();
    for (size_t i = 0; i < availablePresets.size(); ++i)
    {
        if (availablePresets[i].first == visualPresetId_)
        {
            visualPresetDropdown_->setSelectedIndex(static_cast<int>(i), false);
            break;
        }
    }

    paneSelectorComponent_->setSelectedPaneId(paneId_, false);

    repaint();
}

void OscillatorConfigDialog::setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes)
{
    paneSelectorComponent_->setAvailablePanes(panes);
}

void OscillatorConfigDialog::notifyConfigChanged()
{
    juce::ValueTree state("Oscillator");
    state.setProperty("id", oscillatorId_.id, nullptr);
    state.setProperty("sourceId", sourceId_.id, nullptr);
    state.setProperty("oscillatorState", sourceId_.isValid() ? 0 : 1, nullptr);
    state.setProperty("processingMode", processingModeToString(processingMode_), nullptr);
    state.setProperty("colour", static_cast<int>(colour_.getARGB()), nullptr);
    state.setProperty("opacity", opacity_, nullptr);
    state.setProperty("visible", visible_, nullptr);
    state.setProperty("name", name_, nullptr);
    state.setProperty("lineWidth", lineWidth_, nullptr);
    state.setProperty("paneId", paneId_.id, nullptr);
    state.setProperty("order", orderIndex_, nullptr);
    state.setProperty(StateIds::VisualPresetId, visualPresetId_, nullptr);
    state.setProperty(StateIds::SchemaVersion, Oscillator::CURRENT_SCHEMA_VERSION, nullptr);

    if (visualOverrides_.isValid() && visualOverrides_.getNumProperties() > 0)
    {
        state.addChild(visualOverrides_.createCopy(), -1, nullptr);
    }

    Oscillator updated(state);

    listeners_.call([this, &updated](Listener& l) { l.oscillatorConfigChanged(oscillatorId_, updated); });
}

void OscillatorConfigDialog::handleClose()
{
    if (onClose)
        onClose();
    listeners_.call([](Listener& l) { l.configDialogClosed(); });
}

void OscillatorConfigDialog::handleNameEdit()
{
    auto newName = nameEditor_->getText();
    if (Oscillator::isValidName(newName) && newName != name_)
    {
        name_ = newName;
        notifyConfigChanged();
    }
}

void OscillatorConfigDialog::handleSourceChange(const SourceId& sourceId)
{
    sourceId_ = sourceId;
    notifyConfigChanged();
}

void OscillatorConfigDialog::handleProcessingModeChange(int modeId)
{
    if (modeId < 0 || modeId > static_cast<int>(ProcessingMode::Right))
        return;
    processingMode_ = static_cast<ProcessingMode>(modeId);
    notifyConfigChanged();
}

void OscillatorConfigDialog::handleColorSelect(juce::Colour colour)
{
    colour_ = colour;
    repaint();
    notifyConfigChanged();
}

void OscillatorConfigDialog::handleLineWidthChange()
{
    lineWidth_ = static_cast<float>(lineWidthSlider_->getValue());
    notifyConfigChanged();
}

void OscillatorConfigDialog::handleOpacityChange()
{
    opacity_ = static_cast<float>(opacitySlider_->getValue()) / 100.0f;
    notifyConfigChanged();
}

void OscillatorConfigDialog::handlePaneChange()
{
    PaneId selectedPaneId = paneSelectorComponent_->getSelectedPaneId();
    if (selectedPaneId.isValid() && selectedPaneId != paneId_)
    {
        paneId_ = selectedPaneId;
        notifyConfigChanged();
    }
}

void OscillatorConfigDialog::handleVisualPresetChange()
{
    int index = visualPresetDropdown_->getSelectedIndex();
    auto availablePresets = VisualConfiguration::getAvailablePresets();
    if (index >= 0 && static_cast<size_t>(index) < availablePresets.size())
    {
        visualPresetId_ = availablePresets[static_cast<size_t>(index)].first;
        visualOverrides_.removeAllProperties(nullptr);
        notifyConfigChanged();
    }
}

void OscillatorConfigDialog::addListener(Listener* listener) { listeners_.add(listener); }

void OscillatorConfigDialog::removeListener(Listener* listener) { listeners_.remove(listener); }

} // namespace oscil
