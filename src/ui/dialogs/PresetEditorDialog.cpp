/*
    Oscil - Preset Editor Dialog Implementation
*/

#include "ui/dialogs/PresetEditorDialog.h"

namespace oscil
{

PresetEditorDialog::PresetEditorDialog(IThemeService& themeService, VisualPresetManager& presetManager)
    : presetManager_(presetManager)
    , themeService_(themeService)
{
    themeService_.addListener(this);
    theme_ = themeService_.getCurrentTheme();

    OSCIL_REGISTER_TEST_ID("preset_editor_dialog");

    setupComponents();
}

PresetEditorDialog::~PresetEditorDialog()
{
    // Clear viewport's viewed component before tabContent_ is destroyed
    // to prevent dangling pointer access during component destruction
    if (tabViewport_)
        tabViewport_->setViewedComponent(nullptr, false);

    themeService_.removeListener(this);
}

void PresetEditorDialog::setupComponents()
{
    setupHeaderSection();
    setupPreviewPanel();
    setupTabs();
    setupFooterButtons();
}

void PresetEditorDialog::setupHeaderSection()
{
    // Name field
    nameLabel_ = std::make_unique<juce::Label>("", "Name:");
    nameLabel_->setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(*nameLabel_);

    nameField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Text, "preset_editor_name_field");
    nameField_->setPlaceholder("Enter preset name...");
    nameField_->onTextChanged = [this](const juce::String&) { markChanged(); };
    addAndMakeVisible(*nameField_);

    // Description field
    descriptionLabel_ = std::make_unique<juce::Label>("", "Description:");
    descriptionLabel_->setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(*descriptionLabel_);

    descriptionField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Text, "preset_editor_description_field");
    descriptionField_->setPlaceholder("Optional description...");
    descriptionField_->onTextChanged = [this](const juce::String&) { markChanged(); };
    addAndMakeVisible(*descriptionField_);

    // Favorite toggle
    favoriteToggle_ = std::make_unique<OscilToggle>(themeService_, "Favorite", "preset_editor_favorite_toggle");
    favoriteToggle_->onValueChanged = [this](bool) { markChanged(); };
    addAndMakeVisible(*favoriteToggle_);

    // Error label
    errorLabel_ = std::make_unique<juce::Label>();
    errorLabel_->setFont(juce::FontOptions(12.0f));
    errorLabel_->setColour(juce::Label::textColourId, juce::Colours::red);
    addAndMakeVisible(*errorLabel_);
}

void PresetEditorDialog::setupPreviewPanel()
{
    previewPanel_ = std::make_unique<PresetPreviewPanel>(themeService_, "preset_editor_preview");
    addAndMakeVisible(*previewPanel_);
}

void PresetEditorDialog::setupTabs()
{
    tabs_ = std::make_unique<OscilTabs>(themeService_, OscilTabs::Orientation::Horizontal, "preset_editor_tabs");
    tabs_->addTab("Shader", "shader");
    tabs_->addTab("Effects", "effects");
    tabs_->addTab("3D/Camera", "3d");
    tabs_->addTab("Materials", "materials");
    tabs_->onTabChanged = [this](int index) {
        currentTabIndex_ = index;
        resized(); // Re-layout for different tab content
    };
    addAndMakeVisible(*tabs_);

    // Tab content viewport
    tabContent_ = std::make_unique<juce::Component>();
    tabViewport_ = std::make_unique<juce::Viewport>();
    tabViewport_->setViewedComponent(tabContent_.get(), false);
    tabViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*tabViewport_);

    // Setup all tab contents
    setupShaderTab();
    setupEffectsTab();
    setup3DTab();
    setupMaterialsTab();
}

void PresetEditorDialog::setupShaderTab()
{
    shaderTab_ = std::make_unique<ShaderSettingsTab>(themeService_);
    shaderTab_->onConfigChanged = [this]() {
        markChanged();
        updateConfiguration();
        updatePreview();
    };
    tabContent_->addAndMakeVisible(*shaderTab_);
}

void PresetEditorDialog::setupEffectsTab()
{
    effectsTab_ = std::make_unique<EffectsTab>(themeService_);
    effectsTab_->onConfigChanged = [this]() {
        markChanged();
        updateConfiguration();
        updatePreview();
    };
    tabContent_->addAndMakeVisible(*effectsTab_);
}

