/*
    Oscil - Add Oscillator Dialog Implementation
    Content component for adding a new oscillator (hosted in OscilModal)
*/

#include "ui/dialogs/AddOscillatorDialog.h"
#include "rendering/VisualConfiguration.h"

namespace oscil
{

AddOscillatorDialog::AddOscillatorDialog(IThemeService& themeService)
    : themeService_(themeService)
{
    OSCIL_REGISTER_TEST_ID("addOscillatorDialog");
    setupComponents();
    themeService_.addListener(this);
}

AddOscillatorDialog::~AddOscillatorDialog()
{
    themeService_.removeListener(this);
}

void AddOscillatorDialog::setupComponents()
{
    // Source section
    sourceLabel_ = std::make_unique<juce::Label>("", "Source");
    addAndMakeVisible(*sourceLabel_);

    sourceDropdown_ = std::make_unique<OscilDropdown>(themeService_, "Select source...", "addOscillatorDialog_sourceDropdown");
    sourceDropdown_->onSelectionChanged = [this](int) { handleSourceChange(); };
    addAndMakeVisible(*sourceDropdown_);

    // Pane section
    paneLabel_ = std::make_unique<juce::Label>("", "Pane");
    addAndMakeVisible(*paneLabel_);

    paneSelector_ = std::make_unique<PaneSelectorComponent>(themeService_, true, "addOscillatorDialog_paneSelector");
    paneSelector_->onSelectionChanged = [this](const PaneId&, bool) { clearError(); };
    addAndMakeVisible(*paneSelector_);

    // Name section
    nameLabel_ = std::make_unique<juce::Label>("", "Name");
    addAndMakeVisible(*nameLabel_);

    nameField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Text, "addOscillatorDialog_nameField");
    nameField_->setPlaceholder("Oscillator name");
    addAndMakeVisible(*nameField_);

    // Color section
    colorLabel_ = std::make_unique<juce::Label>("", "Color");
    addAndMakeVisible(*colorLabel_);

    colorSwatches_ = std::make_unique<OscilColorSwatches>(themeService_, "addOscillatorDialog_colorPicker");
    colorSwatches_->setColors(getDefaultColors());
    addAndMakeVisible(*colorSwatches_);

    // Visual Preset section
    visualPresetLabel_ = std::make_unique<juce::Label>("", "Visual Preset");
    addAndMakeVisible(*visualPresetLabel_);

    visualPresetDropdown_ = std::make_unique<OscilDropdown>(themeService_, "", "addOscillatorDialog_visualPresetDropdown");
    populateVisualPresetDropdown();
    addAndMakeVisible(*visualPresetDropdown_);

    browsePresetsButton_ = std::make_unique<OscilButton>(themeService_, juce::CharPointer_UTF8("\xE2\x9A\x99"), "addOscillatorDialog_browsePresetsBtn");  // Gear symbol: ⚙
    browsePresetsButton_->setVariant(ButtonVariant::Ghost);
    browsePresetsButton_->onClick = [this]() {
        if (browsePresetsCallback_)
        {
            juce::String currentPresetId = visualPresetDropdown_->getSelectedId();
            browsePresetsCallback_(currentPresetId, [this](const juce::String& selectedPresetId) {
                // Update the dropdown with the newly selected preset
                visualPresetDropdown_->setSelectedId(selectedPresetId, false);
            });
        }
    };
    addAndMakeVisible(*browsePresetsButton_);

    // Error label (hidden by default)
    errorLabel_ = std::make_unique<juce::Label>("", "");
    errorLabel_->setVisible(false);
    addAndMakeVisible(*errorLabel_);

    // OK button (Primary)
    okButton_ = std::make_unique<OscilButton>(themeService_, "OK", "addOscillatorDialog_okBtn");
    okButton_->setVariant(ButtonVariant::Primary);
    okButton_->onClick = [this]() { handleOkClick(); };
    addAndMakeVisible(*okButton_);

