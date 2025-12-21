/*
    Oscil - Preset Browser Dialog
    Browse, search, and select visual presets
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/VisualPreset.h"
#include "core/VisualPresetManager.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/PresetCard.h"
#include "ui/components/TestId.h"
#include <functional>
#include <vector>

namespace oscil
{

/**
 * Preset browser dialog for browsing and selecting visual presets.
 *
 * Features:
 * - Grid view of preset cards
 * - Category sidebar (All, Favorites, System, User)
 * - Search and filter functionality
 * - New, Import buttons
 * - Context menu on presets
 */
class PresetBrowserDialog : public juce::Component,
                            public ThemeManagerListener,
                            public TestIdSupport,
                            public PresetCard::Listener,
                            public VisualPresetManager::Listener
{
public:
    /**
     * Category filter for preset list.
     */
    enum class Category
    {
        All,
        Favorites,
        System,
        User
    };

    using PresetSelectedCallback = std::function<void(const juce::String& presetId)>;
    using EditRequestedCallback = std::function<void(const juce::String& presetId)>;
    using NewRequestedCallback = std::function<void()>;
    using CancelCallback = std::function<void()>;

    PresetBrowserDialog(IThemeService& themeService, VisualPresetManager& presetManager);
    ~PresetBrowserDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // PresetCard::Listener
    void presetCardClicked(const juce::String& presetId) override;
    void presetCardDoubleClicked(const juce::String& presetId) override;
    void presetCardEditRequested(const juce::String& presetId) override;
    void presetCardDuplicateRequested(const juce::String& presetId) override;
    void presetCardDeleteRequested(const juce::String& presetId) override;
    void presetCardExportRequested(const juce::String& presetId) override;
    void presetCardFavoriteToggled(const juce::String& presetId, bool isFavorite) override;

    // VisualPresetManager::Listener
    void presetAdded(const VisualPreset& preset) override;
    void presetUpdated(const VisualPreset& preset) override;
    void presetRemoved(const juce::String& presetId) override;
    void presetsReloaded() override;

    /**
     * Refresh the preset list from the manager.
     */
    void refreshPresets();

    /**
     * Set the currently selected preset.
     */
    void setSelectedPresetId(const juce::String& presetId);
    juce::String getSelectedPresetId() const { return selectedPresetId_; }

    // Callbacks
    void setOnPresetSelected(PresetSelectedCallback callback) { onPresetSelected_ = std::move(callback); }
    void setOnEditRequested(EditRequestedCallback callback) { onEditRequested_ = std::move(callback); }
    void setOnNewRequested(NewRequestedCallback callback) { onNewRequested_ = std::move(callback); }
    void setOnCancel(CancelCallback callback) { onCancel_ = std::move(callback); }

    // Size
    int getPreferredWidth() const { return CONTENT_WIDTH; }
    int getPreferredHeight() const { return CONTENT_HEIGHT; }

private:
    void setupComponents();
    void setupCategorySidebar();
    void setupToolbar();
    void setupPresetGrid();

    void setCategory(Category category);
    void filterPresets();
    void updatePresetGrid();
    void layoutPresetCards();

    void handleNewClick();
    void handleImportClick();
    void handleApplyClick();
    void handleCancelClick();
    void handleSearchChanged();
    void handleFilterChanged();
    void handleCategoryClick(Category category);

    void showDeleteConfirmation(const juce::String& presetId);
    void showDuplicatePrompt(const juce::String& presetId);
    void showExportDialog(const juce::String& presetId);
    void showImportDialog();

    // Data
    VisualPresetManager& presetManager_;
    std::vector<VisualPreset> filteredPresets_;
    juce::String selectedPresetId_;
    Category currentCategory_ = Category::All;
    juce::String searchText_;
    juce::String shaderFilter_;

    // Callbacks
    PresetSelectedCallback onPresetSelected_;
    EditRequestedCallback onEditRequested_;
    NewRequestedCallback onNewRequested_;
    CancelCallback onCancel_;

    // Category sidebar
    std::unique_ptr<OscilButton> categoryAllButton_;
    std::unique_ptr<OscilButton> categoryFavoritesButton_;
    std::unique_ptr<OscilButton> categorySystemButton_;
    std::unique_ptr<OscilButton> categoryUserButton_;
    std::unique_ptr<juce::Label> categoryCounts_[4];

    // Toolbar
    std::unique_ptr<OscilButton> newButton_;
    std::unique_ptr<OscilButton> importButton_;
    std::unique_ptr<OscilTextField> searchField_;
    std::unique_ptr<OscilDropdown> filterDropdown_;

    // Preset grid
    std::unique_ptr<juce::Viewport> gridViewport_;
    std::unique_ptr<juce::Component> gridContainer_;
    std::vector<std::unique_ptr<PresetCard>> presetCards_;

    // Footer
    std::unique_ptr<OscilButton> cancelButton_;
    std::unique_ptr<OscilButton> applyButton_;

    // Theme
    ColorTheme theme_;
    IThemeService& themeService_;

    // Layout constants
    static constexpr int CONTENT_WIDTH = 640;
    static constexpr int CONTENT_HEIGHT = 520;
    static constexpr int SIDEBAR_WIDTH = 140;
    static constexpr int TOOLBAR_HEIGHT = 40;
    static constexpr int FOOTER_HEIGHT = 48;
    static constexpr int CATEGORY_BUTTON_HEIGHT = 32;
    static constexpr int GRID_GAP = 12;
    static constexpr int PADDING = 16;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserDialog)
};

} // namespace oscil
