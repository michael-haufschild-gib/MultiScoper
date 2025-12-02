/*
    Oscil - Theme Editor Component Implementation
    UI for creating and editing themes
*/

#include "ui/ThemeEditorComponent.h"
#include "ui/ThemeManager.h"
#include "ui/components/TestId.h"

namespace oscil
{

static constexpr int kLeftPanelWidth = 170;
static constexpr int kSeparatorPadding = 10;

//==============================================================================
// ColorSwatchButton
//==============================================================================

ColorSwatchButton::ColorSwatchButton(const juce::String& label, juce::Colour initialColor)
    : label_(label)
    , colour_(initialColor)
{
}

void ColorSwatchButton::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Draw label
    g.setColour(theme.textPrimary);
    g.setFont(12.0f);
    auto labelBounds = bounds.removeFromLeft(100.0f);
    g.drawText(label_, labelBounds.toNearestInt(), juce::Justification::centredRight);

    bounds.removeFromLeft(8.0f);

    // Draw color swatch
    auto swatchBounds = bounds.reduced(2.0f);

    // Checkerboard for transparency
    const int checkSize = 6;
    auto swatchRect = swatchBounds.toNearestInt();
    for (int y = swatchRect.getY(); y < swatchRect.getBottom(); y += checkSize)
    {
        for (int x = swatchRect.getX(); x < swatchRect.getRight(); x += checkSize)
        {
            bool isLight = ((x - swatchRect.getX()) / checkSize + (y - swatchRect.getY()) / checkSize) % 2 == 0;
            g.setColour(isLight ? juce::Colours::white : juce::Colours::lightgrey);
            g.fillRect(x, y, checkSize, checkSize);
        }
    }

    // Draw color
    g.setColour(colour_);
    g.fillRect(swatchBounds);

    // Draw border
    g.setColour(theme.controlBorder);
    g.drawRect(swatchBounds, 1.0f);
}

void ColorSwatchButton::resized()
{
}

void ColorSwatchButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        // Create a color picker popup using modern C++ pattern
        auto colorPicker = std::make_unique<ColorPickerComponent>();
        colorPicker->setColour(colour_);
        colorPicker->setSize(280, ColorPickerComponent::PREFERRED_HEIGHT);

        // Set up callback
        juce::Component::SafePointer<ColorSwatchButton> safeThis(this);
        colorPicker->onColourChanged([safeThis](juce::Colour newColour)
        {
            if (safeThis != nullptr)
            {
                safeThis->setColour(newColour);
            }
        });

        juce::CallOutBox::launchAsynchronously(
            std::move(colorPicker),
            getScreenBounds(),
            nullptr);
    }
}

void ColorSwatchButton::setColour(juce::Colour colour)
{
    if (colour_ != colour)
    {
        colour_ = colour;
        repaint();

        if (colourChangedCallback_)
            colourChangedCallback_(colour_);
    }
}

//==============================================================================
// ThemeColorSection
//==============================================================================

ThemeColorSection::ThemeColorSection(const juce::String& title)
    : title_(title)
{
}

void ThemeColorSection::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds();

    // Draw section header
    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(title_, bounds.removeFromTop(24), juce::Justification::centredLeft);

    // Draw separator
    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(22, 0.0f, static_cast<float>(getWidth()));
}

void ThemeColorSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(28);  // Header

    for (auto& swatch : swatches_)
    {
        swatch->setBounds(bounds.removeFromTop(ColorSwatchButton::PREFERRED_HEIGHT));
        bounds.removeFromTop(2);
    }
}

void ThemeColorSection::addColorSwatch(const juce::String& label, juce::Colour* colorRef)
{
    auto swatch = std::make_unique<ColorSwatchButton>(label, *colorRef);
    swatch->onColourChanged([this, colorRef](juce::Colour c)
    {
        *colorRef = c;
        if (colorChangedCallback_)
            colorChangedCallback_();
    });
    addAndMakeVisible(*swatch);
    swatches_.push_back(std::move(swatch));
    colorRefs_.push_back(colorRef);
}