void PresetEditorDialog::setup3DTab()
{
    settings3DTab_ = std::make_unique<Settings3DTab>(themeService_);
    settings3DTab_->onConfigChanged = [this]() {
        markChanged();
        updateConfiguration();
        updatePreview();
    };
    tabContent_->addAndMakeVisible(*settings3DTab_);
}

void PresetEditorDialog::setupMaterialsTab()
{
    materialsTab_ = std::make_unique<MaterialsTab>(themeService_);
    materialsTab_->onConfigChanged = [this]() {
        markChanged();
        updateConfiguration();
        updatePreview();
    };
    tabContent_->addAndMakeVisible(*materialsTab_);
}

void PresetEditorDialog::setupFooterButtons()
{
    resetButton_ = std::make_unique<OscilButton>(themeService_, "Reset to Default", "preset_editor_reset_button");
    resetButton_->setVariant(ButtonVariant::Ghost);
    resetButton_->onClick = [this]() { handleResetClick(); };
    addAndMakeVisible(*resetButton_);

    cancelButton_ = std::make_unique<OscilButton>(themeService_, "Cancel", "preset_editor_cancel_button");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() { handleCancelClick(); };
    addAndMakeVisible(*cancelButton_);

    saveButton_ = std::make_unique<OscilButton>(themeService_, "Save Preset", "preset_editor_save_button");
    saveButton_->setVariant(ButtonVariant::Primary);
    saveButton_->onClick = [this]() { handleSaveClick(); };
    addAndMakeVisible(*saveButton_);
}

void PresetEditorDialog::setDropdownByItemId(OscilDropdown* dropdown, const juce::String& id)
{
    if (!dropdown)
        return;

    for (int i = 0; i < dropdown->getNumItems(); ++i)
    {
        if (dropdown->getItem(i).id == id)
        {
            dropdown->setSelectedIndex(i, false);
            return;
        }
    }
}

void PresetEditorDialog::paint(juce::Graphics& g)
{
    g.setColour(theme_.backgroundPrimary);
    g.fillAll();

    // Preview panel border
    auto previewBounds = previewPanel_->getBounds().expanded(1);
    g.setColour(theme_.controlBorder);
    g.drawRect(previewBounds, 1);
}

void PresetEditorDialog::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(PADDING, PADDING);

    // Header section
    auto header = bounds.removeFromTop(HEADER_HEIGHT);

    // Name row
    auto nameRow = header.removeFromTop(CONTROL_HEIGHT);
    nameLabel_->setBounds(nameRow.removeFromLeft(80));
    favoriteToggle_->setBounds(nameRow.removeFromRight(100));
    nameRow.removeFromRight(8);
    nameField_->setBounds(nameRow);

    header.removeFromTop(8);

    // Description row
    auto descRow = header.removeFromTop(CONTROL_HEIGHT);
    descriptionLabel_->setBounds(descRow.removeFromLeft(80));
    descriptionField_->setBounds(descRow);

    header.removeFromTop(4);
    errorLabel_->setBounds(header.removeFromTop(16));

    bounds.removeFromTop(8);

    // Main content area - split into preview (left) and tabs (right)
    auto contentArea = bounds.removeFromTop(bounds.getHeight() - FOOTER_HEIGHT - 8);

    // Preview panel on left
    auto previewArea = contentArea.removeFromLeft(PREVIEW_WIDTH);
    previewPanel_->setBounds(previewArea.removeFromTop(PREVIEW_HEIGHT));

    contentArea.removeFromLeft(PADDING);

    // Tabs on right
    tabs_->setBounds(contentArea.removeFromTop(TAB_HEIGHT));
    contentArea.removeFromTop(4);
    tabViewport_->setBounds(contentArea);

    // Layout tab content based on current tab
    layoutTabContent();

    // Footer
    bounds.removeFromTop(8);
    auto footer = bounds;
    resetButton_->setBounds(footer.removeFromLeft(140).reduced(0, 4));

    int buttonWidth = 100;
    saveButton_->setBounds(footer.removeFromRight(buttonWidth).reduced(0, 4));
    footer.removeFromRight(8);
    cancelButton_->setBounds(footer.removeFromRight(buttonWidth).reduced(0, 4));
}

