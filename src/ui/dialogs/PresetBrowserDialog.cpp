/*
    Oscil - Preset Browser Dialog Implementation
*/

#include "ui/dialogs/PresetBrowserDialog.h"
#include "rendering/VisualConfiguration.h"

namespace oscil
{

PresetBrowserDialog::PresetBrowserDialog(IThemeService& themeService, VisualPresetManager& presetManager)
    : presetManager_(presetManager)
    , themeService_(themeService)
{
    themeService_.addListener(this);
    theme_ = themeService_.getCurrentTheme();

    presetManager_.addListener(this);

    OSCIL_REGISTER_TEST_ID("preset_browser_dialog");

    setupComponents();
    refreshPresets();
}

PresetBrowserDialog::~PresetBrowserDialog()
{
    presetManager_.removeListener(this);
    themeService_.removeListener(this);
}

void PresetBrowserDialog::setupComponents()
{
    setupCategorySidebar();
    setupToolbar();
    setupPresetGrid();

    // Footer buttons
    cancelButton_ = std::make_unique<OscilButton>(themeService_, "Cancel", "preset_browser_cancel_button");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() { handleCancelClick(); };
    addAndMakeVisible(*cancelButton_);

    applyButton_ = std::make_unique<OscilButton>(themeService_, "Apply", "preset_browser_apply_button");
    applyButton_->setVariant(ButtonVariant::Primary);
    applyButton_->onClick = [this]() { handleApplyClick(); };
    addAndMakeVisible(*applyButton_);
}

void PresetBrowserDialog::setupCategorySidebar()
{
    // Category buttons with counts
    auto createCategoryButton = [this](const juce::String& label, const juce::String& testId, Category cat) {
        auto btn = std::make_unique<OscilButton>(themeService_, label, testId);
        btn->setVariant(ButtonVariant::Ghost);
        btn->onClick = [this, cat]() { handleCategoryClick(cat); };
        addAndMakeVisible(*btn);
        return btn;
    };

    categoryAllButton_ = createCategoryButton("All Presets", "preset_category_all", Category::All);
    categoryFavoritesButton_ = createCategoryButton("Favorites", "preset_category_favorites", Category::Favorites);
    categorySystemButton_ = createCategoryButton("System", "preset_category_system", Category::System);
    categoryUserButton_ = createCategoryButton("User", "preset_category_user", Category::User);

    // Count labels
    for (int i = 0; i < 4; ++i)
    {
        categoryCounts_[i] = std::make_unique<juce::Label>();
        categoryCounts_[i]->setJustificationType(juce::Justification::centredRight);
        categoryCounts_[i]->setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(*categoryCounts_[i]);
    }
}

void PresetBrowserDialog::setupToolbar()
{
    // New button
    newButton_ = std::make_unique<OscilButton>(themeService_, "+ New", "preset_browser_new_button");
    newButton_->setVariant(ButtonVariant::Primary);
    newButton_->onClick = [this]() { handleNewClick(); };
    addAndMakeVisible(*newButton_);

    // Import button
    importButton_ = std::make_unique<OscilButton>(themeService_, "Import", "preset_browser_import_button");
    importButton_->setVariant(ButtonVariant::Secondary);
    importButton_->onClick = [this]() { handleImportClick(); };
    addAndMakeVisible(*importButton_);

    // Search field
    searchField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Search, "preset_browser_search_field");
    searchField_->setPlaceholder("Search presets...");
    searchField_->onTextChanged = [this](const juce::String&) { handleSearchChanged(); };
    addAndMakeVisible(*searchField_);

    // Filter dropdown
    filterDropdown_ = std::make_unique<OscilDropdown>(themeService_, "All Types", "preset_browser_filter_dropdown");
    filterDropdown_->addItem("All Types", "all");
    filterDropdown_->addItem("2D Shaders", "2d");
    filterDropdown_->addItem("3D Shaders", "3d");
    filterDropdown_->addItem("Materials", "material");
    filterDropdown_->onSelectionChanged = [this](int) { handleFilterChanged(); };
    addAndMakeVisible(*filterDropdown_);
}

void PresetBrowserDialog::setupPresetGrid()
{
    gridContainer_ = std::make_unique<juce::Component>();

    gridViewport_ = std::make_unique<juce::Viewport>();
    gridViewport_->setViewedComponent(gridContainer_.get(), false);
    gridViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*gridViewport_);
}