void ThemeColorSection::updateFromTheme(ColorTheme& /*theme*/)
{
    // Update swatch colors from their refs
    for (size_t i = 0; i < swatches_.size() && i < colorRefs_.size(); ++i)
    {
        swatches_[i]->setColour(*colorRefs_[i]);
    }
}

void ThemeColorSection::setEnabled(bool enabled)
{
    enabled_ = enabled;
    for (auto& swatch : swatches_)
    {
        swatch->setEnabled(enabled);
        swatch->setAlpha(enabled ? 1.0f : 0.5f);
    }
}

int ThemeColorSection::getPreferredHeight() const
{
    return 28 + static_cast<int>(swatches_.size()) * (ColorSwatchButton::PREFERRED_HEIGHT + 2);
}

//==============================================================================
// ThemeEditorComponent
//==============================================================================

ThemeEditorComponent::ThemeEditorComponent()
{
    ThemeManager::getInstance().addListener(this);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("themeEditor");
#endif

    // Theme list
    themeList_ = std::make_unique<juce::ListBox>("Themes", this);
    themeList_->setRowHeight(24);
    addAndMakeVisible(*themeList_);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_CHILD_TEST_ID(*themeList_, "themeEditor_themeList");
#endif

    // Action buttons
    createButton_ = std::make_unique<OscilButton>("New", "themeEditor_createBtn");
    createButton_->setVariant(ButtonVariant::Primary);
    createButton_->onClick = [this]() { handleCreateTheme(); };
    addAndMakeVisible(*createButton_);

    cloneButton_ = std::make_unique<OscilButton>("Clone", "themeEditor_cloneBtn");
    cloneButton_->setVariant(ButtonVariant::Secondary);
    cloneButton_->onClick = [this]() { handleCloneTheme(); };
    addAndMakeVisible(*cloneButton_);

    deleteButton_ = std::make_unique<OscilButton>("Delete", "themeEditor_deleteBtn");
    deleteButton_->setVariant(ButtonVariant::Danger);
    deleteButton_->onClick = [this]() { handleDeleteTheme(); };
    addAndMakeVisible(*deleteButton_);

    importButton_ = std::make_unique<OscilButton>("Import", "themeEditor_importBtn");
    importButton_->setVariant(ButtonVariant::Secondary);
    importButton_->onClick = [this]() { handleImportTheme(); };
    addAndMakeVisible(*importButton_);

    exportButton_ = std::make_unique<OscilButton>("Export", "themeEditor_exportBtn");
    exportButton_->setVariant(ButtonVariant::Secondary);
    exportButton_->onClick = [this]() { handleExportTheme(); };
    addAndMakeVisible(*exportButton_);

    applyButton_ = std::make_unique<OscilButton>("Apply", "themeEditor_applyBtn");
    applyButton_->setVariant(ButtonVariant::Primary);
    applyButton_->onClick = [this]() { handleApplyTheme(); };
    addAndMakeVisible(*applyButton_);

    closeButton_ = std::make_unique<OscilButton>("Close", "themeEditor_closeBtn");
    closeButton_->setVariant(ButtonVariant::Ghost);
    closeButton_->onClick = [this]()
    {
        if (closeCallback_)
            closeCallback_();
    };
    addAndMakeVisible(*closeButton_);

    // Name editor
    nameLabel_ = std::make_unique<juce::Label>("", "Name:");
    addAndMakeVisible(*nameLabel_);

    nameEditor_ = std::make_unique<OscilTextField>("themeEditor_nameField");
    nameEditor_->setPlaceholder("Theme Name");
    addAndMakeVisible(*nameEditor_);

    // System theme indicator
    systemThemeLabel_ = std::make_unique<juce::Label>("", "(System theme - read only)");
    systemThemeLabel_->setColour(juce::Label::textColourId, juce::Colours::orange);
    addChildComponent(*systemThemeLabel_);

    // Color sections container
    colorContainer_ = std::make_unique<juce::Component>();
    colorViewport_ = std::make_unique<juce::Viewport>();
    colorViewport_->setViewedComponent(colorContainer_.get(), false);
    colorViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*colorViewport_);

    // Initialize theme list
    refreshThemeList();

    // Select current theme
    auto currentThemeName = ThemeManager::getInstance().getCurrentTheme().name;
    selectTheme(currentThemeName);
}

