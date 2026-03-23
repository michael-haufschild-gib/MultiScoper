/*
    Oscil - Theme Editor Component Implementation
    UI for creating and editing themes
    (Action handlers are in ThemeEditorActions.cpp)
*/

#include "ui/theme/ThemeEditorComponent.h"

#include "ui/components/TestId.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

static constexpr int kLeftPanelWidth = 170;
static constexpr int kSeparatorPadding = 10;

//==============================================================================
// ColorSwatchButton
//==============================================================================

ColorSwatchButton::ColorSwatchButton(IThemeService& themeService, const juce::String& label, juce::Colour initialColor)
    : themeService_(themeService)
    , label_(label)
    , colour_(initialColor)
{
}

void ColorSwatchButton::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    g.setColour(theme.textPrimary);
    g.setFont(12.0f);
    auto labelBounds = bounds.removeFromLeft(100.0f);
    g.drawText(label_, labelBounds.toNearestInt(), juce::Justification::centredRight);

    bounds.removeFromLeft(8.0f);

    auto swatchBounds = bounds.reduced(2.0f);

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

    g.setColour(colour_);
    g.fillRect(swatchBounds);

    g.setColour(theme.controlBorder);
    g.drawRect(swatchBounds, 1.0f);
}

void ColorSwatchButton::resized() {}

void ColorSwatchButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        auto colorPicker = std::make_unique<ColorPickerComponent>(themeService_);
        colorPicker->setColour(colour_);
        colorPicker->setSize(280, ColorPickerComponent::PREFERRED_HEIGHT);

        juce::Component::SafePointer<ColorSwatchButton> safeThis(this);
        colorPicker->onColourChanged([safeThis](juce::Colour newColour) {
            if (safeThis != nullptr)
            {
                safeThis->setColour(newColour);
            }
        });

        juce::CallOutBox::launchAsynchronously(std::move(colorPicker), getScreenBounds(), nullptr);
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

ThemeColorSection::ThemeColorSection(IThemeService& themeService, const juce::String& title)
    : themeService_(themeService)
    , title_(title)
{
}

void ThemeColorSection::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(title_, bounds.removeFromTop(24), juce::Justification::centredLeft);

    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(22, 0.0f, static_cast<float>(getWidth()));
}

void ThemeColorSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(28);

    for (auto& swatch : swatches_)
    {
        swatch->setBounds(bounds.removeFromTop(ColorSwatchButton::PREFERRED_HEIGHT));
        bounds.removeFromTop(2);
    }
}

