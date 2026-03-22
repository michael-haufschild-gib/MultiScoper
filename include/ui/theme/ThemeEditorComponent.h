/*
    Oscil - Theme Editor Component Header
    UI for creating and editing themes
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/IThemeService.h"
#include "ThemeManager.h"
#include "ColorPickerComponent.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/TestId.h"
#include <memory>
#include <functional>

namespace oscil
{

/**
 * Color swatch button for quick color editing
 */
class ColorSwatchButton : public juce::Component
{
public:
    ColorSwatchButton(IThemeService& themeService, const juce::String& label, juce::Colour initialColor);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setColour(juce::Colour colour);
    juce::Colour getColour() const { return colour_; }

    void onColourChanged(std::function<void(juce::Colour)> callback)
    {
        colourChangedCallback_ = std::move(callback);
    }

    static constexpr int PREFERRED_HEIGHT = 28;

private:
    IThemeService& themeService_;
    juce::String label_;
    juce::Colour colour_;
    std::function<void(juce::Colour)> colourChangedCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorSwatchButton)
};

/**
 * Theme color section - groups related colors
 */
class ThemeColorSection : public juce::Component
{
public:
    ThemeColorSection(IThemeService& themeService, const juce::String& title);

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Add a named colour swatch row bound to the given colour variable.
    void addColorSwatch(const juce::String& label, juce::Colour* colorRef);
    /// Refresh all swatch displays from the given theme's current colour values.
    void updateFromTheme(ColorTheme& theme);
    void setEnabled(bool enabled);

    int getPreferredHeight() const;

    void onColorChanged(std::function<void()> callback)
    {
        colorChangedCallback_ = std::move(callback);
    }

private:
    IThemeService& themeService_;
    juce::String title_;
    std::vector<std::unique_ptr<ColorSwatchButton>> swatches_;
    std::vector<juce::Colour*> colorRefs_;
    std::function<void()> colorChangedCallback_;
    bool enabled_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeColorSection)
};

/**
 * Main theme editor component
 * Allows creating, editing, and deleting themes
 */
class ThemeEditorComponent : public juce::Component,
                              public juce::ListBoxModel,
                              public ThemeManagerListener,
                              public TestIdSupport
{
public:
    explicit ThemeEditorComponent(IThemeService& themeService);
    ~ThemeEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Return the number of saved themes for the list box (ListBoxModel).
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;
    /// Apply the selected theme when the list selection changes (ListBoxModel).
    void selectedRowsChanged(int lastRowSelected) override;

    // ThemeManagerListener interface
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Callback when user wants to close the editor
     */
    void onClose(std::function<void()> callback) { closeCallback_ = std::move(callback); }

    static constexpr int PREFERRED_WIDTH = 600;
    static constexpr int PREFERRED_HEIGHT = 500;

private:
    void createButtons();
    void layoutColorSections(int sectionWidth);
    void refreshThemeList();
    void selectTheme(const juce::String& name);
    void updateColorSections();
    void handleCreateTheme();
    void handleCloneTheme();
    void handleDeleteTheme();
    void handleImportTheme();
    void handleExportTheme();
    void handleApplyTheme();
    void handleColorChanged();

    IThemeService& themeService_;

    // Theme list
    std::unique_ptr<juce::ListBox> themeList_;
    std::vector<juce::String> themeNames_;
    juce::String selectedThemeName_;
    ColorTheme editingTheme_;  // Copy of theme being edited

    // Action buttons
    std::unique_ptr<OscilButton> createButton_;
    std::unique_ptr<OscilButton> cloneButton_;
    std::unique_ptr<OscilButton> deleteButton_;
    std::unique_ptr<OscilButton> importButton_;
    std::unique_ptr<OscilButton> exportButton_;
    std::unique_ptr<OscilButton> applyButton_;
    std::unique_ptr<OscilButton> closeButton_;

    // Color sections (scrollable)
    std::unique_ptr<juce::Viewport> colorViewport_;
    std::unique_ptr<juce::Component> colorContainer_;
    std::unique_ptr<ThemeColorSection> backgroundSection_;
    std::unique_ptr<ThemeColorSection> gridSection_;
    std::unique_ptr<ThemeColorSection> textSection_;
    std::unique_ptr<ThemeColorSection> controlSection_;
    std::unique_ptr<ThemeColorSection> statusSection_;

    // Theme name editor
    std::unique_ptr<juce::Label> nameLabel_;
    std::unique_ptr<OscilTextField> nameEditor_;

    // System theme indicator
    std::unique_ptr<juce::Label> systemThemeLabel_;

    std::function<void()> closeCallback_;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeEditorComponent)
};

} // namespace oscil