    // Cancel button (Secondary)
    cancelButton_ = std::make_unique<OscilButton>(themeService_, "Cancel", "addOscillatorDialog_cancelBtn");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() { handleCancelClick(); };
    addAndMakeVisible(*cancelButton_);

    // Apply initial theme
    themeChanged(themeService_.getCurrentTheme());

    // Set size for OscilModal
    setSize(getPreferredWidth(), getPreferredHeight());
}

void AddOscillatorDialog::paint(juce::Graphics&)
{
    // No custom painting - OscilModal handles backdrop/frame
    // Child components handle their own painting
}

void AddOscillatorDialog::resized()
{
    auto bounds = getLocalBounds();

    // Source section
    sourceLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(4);
    sourceDropdown_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SECTION_SPACING);

    // Pane section
    paneLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(4);
    paneSelector_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SECTION_SPACING);

    // Name section
    nameLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(4);
    nameField_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SECTION_SPACING);

    // Color section
    colorLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(4);
    colorSwatches_->setBounds(bounds.removeFromTop(COLOR_PICKER_HEIGHT));
    bounds.removeFromTop(SECTION_SPACING);

    // Visual Preset section (dropdown + gear button)
    visualPresetLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(4);
    auto presetRow = bounds.removeFromTop(CONTROL_HEIGHT);
    int gearButtonWidth = CONTROL_HEIGHT;  // Square button
    browsePresetsButton_->setBounds(presetRow.removeFromRight(gearButtonWidth));
    presetRow.removeFromRight(4);  // Spacing between dropdown and button
    visualPresetDropdown_->setBounds(presetRow);
    bounds.removeFromTop(8);

    // Error label
    errorLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(8);

    // Footer buttons at bottom
    auto footerRow = bounds.removeFromBottom(BUTTON_HEIGHT);
    int buttonWidth = (footerRow.getWidth() - 8) / 2;
    cancelButton_->setBounds(footerRow.removeFromLeft(buttonWidth));
    footerRow.removeFromLeft(8);
    okButton_->setBounds(footerRow);
}

void AddOscillatorDialog::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;

    auto styleLabel = [&newTheme](juce::Label* label) {
        label->setColour(juce::Label::textColourId, newTheme.textSecondary);
        label->setFont(juce::FontOptions(12.0f));
    };

    styleLabel(sourceLabel_.get());
    styleLabel(paneLabel_.get());
    styleLabel(nameLabel_.get());
    styleLabel(colorLabel_.get());
    styleLabel(visualPresetLabel_.get());

    // Error label uses error color
    errorLabel_->setColour(juce::Label::textColourId, newTheme.statusError);
    errorLabel_->setFont(juce::FontOptions(12.0f));

    repaint();
}

void AddOscillatorDialog::setData(const std::vector<SourceInfo>& sources,
                                   const std::vector<Pane>& panes)
{
    sources_ = sources;
    panes_ = panes;

    // Populate dropdowns
    populateSourceDropdown();
    paneSelector_->setAvailablePanes(panes);

    // Reset state
    reset();
}

void AddOscillatorDialog::reset()
{
    nameField_->setText("", false);
    clearError();
    selectRandomColor();
    sourceDropdown_->setSelectedIndex(-1, false);
    paneSelector_->selectNewPane(false);
}

void AddOscillatorDialog::populateSourceDropdown()
{
    sourceDropdown_->clearItems();

    for (const auto& source : sources_)
    {
        juce::String label = source.name.isEmpty() ? source.sourceId.id : source.name;
        sourceDropdown_->addItem(label, source.sourceId.id);
    }

    // Don't auto-select anything - user must choose
    sourceDropdown_->setSelectedIndex(-1, false);
}