void PresetBrowserDialog::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(theme_.backgroundPrimary);
    g.fillAll();

    // Sidebar background
    auto sidebarBounds = bounds.removeFromLeft(SIDEBAR_WIDTH);
    g.setColour(theme_.backgroundSecondary);
    g.fillRect(sidebarBounds);

    // Sidebar border
    g.setColour(theme_.controlBorder);
    g.drawVerticalLine(SIDEBAR_WIDTH - 1, 0.0f, static_cast<float>(getHeight()));

    // Toolbar border
    g.drawHorizontalLine(TOOLBAR_HEIGHT, static_cast<float>(SIDEBAR_WIDTH), static_cast<float>(getWidth()));

    // Footer border
    g.drawHorizontalLine(getHeight() - FOOTER_HEIGHT, 0.0f, static_cast<float>(getWidth()));
}

void PresetBrowserDialog::resized()
{
    auto bounds = getLocalBounds();

    // Sidebar
    auto sidebar = bounds.removeFromLeft(SIDEBAR_WIDTH);
    sidebar.reduce(8, 8);

    OscilButton* categoryButtons[] = { categoryAllButton_.get(), categoryFavoritesButton_.get(),
                                        categorySystemButton_.get(), categoryUserButton_.get() };

    for (int i = 0; i < 4; ++i)
    {
        auto btnBounds = sidebar.removeFromTop(CATEGORY_BUTTON_HEIGHT);
        categoryButtons[i]->setBounds(btnBounds.reduced(0, 2));

        auto countBounds = btnBounds.removeFromRight(30);
        categoryCounts_[i]->setBounds(countBounds);

        sidebar.removeFromTop(4);
    }

    // Main content area
    auto content = bounds;

    // Toolbar
    auto toolbar = content.removeFromTop(TOOLBAR_HEIGHT);
    toolbar.reduce(8, 4);

    newButton_->setBounds(toolbar.removeFromLeft(70).reduced(0, 2));
    toolbar.removeFromLeft(8);
    importButton_->setBounds(toolbar.removeFromLeft(70).reduced(0, 2));
    toolbar.removeFromLeft(16);

    filterDropdown_->setBounds(toolbar.removeFromRight(120).reduced(0, 2));
    toolbar.removeFromRight(8);
    searchField_->setBounds(toolbar.reduced(0, 2));

    // Footer
    auto footer = content.removeFromBottom(FOOTER_HEIGHT);
    footer.reduce(PADDING, 8);

    int buttonWidth = 80;
    applyButton_->setBounds(footer.removeFromRight(buttonWidth).reduced(0, 4));
    footer.removeFromRight(8);
    cancelButton_->setBounds(footer.removeFromRight(buttonWidth).reduced(0, 4));

    // Grid viewport
    auto gridArea = content.reduced(8);
    gridViewport_->setBounds(gridArea);

    layoutPresetCards();
}

void PresetBrowserDialog::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;

    // Update category count label colors
    for (int i = 0; i < 4; ++i)
    {
        categoryCounts_[i]->setColour(juce::Label::textColourId, theme_.textSecondary);
    }

    repaint();
}

void PresetBrowserDialog::refreshPresets()
{
    filterPresets();
    updatePresetGrid();

    // Update category counts
    auto allPresets = presetManager_.getAllPresets();
    int allCount = static_cast<int>(allPresets.size());
    int favCount = static_cast<int>(presetManager_.getFavoritePresets().size());
    int sysCount = static_cast<int>(presetManager_.getPresetsByCategory(PresetCategory::System).size());
    int userCount = static_cast<int>(presetManager_.getPresetsByCategory(PresetCategory::User).size());

    categoryCounts_[0]->setText("(" + juce::String(allCount) + ")", juce::dontSendNotification);
    categoryCounts_[1]->setText("(" + juce::String(favCount) + ")", juce::dontSendNotification);
    categoryCounts_[2]->setText("(" + juce::String(sysCount) + ")", juce::dontSendNotification);
    categoryCounts_[3]->setText("(" + juce::String(userCount) + ")", juce::dontSendNotification);
}

void PresetBrowserDialog::setCategory(Category category)
{
    currentCategory_ = category;

    // Update button states
    categoryAllButton_->setToggled(category == Category::All);
    categoryFavoritesButton_->setToggled(category == Category::Favorites);
    categorySystemButton_->setToggled(category == Category::System);
    categoryUserButton_->setToggled(category == Category::User);

    filterPresets();
    updatePresetGrid();
}

