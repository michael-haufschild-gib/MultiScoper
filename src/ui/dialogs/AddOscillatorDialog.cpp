/*
    Oscil - Add Oscillator Dialog Implementation
    Modal popup for creating a new oscillator with source and pane selection
*/

#include "ui/dialogs/AddOscillatorDialog.h"
#include "rendering/VisualConfiguration.h"

namespace oscil
{

AddOscillatorDialog::AddOscillatorDialog()
{
    OSCIL_REGISTER_TEST_ID("addOscillatorDialog");
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

AddOscillatorDialog::~AddOscillatorDialog()
{
    ThemeManager::getInstance().removeListener(this);
}

void AddOscillatorDialog::setupComponents()
{
    // Title label
    titleLabel_ = std::make_unique<juce::Label>("", "Add Oscillator");
    titleLabel_->setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(*titleLabel_);

    // Close button (X) - use unicode × for cleaner look, Secondary variant for visibility
    closeButton_ = std::make_unique<OscilButton>(juce::CharPointer_UTF8("\xc3\x97"), "addOscillatorDialog_closeBtn");
    closeButton_->setVariant(ButtonVariant::Secondary);
    closeButton_->onClick = [this]() { handleCloseClick(); };
    addAndMakeVisible(*closeButton_);

    // Source section
    sourceLabel_ = std::make_unique<juce::Label>("", "Source");
    addAndMakeVisible(*sourceLabel_);

    sourceDropdown_ = std::make_unique<OscilDropdown>("Select source...", "addOscillatorDialog_sourceDropdown");
    sourceDropdown_->onSelectionChanged = [this](int) { handleSourceChange(); };
    addAndMakeVisible(*sourceDropdown_);

    // Pane section
    paneLabel_ = std::make_unique<juce::Label>("", "Pane");
    addAndMakeVisible(*paneLabel_);

    paneDropdown_ = std::make_unique<OscilDropdown>("Select pane...", "addOscillatorDialog_paneDropdown");
    paneDropdown_->onSelectionChanged = [this](int) { handlePaneChange(); };
    addAndMakeVisible(*paneDropdown_);

    // Name section
    nameLabel_ = std::make_unique<juce::Label>("", "Name");
    addAndMakeVisible(*nameLabel_);

    nameField_ = std::make_unique<OscilTextField>(TextFieldVariant::Text, "addOscillatorDialog_nameField");
    nameField_->setPlaceholder("Oscillator name");
    addAndMakeVisible(*nameField_);

    // Color section
    colorLabel_ = std::make_unique<juce::Label>("", "Color Picker");
    addAndMakeVisible(*colorLabel_);

    colorSwatches_ = std::make_unique<OscilColorSwatches>("addOscillatorDialog_colorPicker");
    colorSwatches_->setColors(getDefaultColors());
    addAndMakeVisible(*colorSwatches_);

    // Visual Preset section
    visualPresetLabel_ = std::make_unique<juce::Label>("", "Visual Preset");
    addAndMakeVisible(*visualPresetLabel_);

    visualPresetDropdown_ = std::make_unique<OscilDropdown>("", "addOscillatorDialog_visualPresetDropdown");
    populateVisualPresetDropdown();
    addAndMakeVisible(*visualPresetDropdown_);

    // Error label (hidden by default)
    errorLabel_ = std::make_unique<juce::Label>("", "");
    errorLabel_->setVisible(false);
    addAndMakeVisible(*errorLabel_);

    // OK button (Primary)
    okButton_ = std::make_unique<OscilButton>("OK", "addOscillatorDialog_okBtn");
    okButton_->setVariant(ButtonVariant::Primary);
    okButton_->onClick = [this]() { handleOkClick(); };
    addAndMakeVisible(*okButton_);

    // Cancel button (Secondary)
    cancelButton_ = std::make_unique<OscilButton>("Cancel", "addOscillatorDialog_cancelBtn");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() { handleCancelClick(); };
    addAndMakeVisible(*cancelButton_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void AddOscillatorDialog::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Semi-transparent overlay for modal effect
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRect(bounds);

    // Dialog background
    auto dialogBounds = bounds.reduced(
        (bounds.getWidth() - DIALOG_WIDTH) / 2,
        (bounds.getHeight() - DIALOG_HEIGHT) / 2
    );

    g.setColour(theme.backgroundPrimary);
    g.fillRoundedRectangle(dialogBounds, 8.0f);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(dialogBounds, 8.0f, 1.0f);

    // Header background
    auto headerBounds = dialogBounds.removeFromTop(HEADER_HEIGHT);
    g.setColour(theme.backgroundSecondary);
    g.fillRoundedRectangle(headerBounds.getX(), headerBounds.getY(),
                           headerBounds.getWidth(), headerBounds.getHeight() + 8, 8.0f);
    g.fillRect(headerBounds.getX(), headerBounds.getY() + 8,
               headerBounds.getWidth(), headerBounds.getHeight() - 8);
}

bool AddOscillatorDialog::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        handleCancelClick();
        return true;
    }
    return false;
}

void AddOscillatorDialog::resized()
{
    auto bounds = getLocalBounds();

    // Center the dialog content
    auto dialogBounds = bounds.reduced(
        (bounds.getWidth() - DIALOG_WIDTH) / 2,
        (bounds.getHeight() - DIALOG_HEIGHT) / 2
    );

    auto contentBounds = dialogBounds.reduced(PADDING);

    // Header row
    auto headerRow = contentBounds.removeFromTop(HEADER_HEIGHT - PADDING);
    closeButton_->setBounds(headerRow.removeFromRight(28).reduced(2));
    titleLabel_->setBounds(headerRow);

    contentBounds.removeFromTop(SECTION_SPACING);

    // Source section
    sourceLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(4);
    sourceDropdown_->setBounds(contentBounds.removeFromTop(CONTROL_HEIGHT));
    contentBounds.removeFromTop(SECTION_SPACING);

    // Pane section
    paneLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(4);
    paneDropdown_->setBounds(contentBounds.removeFromTop(CONTROL_HEIGHT));
    contentBounds.removeFromTop(SECTION_SPACING);

    // Name section
    nameLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(4);
    nameField_->setBounds(contentBounds.removeFromTop(CONTROL_HEIGHT));
    contentBounds.removeFromTop(SECTION_SPACING);

    // Color section
    colorLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(4);
    colorSwatches_->setBounds(contentBounds.removeFromTop(COLOR_PICKER_HEIGHT));
    contentBounds.removeFromTop(SECTION_SPACING);

    // Visual Preset section
    visualPresetLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(4);
    visualPresetDropdown_->setBounds(contentBounds.removeFromTop(CONTROL_HEIGHT));
    contentBounds.removeFromTop(8);

    // Error label
    errorLabel_->setBounds(contentBounds.removeFromTop(LABEL_HEIGHT));
    contentBounds.removeFromTop(8);

    // Footer buttons at bottom
    auto footerRow = contentBounds.removeFromBottom(BUTTON_HEIGHT);
    int buttonWidth = (footerRow.getWidth() - 8) / 2;
    cancelButton_->setBounds(footerRow.removeFromLeft(buttonWidth).reduced(0, 2));
    footerRow.removeFromLeft(8);
    okButton_->setBounds(footerRow.reduced(0, 2));
}

void AddOscillatorDialog::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;