void AddOscillatorDialog::populateVisualPresetDropdown()
{
    visualPresetDropdown_->clearItems();
    auto availablePresets = VisualConfiguration::getAvailablePresets();
    for (const auto& preset : availablePresets)
    {
        visualPresetDropdown_->addItem(preset.second, preset.first);
    }
    // Default to first preset if available
    if (visualPresetDropdown_->getNumItems() > 0)
    {
        visualPresetDropdown_->setSelectedIndex(0, false);
    }
}

void AddOscillatorDialog::selectRandomColor()
{
    auto& random = juce::Random::getSystemRandom();
    int colorIndex = random.nextInt(NUM_COLOR_SWATCHES);
    colorSwatches_->setSelectedIndex(colorIndex, false);
}

void AddOscillatorDialog::updateNameFromSource()
{
    int selectedIndex = sourceDropdown_->getSelectedIndex();
    if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < sources_.size())
    {
        const auto& source = sources_[static_cast<size_t>(selectedIndex)];
        if (!source.name.isEmpty())
        {
            nameField_->setText(source.name, false);
        }
    }
}

void AddOscillatorDialog::handleSourceChange()
{
    clearError();
    updateNameFromSource();
}

void AddOscillatorDialog::handleOkClick()
{
    if (!validateInput())
    {
        return;
    }

    // Build result
    Result result;

    // Get selected source
    int sourceIndex = sourceDropdown_->getSelectedIndex();
    result.sourceId = sources_[static_cast<size_t>(sourceIndex)].sourceId;

    // Get selected pane from PaneSelectorComponent
    result.createNewPane = paneSelector_->isNewPaneSelected();
    if (result.createNewPane)
    {
        result.paneId = PaneId{};  // Empty, will create new
    }
    else
    {
        result.paneId = paneSelector_->getSelectedPaneId();
    }

    // Get name (or use source name as fallback)
    result.name = nameField_->getText();
    if (result.name.isEmpty())
    {
        const auto& source = sources_[static_cast<size_t>(sourceIndex)];
        result.name = source.name.isEmpty() ? "Oscillator" : source.name;
    }

    // Get selected color
    if (colorSwatches_->getSelectedIndex() >= 0)
    {
        result.color = colorSwatches_->getSelectedColor();
    }
    else
    {
        // Should not happen if validation passes, but safe fallback
        result.color = WaveformColorPalette::getColor(0);
    }

    // Get selected visual preset
    int presetIndex = visualPresetDropdown_->getSelectedIndex();
    auto availablePresets = VisualConfiguration::getAvailablePresets();
    if (presetIndex >= 0 && static_cast<size_t>(presetIndex) < availablePresets.size())
    {
        result.visualPresetId = availablePresets[static_cast<size_t>(presetIndex)].first;
    }
    else
    {
        // Default to basic if nothing selected
        result.visualPresetId = "default";
    }

    // Call callback
    if (callback_)
    {
        callback_(result);
    }
}

void AddOscillatorDialog::handleCancelClick()
{
    if (cancelCallback_)
    {
        cancelCallback_();
    }
}

void AddOscillatorDialog::showError(const juce::String& message)
{
    errorLabel_->setText(message, juce::dontSendNotification);
    errorLabel_->setVisible(true);
}

void AddOscillatorDialog::clearError()
{
    errorLabel_->setText("", juce::dontSendNotification);
    errorLabel_->setVisible(false);
}

bool AddOscillatorDialog::validateInput()
{
    // Check source selection
    int sourceIndex = sourceDropdown_->getSelectedIndex();
    if (sourceIndex < 0 || static_cast<size_t>(sourceIndex) >= sources_.size())
    {
        showError("Please select a source.");
        return false;
    }

    // Check pane selection
    if (!paneSelector_->hasValidSelection())
    {
        showError("Please select a pane.");
        return false;
    }

    // Check color selection
    if (colorSwatches_->getSelectedIndex() < 0)
    {
        showError("Please select a color.");
        return false;
    }

    return true;
}

} // namespace oscil