ThemeEditorComponent::~ThemeEditorComponent()
{
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_UNREGISTER_CHILD_TEST_ID("themeEditor_themeList");
#endif

    ThemeManager::getInstance().removeListener(this);
}

void ThemeEditorComponent::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    // Draw separator between list and editor
    g.setColour(theme.gridMajor);
    g.drawVerticalLine(kLeftPanelWidth + kSeparatorPadding, 0.0f, static_cast<float>(getHeight()));
}

void ThemeEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Left side - theme list and buttons
    auto leftPanel = bounds.removeFromLeft(kLeftPanelWidth);

    auto listButtons = leftPanel.removeFromBottom(70);
    listButtons.removeFromTop(5);

    auto buttonRow1 = listButtons.removeFromTop(25);
    createButton_->setBounds(buttonRow1.removeFromLeft(55));
    buttonRow1.removeFromLeft(2);
    cloneButton_->setBounds(buttonRow1.removeFromLeft(55));
    buttonRow1.removeFromLeft(2);
    deleteButton_->setBounds(buttonRow1);

    listButtons.removeFromTop(5);
    auto buttonRow2 = listButtons.removeFromTop(25);
    importButton_->setBounds(buttonRow2.removeFromLeft(82));
    buttonRow2.removeFromLeft(2);
    exportButton_->setBounds(buttonRow2);

    themeList_->setBounds(leftPanel);

    bounds.removeFromLeft(kSeparatorPadding * 2);  // Spacing

    // Bottom buttons
    auto bottomButtons = bounds.removeFromBottom(30);
    closeButton_->setBounds(bottomButtons.removeFromRight(80));
    bottomButtons.removeFromRight(10);
    applyButton_->setBounds(bottomButtons.removeFromRight(80));

    bounds.removeFromBottom(10);

    // Right side - theme editor
    auto nameRow = bounds.removeFromTop(25);
    nameLabel_->setBounds(nameRow.removeFromLeft(50));
    nameRow.removeFromLeft(5);

    systemThemeLabel_->setBounds(nameRow.removeFromRight(180));
    nameEditor_->setBounds(nameRow);

    bounds.removeFromTop(10);

    // Color sections in viewport
    colorViewport_->setBounds(bounds);

    // Layout color sections within container
    int contentHeight = 0;
    int sectionWidth = bounds.getWidth() - 20;  // Account for scrollbar

    if (backgroundSection_)
    {
        backgroundSection_->setBounds(0, contentHeight, sectionWidth, backgroundSection_->getPreferredHeight());
        contentHeight += backgroundSection_->getPreferredHeight() + 10;
    }
    if (gridSection_)
    {
        gridSection_->setBounds(0, contentHeight, sectionWidth, gridSection_->getPreferredHeight());
        contentHeight += gridSection_->getPreferredHeight() + 10;
    }
    if (textSection_)
    {
        textSection_->setBounds(0, contentHeight, sectionWidth, textSection_->getPreferredHeight());
        contentHeight += textSection_->getPreferredHeight() + 10;
    }
    if (controlSection_)
    {
        controlSection_->setBounds(0, contentHeight, sectionWidth, controlSection_->getPreferredHeight());
        contentHeight += controlSection_->getPreferredHeight() + 10;
    }
    if (statusSection_)
    {
        statusSection_->setBounds(0, contentHeight, sectionWidth, statusSection_->getPreferredHeight());
        contentHeight += statusSection_->getPreferredHeight();
    }

    colorContainer_->setSize(sectionWidth, contentHeight);
}

int ThemeEditorComponent::getNumRows()
{
    return static_cast<int>(themeNames_.size());
}

void ThemeEditorComponent::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                             int width, int height, bool rowIsSelected)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    if (rowIsSelected)
    {
        g.setColour(theme.controlActive);
        g.fillRect(0, 0, width, height);
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(themeNames_.size()))
    {
        g.setColour(rowIsSelected ? theme.textHighlight : theme.textPrimary);
        g.setFont(12.0f);

        auto name = themeNames_[static_cast<size_t>(rowNumber)];
        bool isSystem = ThemeManager::getInstance().isSystemTheme(name);

        juce::String displayName = name;
        if (isSystem)
            displayName += " *";

        g.drawText(displayName, 5, 0, width - 10, height, juce::Justification::centredLeft);
    }
}

void ThemeEditorComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(themeNames_.size()))
    {
        selectTheme(themeNames_[static_cast<size_t>(lastRowSelected)]);
    }
}

void ThemeEditorComponent::themeChanged(const ColorTheme& /*newTheme*/)
{
    repaint();
}

void ThemeEditorComponent::refreshThemeList()
{
    themeNames_ = ThemeManager::getInstance().getAvailableThemes();
    themeList_->updateContent();
}

void ThemeEditorComponent::selectTheme(const juce::String& name)
{
    selectedThemeName_ = name;

    // Find and select in list
    for (size_t i = 0; i < themeNames_.size(); ++i)
    {
        if (themeNames_[i] == name)
        {
            themeList_->selectRow(static_cast<int>(i));
            break;
        }
    }

    // Get theme copy for editing
    auto* sourceTheme = ThemeManager::getInstance().getTheme(name);
    if (sourceTheme)
    {
        editingTheme_ = *sourceTheme;
    }

    nameEditor_->setText(name, false);

    bool isSystem = ThemeManager::getInstance().isSystemTheme(name);
    systemThemeLabel_->setVisible(isSystem);
    nameEditor_->setEnabled(!isSystem);
    deleteButton_->setEnabled(!isSystem);

    updateColorSections();
}

void ThemeEditorComponent::updateColorSections()
{
    // Clear existing sections
    backgroundSection_.reset();
    gridSection_.reset();
    textSection_.reset();
    controlSection_.reset();
    statusSection_.reset();

    bool isEditable = !ThemeManager::getInstance().isSystemTheme(selectedThemeName_);

    // Background section
    backgroundSection_ = std::make_unique<ThemeColorSection>("Background Colors");
    backgroundSection_->addColorSwatch("Primary:", &editingTheme_.backgroundPrimary);
    backgroundSection_->addColorSwatch("Secondary:", &editingTheme_.backgroundSecondary);
    backgroundSection_->addColorSwatch("Pane:", &editingTheme_.backgroundPane);
    backgroundSection_->onColorChanged([this]() { handleColorChanged(); });
    backgroundSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*backgroundSection_);

    // Grid section
    gridSection_ = std::make_unique<ThemeColorSection>("Grid Colors");
    gridSection_->addColorSwatch("Major:", &editingTheme_.gridMajor);
    gridSection_->addColorSwatch("Minor:", &editingTheme_.gridMinor);
    gridSection_->addColorSwatch("Zero Line:", &editingTheme_.gridZeroLine);
    gridSection_->onColorChanged([this]() { handleColorChanged(); });
    gridSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*gridSection_);

    // Text section
    textSection_ = std::make_unique<ThemeColorSection>("Text Colors");
    textSection_->addColorSwatch("Primary:", &editingTheme_.textPrimary);
    textSection_->addColorSwatch("Secondary:", &editingTheme_.textSecondary);
    textSection_->addColorSwatch("Highlight:", &editingTheme_.textHighlight);
    textSection_->onColorChanged([this]() { handleColorChanged(); });
    textSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*textSection_);

    // Control section
    controlSection_ = std::make_unique<ThemeColorSection>("Control Colors");
    controlSection_->addColorSwatch("Background:", &editingTheme_.controlBackground);
    controlSection_->addColorSwatch("Border:", &editingTheme_.controlBorder);
    controlSection_->addColorSwatch("Highlight:", &editingTheme_.controlHighlight);
    controlSection_->addColorSwatch("Active:", &editingTheme_.controlActive);
    controlSection_->onColorChanged([this]() { handleColorChanged(); });
    controlSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*controlSection_);

    // Status section
    statusSection_ = std::make_unique<ThemeColorSection>("Status Colors");
    statusSection_->addColorSwatch("Active:", &editingTheme_.statusActive);
    statusSection_->addColorSwatch("Warning:", &editingTheme_.statusWarning);
    statusSection_->addColorSwatch("Error:", &editingTheme_.statusError);
    statusSection_->onColorChanged([this]() { handleColorChanged(); });
    statusSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*statusSection_);

    resized();
}

