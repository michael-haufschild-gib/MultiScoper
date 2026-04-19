/*
    MultiScoper - Theme Editor Component Implementation
    UI for creating and editing themes
    (Action handlers are in ThemeEditorActions.cpp)
*/

#include "ui/theme/ThemeEditorComponent.h"

#include "ui/components/ComponentConstants.h"
#include "ui/components/SurfacePainter.h"
#include "ui/components/SurfaceStyle.h"
#include "ui/components/TestId.h"
#include "ui/theme/ThemeManager.h"

#include <utility>

namespace multiscoper
{

static constexpr int kLeftPanelWidth = 170;
static constexpr int kSeparatorPadding = 10;

// ColorSwatchButton and ThemeColorSection implementations are in
// ThemeEditorWidgets.cpp. Action handlers are in ThemeEditorActions.cpp.

//==============================================================================
// ThemeEditorComponent
//==============================================================================

void ThemeEditorComponent::createButtons()
{
    createButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "New", "themeEditor_createBtn");
    createButton_->setVariant(ButtonVariant::Primary);
    createButton_->onClick = [this]() { handleCreateTheme(); };
    addAndMakeVisible(*createButton_);

    cloneButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Clone", "themeEditor_cloneBtn");
    cloneButton_->setVariant(ButtonVariant::Secondary);
    cloneButton_->onClick = [this]() { handleCloneTheme(); };
    addAndMakeVisible(*cloneButton_);

    deleteButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Delete", "themeEditor_deleteBtn");
    deleteButton_->setVariant(ButtonVariant::Danger);
    deleteButton_->onClick = [this]() { handleDeleteTheme(); };
    addAndMakeVisible(*deleteButton_);

    importButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Import", "themeEditor_importBtn");
    importButton_->setVariant(ButtonVariant::Secondary);
    importButton_->onClick = [this]() { handleImportTheme(); };
    addAndMakeVisible(*importButton_);

    exportButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Export", "themeEditor_exportBtn");
    exportButton_->setVariant(ButtonVariant::Secondary);
    exportButton_->onClick = [this]() { handleExportTheme(); };
    addAndMakeVisible(*exportButton_);

    applyButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Apply", "themeEditor_applyBtn");
    applyButton_->setVariant(ButtonVariant::Primary);
    applyButton_->onClick = [this]() { handleApplyTheme(); };
    addAndMakeVisible(*applyButton_);

    closeButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Close", "themeEditor_closeBtn");
    closeButton_->setVariant(ButtonVariant::Ghost);
    closeButton_->onClick = [this]() {
        if (closeCallback_)
            closeCallback_();
    };
    addAndMakeVisible(*closeButton_);
}

ThemeEditorComponent::ThemeEditorComponent(IThemeService& themeService) : ThemedComponent(themeService)
{
#if defined(TEST_HARNESS) || defined(MULTISCOPER_ENABLE_TEST_IDS)
    MULTISCOPER_REGISTER_TEST_ID("themeEditor");
#endif

    themeList_ = std::make_unique<juce::ListBox>("Themes", this);
    themeList_->setRowHeight(24);
    addAndMakeVisible(*themeList_);

#if defined(TEST_HARNESS) || defined(MULTISCOPER_ENABLE_TEST_IDS)
    MULTISCOPER_REGISTER_CHILD_TEST_ID(*themeList_, "themeEditor_themeList");
#endif

    createButtons();

    nameLabel_ = std::make_unique<juce::Label>("", "Name:");
    addAndMakeVisible(*nameLabel_);

    nameEditor_ = std::make_unique<MultiScoperTextField>(getThemeService(), "themeEditor_nameField");
    nameEditor_->setPlaceholder("Theme Name");
    addAndMakeVisible(*nameEditor_);

    systemThemeLabel_ = std::make_unique<juce::Label>("", "(System theme - read only)");
    systemThemeLabel_->setColour(juce::Label::textColourId, juce::Colours::orange);
    addChildComponent(*systemThemeLabel_);

    accentPresetRow_ = std::make_unique<AccentPresetRow>(getThemeService());
    accentPresetRow_->onAccentSelected = [this](float hue, float sat, float light) {
        editingTheme_.accentHue = hue;
        editingTheme_.accentSaturation = sat;
        editingTheme_.accentLightness = light;
        handleColorChanged();
        accentPresetRow_->repaint();
    };
    addAndMakeVisible(*accentPresetRow_);

    colorContainer_ = std::make_unique<juce::Component>();
    colorViewport_ = std::make_unique<juce::Viewport>();
    colorViewport_->setViewedComponent(colorContainer_.get(), false);
    colorViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*colorViewport_);

    refreshThemeList();
    selectTheme(getThemeService().getCurrentTheme().name);
}