void PresetBrowserDialog::filterPresets()
{
    filteredPresets_.clear();

    auto allPresets = presetManager_.getAllPresets();

    for (const auto& preset : allPresets)
    {
        // Category filter
        switch (currentCategory_)
        {
            case Category::Favorites:
                if (!preset.isFavorite) continue;
                break;
            case Category::System:
                if (preset.category != PresetCategory::System) continue;
                break;
            case Category::User:
                if (preset.category != PresetCategory::User) continue;
                break;
            case Category::All:
            default:
                break;
        }

        // Search filter
        if (searchText_.isNotEmpty())
        {
            if (!preset.name.containsIgnoreCase(searchText_) &&
                !preset.description.containsIgnoreCase(searchText_))
                continue;
        }

        // Shader type filter
        if (shaderFilter_.isNotEmpty() && shaderFilter_ != "all")
        {
            bool match = false;
            auto shaderType = preset.configuration.shaderType;

            if (shaderFilter_ == "2d")
            {
                match = shaderType == ShaderType::Basic2D ||
                        shaderType == ShaderType::NeonGlow ||
                        shaderType == ShaderType::GradientFill;
            }
            // Note: 3D and material shader filters removed as those shaders are no longer available

            if (!match) continue;
        }

        filteredPresets_.push_back(preset);
    }
}

void PresetBrowserDialog::updatePresetGrid()
{
    // Remove this as listener from all existing cards BEFORE clearing
    // to prevent listener leak if cards outlive their expected lifecycle
    for (auto& card : presetCards_)
    {
        card->removeListener(this);
    }

    // Clear existing cards
    presetCards_.clear();

    // Create cards for filtered presets
    for (const auto& preset : filteredPresets_)
    {
        auto card = std::make_unique<PresetCard>(themeService_, "preset_card_" + preset.id);
        card->setPreset(preset);
        card->addListener(this);

        if (preset.id == selectedPresetId_)
            card->setSelected(true);

        gridContainer_->addAndMakeVisible(*card);
        presetCards_.push_back(std::move(card));
    }

    layoutPresetCards();
}

void PresetBrowserDialog::layoutPresetCards()
{
    if (presetCards_.empty())
        return;

    int containerWidth = gridViewport_->getWidth() - gridViewport_->getScrollBarThickness();
    if (containerWidth <= 0)
        containerWidth = CONTENT_WIDTH - SIDEBAR_WIDTH - 24;

    int cardsPerRow = std::max(1, (containerWidth + GRID_GAP) / (PresetCard::CARD_WIDTH + GRID_GAP));
    int numRows = (static_cast<int>(presetCards_.size()) + cardsPerRow - 1) / cardsPerRow;

    int totalHeight = numRows * (PresetCard::CARD_HEIGHT + GRID_GAP) - GRID_GAP + 8;
    gridContainer_->setSize(containerWidth, totalHeight);

    for (size_t i = 0; i < presetCards_.size(); ++i)
    {
        int row = static_cast<int>(i) / cardsPerRow;
        int col = static_cast<int>(i) % cardsPerRow;

        int x = col * (PresetCard::CARD_WIDTH + GRID_GAP);
        int y = row * (PresetCard::CARD_HEIGHT + GRID_GAP);

        presetCards_[i]->setBounds(x, y, PresetCard::CARD_WIDTH, PresetCard::CARD_HEIGHT);
    }
}

void PresetBrowserDialog::setSelectedPresetId(const juce::String& presetId)
{
    if (selectedPresetId_ != presetId)
    {
        selectedPresetId_ = presetId;

        // Update card selection states
        for (auto& card : presetCards_)
        {
            card->setSelected(card->getPresetId() == presetId);
        }
    }
}

void PresetBrowserDialog::handleCategoryClick(Category category)
{
    setCategory(category);
}

void PresetBrowserDialog::handleNewClick()
{
    if (onNewRequested_)
        onNewRequested_();
}

void PresetBrowserDialog::handleImportClick()
{
    showImportDialog();
}

void PresetBrowserDialog::handleApplyClick()
{
    if (selectedPresetId_.isNotEmpty() && onPresetSelected_)
    {
        onPresetSelected_(selectedPresetId_);
    }
}

void PresetBrowserDialog::handleCancelClick()
{
    if (onCancel_)
        onCancel_();
}

void PresetBrowserDialog::handleSearchChanged()
{
    searchText_ = searchField_->getText();
    filterPresets();
    updatePresetGrid();
}

void PresetBrowserDialog::handleFilterChanged()
{
    shaderFilter_ = filterDropdown_->getSelectedId();
    filterPresets();
    updatePresetGrid();
}

// PresetCard::Listener implementations
void PresetBrowserDialog::presetCardClicked(const juce::String& presetId)
{
    setSelectedPresetId(presetId);
}

void PresetBrowserDialog::presetCardDoubleClicked(const juce::String& presetId)
{
    setSelectedPresetId(presetId);
    handleApplyClick();
}