void ThemeEditorComponent::handleCreateTheme()
{
    auto* nameInput = new juce::AlertWindow("New Theme",
        "Enter a name for the new theme:",
        juce::MessageBoxIconType::QuestionIcon);

    nameInput->addTextEditor("name", "My Theme", "Name:");
    nameInput->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameInput->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    nameInput->enterModalState(true, juce::ModalCallbackFunction::create(
        [this](int result)
        {
            if (result == 1)
            {
                auto* aw = dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());
                if (aw)
                {
                    auto name = aw->getTextEditorContents("name");
                    if (name.isNotEmpty())
                    {
                        if (ThemeManager::getInstance().createTheme(name))
                        {
                            refreshThemeList();
                            selectTheme(name);
                        }
                    }
                }
            }
        }), true);
}

void ThemeEditorComponent::handleCloneTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    auto* nameInput = new juce::AlertWindow("Clone Theme",
        "Enter a name for the cloned theme:",
        juce::MessageBoxIconType::QuestionIcon);

    nameInput->addTextEditor("name", selectedThemeName_ + " Copy", "Name:");
    nameInput->addButton("Clone", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameInput->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    auto sourceTheme = selectedThemeName_;
    nameInput->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, sourceTheme](int result)
        {
            if (result == 1)
            {
                auto* aw = dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());
                if (aw)
                {
                    auto name = aw->getTextEditorContents("name");
                    if (name.isNotEmpty())
                    {
                        if (ThemeManager::getInstance().cloneTheme(sourceTheme, name))
                        {
                            refreshThemeList();
                            selectTheme(name);
                        }
                    }
                }
            }
        }), true);
}

void ThemeEditorComponent::handleDeleteTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    if (ThemeManager::getInstance().isSystemTheme(selectedThemeName_))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Cannot Delete",
            "System themes cannot be deleted.");
        return;
    }

    auto themeName = selectedThemeName_;
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
        "Delete Theme",
        "Are you sure you want to delete '" + themeName + "'?",
        "Delete", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([this, themeName](int result)
        {
            if (result == 1)
            {
                ThemeManager::getInstance().deleteTheme(themeName);
                refreshThemeList();
                if (!themeNames_.empty())
                {
                    selectTheme(themeNames_[0]);
                }
            }
        }));
}

void ThemeEditorComponent::handleImportTheme()
{
    auto chooser = std::make_shared<juce::FileChooser>("Import Theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.json");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                auto json = file.loadFileAsString();
                if (ThemeManager::getInstance().importTheme(json))
                {
                    refreshThemeList();
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                        "Import Successful",
                        "Theme imported successfully.");
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Import Failed",
                        "Failed to import theme. Check that the file is valid JSON.");
                }
            }
        });
}

void ThemeEditorComponent::handleExportTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    auto json = ThemeManager::getInstance().exportTheme(selectedThemeName_);
    if (json.isEmpty())
        return;

    auto chooser = std::make_shared<juce::FileChooser>("Export Theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(selectedThemeName_ + ".json"),
        "*.json");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [json, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                if (file.replaceWithText(json))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                        "Export Successful",
                        "Theme exported successfully.");
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Export Failed",
                        "Failed to save theme file.");
                }
            }
        });
}

void ThemeEditorComponent::handleApplyTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    bool isSystem = ThemeManager::getInstance().isSystemTheme(selectedThemeName_);

    if (!isSystem)
    {
        // Update the custom theme with edited colors
        editingTheme_.name = nameEditor_->getText();
        ThemeManager::getInstance().updateTheme(selectedThemeName_, editingTheme_);

        // If name changed, update list
        if (editingTheme_.name != selectedThemeName_)
        {
            refreshThemeList();
            selectedThemeName_ = editingTheme_.name;
        }
    }

    // Apply as current theme
    ThemeManager::getInstance().setCurrentTheme(selectedThemeName_);
}

void ThemeEditorComponent::handleColorChanged()
{
    // Mark that changes have been made
    // Could add a "dirty" indicator here
    repaint();
}

} // namespace oscil