void ThemeColorSection::addColorSwatch(const juce::String& label, juce::Colour* colorRef)
{
    auto swatch = std::make_unique<ColorSwatchButton>(themeService_, label, *colorRef);
    swatch->onColourChanged([this, colorRef](juce::Colour c) {
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

void ThemeEditorComponent::createButtons()
{
    createButton_ = std::make_unique<OscilButton>(themeService_, "New", "themeEditor_createBtn");
    createButton_->setVariant(ButtonVariant::Primary);
    createButton_->onClick = [this]() { handleCreateTheme(); };
    addAndMakeVisible(*createButton_);

    cloneButton_ = std::make_unique<OscilButton>(themeService_, "Clone", "themeEditor_cloneBtn");
    cloneButton_->setVariant(ButtonVariant::Secondary);
    cloneButton_->onClick = [this]() { handleCloneTheme(); };
    addAndMakeVisible(*cloneButton_);

    deleteButton_ = std::make_unique<OscilButton>(themeService_, "Delete", "themeEditor_deleteBtn");
    deleteButton_->setVariant(ButtonVariant::Danger);
    deleteButton_->onClick = [this]() { handleDeleteTheme(); };
    addAndMakeVisible(*deleteButton_);

    importButton_ = std::make_unique<OscilButton>(themeService_, "Import", "themeEditor_importBtn");
    importButton_->setVariant(ButtonVariant::Secondary);
    importButton_->onClick = [this]() { handleImportTheme(); };
    addAndMakeVisible(*importButton_);

    exportButton_ = std::make_unique<OscilButton>(themeService_, "Export", "themeEditor_exportBtn");
    exportButton_->setVariant(ButtonVariant::Secondary);
    exportButton_->onClick = [this]() { handleExportTheme(); };
    addAndMakeVisible(*exportButton_);

    applyButton_ = std::make_unique<OscilButton>(themeService_, "Apply", "themeEditor_applyBtn");
    applyButton_->setVariant(ButtonVariant::Primary);
    applyButton_->onClick = [this]() { handleApplyTheme(); };
    addAndMakeVisible(*applyButton_);

    closeButton_ = std::make_unique<OscilButton>(themeService_, "Close", "themeEditor_closeBtn");
    closeButton_->setVariant(ButtonVariant::Ghost);
    closeButton_->onClick = [this]() {
        if (closeCallback_)
            closeCallback_();
    };
    addAndMakeVisible(*closeButton_);
}

ThemeEditorComponent::ThemeEditorComponent(IThemeService& themeService) : themeService_(themeService)
{
    themeService_.addListener(this);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("themeEditor");
#endif

    themeList_ = std::make_unique<juce::ListBox>("Themes", this);
    themeList_->setRowHeight(24);
    addAndMakeVisible(*themeList_);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_CHILD_TEST_ID(*themeList_, "themeEditor_themeList");
#endif

    createButtons();

    nameLabel_ = std::make_unique<juce::Label>("", "Name:");
    addAndMakeVisible(*nameLabel_);

    nameEditor_ = std::make_unique<OscilTextField>(themeService_, "themeEditor_nameField");
    nameEditor_->setPlaceholder("Theme Name");
    addAndMakeVisible(*nameEditor_);

    systemThemeLabel_ = std::make_unique<juce::Label>("", "(System theme - read only)");
    systemThemeLabel_->setColour(juce::Label::textColourId, juce::Colours::orange);
    addChildComponent(*systemThemeLabel_);

    colorContainer_ = std::make_unique<juce::Component>();
    colorViewport_ = std::make_unique<juce::Viewport>();
    colorViewport_->setViewedComponent(colorContainer_.get(), false);
    colorViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*colorViewport_);

    refreshThemeList();
    selectTheme(themeService_.getCurrentTheme().name);
}

ThemeEditorComponent::~ThemeEditorComponent()
{
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_UNREGISTER_CHILD_TEST_ID("themeEditor_themeList");
#endif

    themeService_.removeListener(this);
}

void ThemeEditorComponent::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    g.setColour(theme.gridMajor);
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

    colorViewport_->setBounds(bounds);
    layoutColorSections(bounds.getWidth() - 20);
}

void ThemeEditorComponent::layoutColorSections(int sectionWidth)
{
    int y = 0;
    auto layoutSection = [&](ThemeColorSection* section) {
        if (!section)
            return;
        int h = section->getPreferredHeight();
        section->setBounds(0, y, sectionWidth, h);
        y += h + 10;
    };

    layoutSection(backgroundSection_.get());
    layoutSection(gridSection_.get());
    layoutSection(textSection_.get());
    layoutSection(controlSection_.get());

    if (statusSection_)
    {
        int h = statusSection_->getPreferredHeight();
        statusSection_->setBounds(0, y, sectionWidth, h);
        y += h;
    }

    colorContainer_->setSize(sectionWidth, y);
}

int ThemeEditorComponent::getNumRows() { return static_cast<int>(themeNames_.size()); }

void ThemeEditorComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto& theme = themeService_.getCurrentTheme();

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
        bool isSystem = themeService_.isSystemTheme(name);

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

void ThemeEditorComponent::themeChanged(const ColorTheme& /*newTheme*/) { repaint(); }

void ThemeEditorComponent::refreshThemeList()
{
    themeNames_ = themeService_.getAvailableThemes();
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

    auto* sourceTheme = themeService_.getTheme(name);
    if (sourceTheme)
    {
        editingTheme_ = *sourceTheme;
    }

    nameEditor_->setText(name, false);

    bool isSystem = themeService_.isSystemTheme(name);
    systemThemeLabel_->setVisible(isSystem);
    nameEditor_->setEnabled(!isSystem);
    deleteButton_->setEnabled(!isSystem);

    updateColorSections();
}

void ThemeEditorComponent::updateColorSections()
{
    backgroundSection_.reset();
    gridSection_.reset();
    textSection_.reset();
    controlSection_.reset();
    statusSection_.reset();

    bool isEditable = !themeService_.isSystemTheme(selectedThemeName_);

    backgroundSection_ = std::make_unique<ThemeColorSection>(themeService_, "Background Colors");
    backgroundSection_->addColorSwatch("Primary:", &editingTheme_.backgroundPrimary);
    backgroundSection_->addColorSwatch("Secondary:", &editingTheme_.backgroundSecondary);
    backgroundSection_->addColorSwatch("Pane:", &editingTheme_.backgroundPane);
    backgroundSection_->onColorChanged([this]() { handleColorChanged(); });
    backgroundSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*backgroundSection_);

    gridSection_ = std::make_unique<ThemeColorSection>(themeService_, "Grid Colors");
    gridSection_->addColorSwatch("Major:", &editingTheme_.gridMajor);
    gridSection_->addColorSwatch("Minor:", &editingTheme_.gridMinor);
    gridSection_->addColorSwatch("Zero Line:", &editingTheme_.gridZeroLine);
    gridSection_->onColorChanged([this]() { handleColorChanged(); });
    gridSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*gridSection_);

    textSection_ = std::make_unique<ThemeColorSection>(themeService_, "Text Colors");
    textSection_->addColorSwatch("Primary:", &editingTheme_.textPrimary);
    textSection_->addColorSwatch("Secondary:", &editingTheme_.textSecondary);
    textSection_->addColorSwatch("Highlight:", &editingTheme_.textHighlight);
    textSection_->onColorChanged([this]() { handleColorChanged(); });
    textSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*textSection_);

    controlSection_ = std::make_unique<ThemeColorSection>(themeService_, "Control Colors");
    controlSection_->addColorSwatch("Background:", &editingTheme_.controlBackground);
    controlSection_->addColorSwatch("Border:", &editingTheme_.controlBorder);
    controlSection_->addColorSwatch("Highlight:", &editingTheme_.controlHighlight);
    controlSection_->addColorSwatch("Active:", &editingTheme_.controlActive);
    controlSection_->onColorChanged([this]() { handleColorChanged(); });
    controlSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*controlSection_);

    statusSection_ = std::make_unique<ThemeColorSection>(themeService_, "Status Colors");
    statusSection_->addColorSwatch("Active:", &editingTheme_.statusActive);
    statusSection_->addColorSwatch("Warning:", &editingTheme_.statusWarning);
    statusSection_->addColorSwatch("Error:", &editingTheme_.statusError);
    statusSection_->onColorChanged([this]() { handleColorChanged(); });
    statusSection_->setEnabled(isEditable);
    colorContainer_->addAndMakeVisible(*statusSection_);

    resized();
}

} // namespace oscil