void PresetEditorDialog::layoutTabContent()
{
    auto contentWidth = tabViewport_->getWidth() - tabViewport_->getScrollBarThickness();

    // Hide all tab containers first
    if (shaderTab_) shaderTab_->setVisible(currentTabIndex_ == 0);
    if (effectsTab_) effectsTab_->setVisible(currentTabIndex_ == 1);
    if (settings3DTab_) settings3DTab_->setVisible(currentTabIndex_ == 2);
    if (materialsTab_) materialsTab_->setVisible(currentTabIndex_ == 3);

    if (currentTabIndex_ == 0)
    {
        // Shader tab layout
        if (shaderTab_)
        {
            shaderTab_->setBounds(0, 0, contentWidth, shaderTab_->getPreferredHeight());
            tabContent_->setSize(contentWidth, shaderTab_->getPreferredHeight() + 20);
        }
    }
    else if (currentTabIndex_ == 1)
    {
        // Effects tab
        if (effectsTab_)
        {
            effectsTab_->setBounds(0, 0, contentWidth, effectsTab_->getPreferredHeight());
            tabContent_->setSize(contentWidth, effectsTab_->getPreferredHeight() + 20);
        }
    }
    else if (currentTabIndex_ == 2)
    {
        // 3D tab
        if (settings3DTab_)
        {
            settings3DTab_->setBounds(0, 0, contentWidth, settings3DTab_->getPreferredHeight());
            tabContent_->setSize(contentWidth, settings3DTab_->getPreferredHeight() + 20);
        }
    }
    else if (currentTabIndex_ == 3)
    {
        // Materials tab
        if (materialsTab_)
        {
            materialsTab_->setBounds(0, 0, contentWidth, materialsTab_->getPreferredHeight());
            tabContent_->setSize(contentWidth, materialsTab_->getPreferredHeight() + 20);
        }
    }
}

void PresetEditorDialog::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    nameLabel_->setColour(juce::Label::textColourId, theme_.textPrimary);
    descriptionLabel_->setColour(juce::Label::textColourId, theme_.textPrimary);
    repaint();
}

void PresetEditorDialog::setNewPreset()
{
    isNewPreset_ = true;
    hasChanges_ = false;
    originalPreset_ = VisualPreset();
    originalPreset_.id = VisualPreset::generateId();
    originalPreset_.name = "";
    originalPreset_.description = "";
    originalPreset_.category = PresetCategory::User;
    originalPreset_.createdAt = VisualPreset::now();
    originalPreset_.modifiedAt = originalPreset_.createdAt;
    workingConfig_ = VisualConfiguration::getDefault();

    nameField_->setText("");
    descriptionField_->setText("");
    favoriteToggle_->setValue(false);

    updateFromConfiguration();
    updatePreview();
    clearError();
}

void PresetEditorDialog::setPreset(const VisualPreset& preset)
{
    isNewPreset_ = false;
    hasChanges_ = false;
    originalPreset_ = preset;
    workingConfig_ = preset.configuration;

    nameField_->setText(preset.name);
    descriptionField_->setText(preset.description);
    favoriteToggle_->setValue(preset.isFavorite);

    // Disable editing for system presets
    bool editable = !preset.isReadOnly;
    nameField_->setEnabled(editable);
    descriptionField_->setEnabled(editable);
    saveButton_->setEnabled(editable);

    updateFromConfiguration();
    updatePreview();
    clearError();
}

VisualPreset PresetEditorDialog::getPreset() const
{
    VisualPreset preset = originalPreset_;
    preset.name = nameField_->getText();
    preset.description = descriptionField_->getText();
    preset.isFavorite = favoriteToggle_->getValue();
    preset.configuration = workingConfig_;
    preset.modifiedAt = VisualPreset::now();
    return preset;
}