void PresetBrowserDialog::presetCardEditRequested(const juce::String& presetId)
{
    if (onEditRequested_)
        onEditRequested_(presetId);
}

void PresetBrowserDialog::presetCardDuplicateRequested(const juce::String& presetId)
{
    showDuplicatePrompt(presetId);
}

void PresetBrowserDialog::presetCardDeleteRequested(const juce::String& presetId)
{
    showDeleteConfirmation(presetId);
}

void PresetBrowserDialog::presetCardExportRequested(const juce::String& presetId)
{
    showExportDialog(presetId);
}

void PresetBrowserDialog::presetCardFavoriteToggled(const juce::String& presetId, bool isFavorite)
{
    auto result = presetManager_.setFavorite(presetId, isFavorite);
    if (!result.success)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error",
            result.error
        );
    }
}

// VisualPresetManager::Listener implementations
void PresetBrowserDialog::presetAdded(const VisualPreset& preset)
{
    juce::ignoreUnused(preset);
    refreshPresets();
}

void PresetBrowserDialog::presetUpdated(const VisualPreset& preset)
{
    juce::ignoreUnused(preset);
    refreshPresets();
}

void PresetBrowserDialog::presetRemoved(const juce::String& presetId)
{
    juce::ignoreUnused(presetId);
    if (selectedPresetId_ == presetId)
        selectedPresetId_.clear();
    refreshPresets();
}

void PresetBrowserDialog::presetsReloaded()
{
    refreshPresets();
}

void PresetBrowserDialog::showDeleteConfirmation(const juce::String& presetId)
{
    auto preset = presetManager_.getPresetById(presetId);
    if (!preset)
        return;

    // Use SafePointer to safely reference this dialog in the async callback
    // This prevents crashes if the dialog is destroyed before the callback executes
    juce::Component::SafePointer<PresetBrowserDialog> safeThis(this);

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Delete Preset",
        "Delete preset '" + preset->name + "'? This cannot be undone.",
        "Delete",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([safeThis, presetId](int result) {
            if (safeThis != nullptr && result == 1)
            {
                safeThis->presetManager_.deletePreset(presetId);
            }
        })
    );
}

void PresetBrowserDialog::showDuplicatePrompt(const juce::String& presetId)
{
    auto preset = presetManager_.getPresetById(presetId);
    if (!preset)
        return;

    // Use SafePointer to safely reference this dialog in the async callback
    juce::Component::SafePointer<PresetBrowserDialog> safeThis(this);

    auto* window = new juce::AlertWindow("Duplicate Preset", "Enter name for duplicate:", juce::MessageBoxIconType::QuestionIcon);
    window->addTextEditor("name", preset->name + " Copy", "Name:");
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // Note: enterModalState with deleteWhenDismissed=true handles window deletion automatically
    window->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, presetId, window](int result) {
        if (safeThis != nullptr && result == 1)
        {
            auto newName = window->getTextEditorContents("name");
            auto duplicateResult = safeThis->presetManager_.duplicatePreset(presetId, newName);
            if (!duplicateResult.success)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Error",
                    duplicateResult.error
                );
            }
        }
        // Note: window is deleted by JUCE's modal component manager (deleteWhenDismissed=true)
    }), true);
}

void PresetBrowserDialog::showExportDialog(const juce::String& presetId)
{
    auto preset = presetManager_.getPresetById(presetId);
    if (!preset)
        return;

    // Use SafePointer to safely reference this dialog in the async callback
    juce::Component::SafePointer<PresetBrowserDialog> safeThis(this);

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(preset->name + ".json"),
        "*.json"
    );

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, presetId, chooser](const juce::FileChooser& fc) {
            if (safeThis == nullptr)
                return;

            auto file = fc.getResult();
            if (file != juce::File())
            {
                auto result = safeThis->presetManager_.exportPreset(presetId, file);
                if (!result.success)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::WarningIcon,
                        "Export Failed",
                        result.error
                    );
                }
            }
        }
    );
}

void PresetBrowserDialog::showImportDialog()
{
    // Use SafePointer to safely reference this dialog in the async callback
    juce::Component::SafePointer<PresetBrowserDialog> safeThis(this);

    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.json"
    );

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, chooser](const juce::FileChooser& fc) {
            if (safeThis == nullptr)
                return;

            auto file = fc.getResult();
            if (file != juce::File())
            {
                auto result = safeThis->presetManager_.importPreset(file);
                if (!result.success)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::WarningIcon,
                        "Import Failed",
                        result.error
                    );
                }
            }
        }
    );
}

} // namespace oscil