    auto styleLabel = [&newTheme](juce::Label* label, bool isTitle = false) {
        if (isTitle)
        {
            label->setColour(juce::Label::textColourId, newTheme.textPrimary);
            label->setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        }
        else
        {
            label->setColour(juce::Label::textColourId, newTheme.textSecondary);
            label->setFont(juce::FontOptions(12.0f));
        }
    };

    styleLabel(titleLabel_.get(), true);
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

void AddOscillatorDialog::showDialog(juce::Component* /*parent*/,
                                      const std::vector<SourceInfo>& sources,
                                      const std::vector<Pane>& panes,
                                      Callback onComplete)
{
    sources_ = sources;
    panes_ = panes;
    callback_ = std::move(onComplete);

    // Populate dropdowns
    populateSourceDropdown();
    populatePaneDropdown();
    // Visual presets are static, populated in setup

    // Clear previous state
    nameField_->setText("", false);
    clearError();

    // Select a random color
    selectRandomColor();

    setVisible(true);
    toFront(true);
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

void AddOscillatorDialog::hideDialog()
{
    setVisible(false);
    callback_ = nullptr;
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

void AddOscillatorDialog::populatePaneDropdown()
{
    paneDropdown_->clearItems();

    // First option is always "New pane"
    paneDropdown_->addItem("New pane", "new_pane");

    // Add existing panes
    for (const auto& pane : panes_)
    {
        paneDropdown_->addItem(pane.getName(), pane.getId().id);
    }

    // Default to "New pane" selected
    paneDropdown_->setSelectedIndex(NEW_PANE_OPTION_INDEX, false);
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

void AddOscillatorDialog::handlePaneChange()
{
    clearError();
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

    // Get selected pane
    int paneIndex = paneDropdown_->getSelectedIndex();
    if (paneIndex == NEW_PANE_OPTION_INDEX)
    {
        result.createNewPane = true;
        result.paneId = PaneId{};  // Empty, will create new
    }
    else
    {
        result.createNewPane = false;
        // Adjust for "New pane" option at index 0
        size_t actualPaneIndex = static_cast<size_t>(paneIndex - 1);
        if (actualPaneIndex < panes_.size())
        {
            result.paneId = panes_[actualPaneIndex].getId();
        }
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

    // Call callback and hide
    if (callback_)
    {
        callback_(result);
    }
    hideDialog();
}

void AddOscillatorDialog::handleCancelClick()
{
    hideDialog();
}

void AddOscillatorDialog::handleCloseClick()
{
    hideDialog();
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

    // Check pane selection (pane dropdown always has at least "New pane")
    int paneIndex = paneDropdown_->getSelectedIndex();
    if (paneIndex < 0)
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