ThemeEditorComponent::~ThemeEditorComponent()
{
#if defined(TEST_HARNESS) || defined(MULTISCOPER_ENABLE_TEST_IDS)
    MULTISCOPER_UNREGISTER_CHILD_TEST_ID(*themeList_, "themeEditor_themeList");
#endif
}

void ThemeEditorComponent::paint(juce::Graphics& g)
{
    const auto& theme = getThemeService().getCurrentTheme();
    auto glass = SurfaceStyle::fromTheme(theme);

    SurfacePainter::paintPanel(g, getLocalBounds().toFloat(), glass, ComponentLayout::RADIUS_XL, BorderLevel::Subtle);

    g.setColour(glass.borderSubtle);
    g.drawVerticalLine(kLeftPanelWidth + kSeparatorPadding, 0.0f, static_cast<float>(getHeight()));
}

void ThemeEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

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

    bounds.removeFromLeft(kSeparatorPadding * 2);

    auto bottomButtons = bounds.removeFromBottom(30);
    closeButton_->setBounds(bottomButtons.removeFromRight(80));
    bottomButtons.removeFromRight(10);
    applyButton_->setBounds(bottomButtons.removeFromRight(80));

    bounds.removeFromBottom(10);

    auto nameRow = bounds.removeFromTop(25);
    nameLabel_->setBounds(nameRow.removeFromLeft(50));
    nameRow.removeFromLeft(5);

    systemThemeLabel_->setBounds(nameRow.removeFromRight(180));
    nameEditor_->setBounds(nameRow);

    bounds.removeFromTop(10);

    // Accent color presets
    accentPresetRow_->setBounds(bounds.removeFromTop(AccentPresetRow::PREFERRED_HEIGHT));
    bounds.removeFromTop(6);

    colorViewport_->setBounds(bounds);
    layoutColorSections(bounds.getWidth() - 20);
}

void ThemeEditorComponent::layoutColorSections(int sectionWidth)
{
    int y = 0;
    auto layoutSection = [&](ThemeColorSection* section) {
        if (!section)
            return;
        int const h = section->getPreferredHeight();
        section->setBounds(0, y, sectionWidth, h);
        y += h + 10;
    };

    layoutSection(backgroundSection_.get());
    layoutSection(gridSection_.get());
    layoutSection(textSection_.get());
    layoutSection(controlSection_.get());

    if (statusSection_)
    {
        int const h = statusSection_->getPreferredHeight();
        statusSection_->setBounds(0, y, sectionWidth, h);
        y += h;
    }

    colorContainer_->setSize(sectionWidth, y);
}

int ThemeEditorComponent::getNumRows() { return static_cast<int>(themeNames_.size()); }

void ThemeEditorComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto& theme = getThemeService().getCurrentTheme();

    if (rowIsSelected)
    {
        g.setColour(theme.controlActive);
        g.fillRect(0, 0, width, height);
    }

    if (rowNumber >= 0 && std::cmp_less(rowNumber, themeNames_.size()))
    {
        g.setColour(rowIsSelected ? theme.textHighlight : theme.textPrimary);
        g.setFont(12.0f);

        auto name = themeNames_[static_cast<size_t>(rowNumber)];
        bool const isSystem = getThemeService().isSystemTheme(name);

        juce::String displayName = name;
        if (isSystem)
            displayName += " *";

        g.drawText(displayName, 5, 0, width - 10, height, juce::Justification::centredLeft);
    }
}

void ThemeEditorComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && std::cmp_less(lastRowSelected, themeNames_.size()))
    {
        selectTheme(themeNames_[static_cast<size_t>(lastRowSelected)]);
    }
}

void ThemeEditorComponent::refreshThemeList()
{
    themeNames_ = getThemeService().getAvailableThemes();
    themeList_->updateContent();
}

void ThemeEditorComponent::selectTheme(const juce::String& name)
{
    selectedThemeName_ = name;

    for (size_t i = 0; i < themeNames_.size(); ++i)
    {
        if (themeNames_[i] == name)
        {
            themeList_->selectRow(static_cast<int>(i));
            break;
        }
    }

    const auto* sourceTheme = getThemeService().getTheme(name);
    if (sourceTheme)
    {
        editingTheme_ = *sourceTheme;
    }

    nameEditor_->setText(name, false);

    bool const isSystem = getThemeService().isSystemTheme(name);
    systemThemeLabel_->setVisible(isSystem);
    nameEditor_->setEnabled(!isSystem);
    deleteButton_->setEnabled(!isSystem);
    accentPresetRow_->setRowEnabled(!isSystem);
    accentPresetRow_->repaint();

    updateColorSections();
}

void ThemeEditorComponent::updateColorSections()
{
    backgroundSection_.reset();
    gridSection_.reset();
    textSection_.reset();
    controlSection_.reset();
    statusSection_.reset();

    bool const isEditable = !getThemeService().isSystemTheme(selectedThemeName_);

    backgroundSection_ = std::make_unique<ThemeColorSection>(getThemeService(), "Background Colors");
    backgroundSection_->addColorSwatch("Primary:", &editingTheme_.backgroundPrimary);
    backgroundSection_->addColorSwatch("Secondary:", &editingTheme_.backgroundSecondary);
    backgroundSection_->addColorSwatch("Pane:", &editingTheme_.backgroundPane);
    backgroundSection_->onColorChanged([this]() { handleColorChanged(); });
    backgroundSection_->setSectionEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*backgroundSection_);

    gridSection_ = std::make_unique<ThemeColorSection>(getThemeService(), "Grid Colors");
    gridSection_->addColorSwatch("Major:", &editingTheme_.gridMajor);
    gridSection_->addColorSwatch("Minor:", &editingTheme_.gridMinor);
    gridSection_->addColorSwatch("Zero Line:", &editingTheme_.gridZeroLine);
    gridSection_->addColorSwatch("Crosshair:", &editingTheme_.crosshairLine);
    gridSection_->onColorChanged([this]() { handleColorChanged(); });
    gridSection_->setSectionEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*gridSection_);

    textSection_ = std::make_unique<ThemeColorSection>(getThemeService(), "Text Colors");
    textSection_->addColorSwatch("Primary:", &editingTheme_.textPrimary);
    textSection_->addColorSwatch("Secondary:", &editingTheme_.textSecondary);
    textSection_->addColorSwatch("Highlight:", &editingTheme_.textHighlight);
    textSection_->onColorChanged([this]() { handleColorChanged(); });
    textSection_->setSectionEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*textSection_);

    controlSection_ = std::make_unique<ThemeColorSection>(getThemeService(), "Control Colors");
    controlSection_->addColorSwatch("Background:", &editingTheme_.controlBackground);
    controlSection_->addColorSwatch("Border:", &editingTheme_.controlBorder);
    controlSection_->addColorSwatch("Highlight:", &editingTheme_.controlHighlight);
    controlSection_->addColorSwatch("Active:", &editingTheme_.controlActive);
    controlSection_->onColorChanged([this]() { handleColorChanged(); });
    controlSection_->setSectionEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*controlSection_);

    statusSection_ = std::make_unique<ThemeColorSection>(getThemeService(), "Status Colors");
    statusSection_->addColorSwatch("Active:", &editingTheme_.statusActive);
    statusSection_->addColorSwatch("Warning:", &editingTheme_.statusWarning);
    statusSection_->addColorSwatch("Error:", &editingTheme_.statusError);
    statusSection_->onColorChanged([this]() { handleColorChanged(); });
    statusSection_->setSectionEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*statusSection_);

    resized();
}

} // namespace multiscoper