void PresetEditorDialog::updateFromConfiguration()
{
    // Update all UI controls from workingConfig_
    // Shader
    if (shaderTab_)
        shaderTab_->setConfiguration(workingConfig_);

    if (effectsTab_)
        effectsTab_->setConfiguration(workingConfig_);

    if (settings3DTab_)
        settings3DTab_->setConfiguration(workingConfig_);

    if (materialsTab_)
        materialsTab_->setConfiguration(workingConfig_);
}

void PresetEditorDialog::updateConfiguration()
{
    // Update workingConfig_ from UI controls

    // Shader type
    if (shaderTab_)
        shaderTab_->updateConfiguration(workingConfig_);

    if (effectsTab_)
        effectsTab_->updateConfiguration(workingConfig_);

    if (settings3DTab_)
        settings3DTab_->updateConfiguration(workingConfig_);

    if (materialsTab_)
        materialsTab_->updateConfiguration(workingConfig_);
}

void PresetEditorDialog::markChanged()
{
    hasChanges_ = true;
}

void PresetEditorDialog::updatePreview()
{
    previewPanel_->setConfiguration(workingConfig_);
}

void PresetEditorDialog::handleSaveClick()
{
    if (!validateInput())
        return;

    auto preset = getPreset();

    if (isNewPreset_)
    {
        auto result = presetManager_.createPreset(preset.name, preset.description, preset.configuration);
        if (!result.success)
        {
            showError(result.error);
            return;
        }
        preset = result.value;
        presetManager_.setFavorite(preset.id, preset.isFavorite);
    }
    else
    {
        auto result = presetManager_.updatePreset(preset.id, preset.configuration);
        if (!result.success)
        {
            showError(result.error);
            return;
        }

        if (preset.name != originalPreset_.name)
        {
            auto renameResult = presetManager_.renamePreset(preset.id, preset.name);
            if (!renameResult.success)
            {
                showError(renameResult.error);
                return;
            }
        }

        if (preset.description != originalPreset_.description)
        {
            auto descResult = presetManager_.setDescription(preset.id, preset.description);
            if (!descResult.success)
            {
                showError(descResult.error);
                return;
            }
        }

        if (preset.isFavorite != originalPreset_.isFavorite)
        {
            auto favResult = presetManager_.setFavorite(preset.id, preset.isFavorite);
            if (!favResult.success)
            {
                showError(favResult.error);
                return;
            }
        }
    }

    hasChanges_ = false;

    if (onSave_)
        onSave_(preset);
}

void PresetEditorDialog::handleCancelClick()
{
    if (hasChanges_)
    {
        // Use SafePointer to safely reference this dialog in the callback
        // This prevents crashes if the dialog is destroyed before the callback executes
        juce::Component::SafePointer<PresetEditorDialog> safeThis(this);

        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            "Unsaved Changes",
            "You have unsaved changes. Discard them?",
            "Discard",
            "Cancel",
            this,
            juce::ModalCallbackFunction::create([safeThis](int result) {
                if (safeThis != nullptr && result == 1 && safeThis->onCancel_)
                    safeThis->onCancel_();
            })
        );
    }
    else if (onCancel_)
    {
        onCancel_();
    }
}

void PresetEditorDialog::handleResetClick()
{
    workingConfig_ = VisualConfiguration::getDefault();
    updateFromConfiguration();
    updatePreview();
    markChanged();
}

bool PresetEditorDialog::validateInput()
{
    auto name = nameField_->getText();
    auto validation = validatePresetName(name);
    if (!validation.success)
    {
        showError(validation.error);
        return false;
    }

    // Check uniqueness
    if (isNewPreset_ || name != originalPreset_.name)
    {
        if (!presetManager_.isNameUnique(name, PresetCategory::User, originalPreset_.id))
        {
            showError("A preset with this name already exists");
            return false;
        }
    }

    auto description = descriptionField_->getText();
    if (description.length() > 256)
    {
        showError("Description cannot exceed 256 characters");
        return false;
    }

    clearError();
    return true;
}

void PresetEditorDialog::showError(const juce::String& message)
{
    errorLabel_->setText(message, juce::dontSendNotification);
    errorLabel_->setColour(juce::Label::textColourId, theme_.statusError);
}

void PresetEditorDialog::clearError()
{
    errorLabel_->setText("", juce::dontSendNotification);
}

} // namespace oscil
