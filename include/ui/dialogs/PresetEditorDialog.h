/*
    Oscil - Preset Editor Dialog
    Full visual preset editor with all rendering parameters
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
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilTabs.h"
#include "ui/components/OscilAccordion.h"
#include "ui/components/OscilColorPicker.h"
#include "ui/components/PresetPreviewPanel.h"
#include "ui/dialogs/preset_editor/ShaderSettingsTab.h"
#include "ui/dialogs/preset_editor/EffectsTab.h"
#include "ui/dialogs/preset_editor/ParticlesTab.h"
#include "ui/dialogs/preset_editor/Settings3DTab.h"
#include "ui/dialogs/preset_editor/MaterialsTab.h"
#include "ui/components/TestId.h"
#include <functional>

namespace oscil
{

/**
 * Full-featured preset editor dialog.
 *
 * Features:
 * - Name/description fields
 * - Live preview panel
 * - 5 tabs: Shader, Effects, Particles, 3D/Camera, Materials
 * - All 80+ rendering parameters accessible
 * - Real-time preview updates
 * - Save/Cancel with unsaved changes detection
 */
class PresetEditorDialog : public juce::Component,
                           public ThemeManagerListener,
                           public TestIdSupport
{
public:
    using SaveCallback = std::function<void(const VisualPreset& preset)>;
    using CancelCallback = std::function<void()>;

    PresetEditorDialog(IThemeService& themeService, VisualPresetManager& presetManager);
    ~PresetEditorDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Set up the dialog for creating a new preset.
     */
    void setNewPreset();

    /**
     * Set up the dialog for editing an existing preset.
     */
    void setPreset(const VisualPreset& preset);

    /**
     * Get the current preset data.
     */
    VisualPreset getPreset() const;

    /**
     * Check if there are unsaved changes.
     */
    bool hasUnsavedChanges() const { return hasChanges_; }

    // Callbacks
    void setOnSave(SaveCallback callback) { onSave_ = std::move(callback); }
    void setOnCancel(CancelCallback callback) { onCancel_ = std::move(callback); }

    // Size
    int getPreferredWidth() const { return CONTENT_WIDTH; }
    int getPreferredHeight() const { return CONTENT_HEIGHT; }

private:
    void setupComponents();
    void setupHeaderSection();
    void setupPreviewPanel();
    void setupTabs();
    void setupShaderTab();
    void setupEffectsTab();
    void setupParticlesTab();
    void setup3DTab();
    void setupMaterialsTab();
    void setupFooterButtons();

    // Helper to set dropdown selection by ID
    void setDropdownByItemId(OscilDropdown* dropdown, const juce::String& id);

    // Layout helper
    void layoutTabContent();

    void updateFromConfiguration();
    void updateConfiguration();
    void markChanged();
    void updatePreview();

    void handleSaveClick();
    void handleCancelClick();
    void handleResetClick();

    bool validateInput();
    void showError(const juce::String& message);
    void clearError();

    // Data
    VisualPresetManager& presetManager_;
    VisualPreset originalPreset_;
    VisualConfiguration workingConfig_;
    bool isNewPreset_ = true;
    bool hasChanges_ = false;

    // Callbacks
    SaveCallback onSave_;
    CancelCallback onCancel_;

    // Header
    std::unique_ptr<juce::Label> nameLabel_;
    std::unique_ptr<OscilTextField> nameField_;
    std::unique_ptr<juce::Label> descriptionLabel_;
    std::unique_ptr<OscilTextField> descriptionField_;
    std::unique_ptr<OscilToggle> favoriteToggle_;
    std::unique_ptr<juce::Label> errorLabel_;

    // Preview
    std::unique_ptr<PresetPreviewPanel> previewPanel_;

    // Tabs
    std::unique_ptr<OscilTabs> tabs_;
    std::unique_ptr<juce::Component> tabContent_;
    std::unique_ptr<juce::Viewport> tabViewport_;

    // Shader tab
    std::unique_ptr<ShaderSettingsTab> shaderTab_;

    // Effects tab
    std::unique_ptr<EffectsTab> effectsTab_;

    // Particles tab
    std::unique_ptr<ParticlesTab> particlesTab_;

    // 3D tab
    std::unique_ptr<Settings3DTab> settings3DTab_;

    // Materials tab
    std::unique_ptr<MaterialsTab> materialsTab_;

    // Footer
    std::unique_ptr<OscilButton> resetButton_;
    std::unique_ptr<OscilButton> cancelButton_;
    std::unique_ptr<OscilButton> saveButton_;

    // Theme
    ColorTheme theme_;
    IThemeService& themeService_;

    // Current tab content
    int currentTabIndex_ = 0;

    // Layout constants
    static constexpr int CONTENT_WIDTH = 900;
    static constexpr int CONTENT_HEIGHT = 700;
    static constexpr int HEADER_HEIGHT = 80;
    static constexpr int PREVIEW_WIDTH = 340;
    static constexpr int PREVIEW_HEIGHT = 260;
    static constexpr int TAB_HEIGHT = 40;
    static constexpr int FOOTER_HEIGHT = 48;
    static constexpr int PADDING = 16;
    static constexpr int LABEL_HEIGHT = 18;
    static constexpr int CONTROL_HEIGHT = 28;
    static constexpr int SECTION_SPACING = 12;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetEditorDialog)
};

} // namespace oscil
