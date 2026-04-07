/*
    Oscil - Theme Editor Actions
    Dialog-based action handlers for ThemeEditorComponent
    Also contains AccentPresetRow implementation (split from ThemeEditorComponent.cpp)
*/

#include "ui/theme/ThemeEditorComponent.h"
#include "ui/theme/ThemeManager.h"

#include <cmath>

namespace oscil
{

//==============================================================================
// AccentPresetRow
//==============================================================================

AccentPresetRow::AccentPresetRow(IThemeService& themeService) : themeService_(themeService)
{
    presets_ = {
        {.hue = 190.0f, .saturation = 0.7f, .lightness = 0.65f, .name = "Cyan"},
        {.hue = 145.0f, .saturation = 0.8f, .lightness = 0.65f, .name = "Green"},
        {.hue = 330.0f, .saturation = 0.8f, .lightness = 0.60f, .name = "Magenta"},
        {.hue = 35.0f, .saturation = 0.8f, .lightness = 0.65f, .name = "Orange"},
        {.hue = 220.0f, .saturation = 0.7f, .lightness = 0.60f, .name = "Blue"},
        {.hue = 270.0f, .saturation = 0.8f, .lightness = 0.60f, .name = "Violet"},
        {.hue = 5.0f, .saturation = 0.8f, .lightness = 0.55f, .name = "Red"},
    };

    setInterceptsMouseClicks(true, false);
}

void AccentPresetRow::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL).withStyle("Bold"));
    g.drawText("Accent Color", getLocalBounds().removeFromTop(20), juce::Justification::centredLeft);

    // Draw colour circles
    auto bounds = getLocalBounds();
    bounds.removeFromTop(22);

    int const numButtons = static_cast<int>(presets_.size());
    int const spacing = 6;
    int const btnSize = juce::jmin(28, (bounds.getWidth() - (spacing * (numButtons - 1))) / numButtons);
    int x = bounds.getX();
    int const y = bounds.getY() + (bounds.getHeight() - btnSize) / 2;

    for (const auto& preset : presets_)
    {
        auto colour = juce::Colour::fromHSL(preset.hue / 360.0f, preset.saturation, preset.lightness, 1.0f);
        auto btnBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(btnSize), static_cast<float>(btnSize))
                             .reduced(2.0f);
        g.setColour(colour);
        g.fillEllipse(btnBounds);

        g.setColour(theme.textPrimary.withAlpha(0.12f));
        g.drawEllipse(btnBounds, 1.0f);

        // Highlight ring if this is the current accent hue
        if (std::abs(theme.accentHue - preset.hue) < 5.0f)
        {
            g.setColour(theme.textPrimary.withAlpha(0.5f));
            g.drawEllipse(btnBounds.expanded(2.0f), 1.5f);
        }

        x += btnSize + spacing;
    }
}

void AccentPresetRow::resized() {}

void AccentPresetRow::setRowEnabled(bool enabled)
{
    setEnabled(enabled);
    setAlpha(enabled ? 1.0f : 0.5f);
}

void AccentPresetRow::mouseUp(const juce::MouseEvent& e)
{
    if (!isEnabled())
        return;

    auto pos = e.getPosition();
    auto bounds = getLocalBounds();
    bounds.removeFromTop(22);

    int const numButtons = static_cast<int>(presets_.size());
    int const spacing = 6;
    int const btnSize = juce::jmin(28, (bounds.getWidth() - (spacing * (numButtons - 1))) / numButtons);
    int x = bounds.getX();
    int const y = bounds.getY() + (bounds.getHeight() - btnSize) / 2;

    for (const auto& preset : presets_)
    {
        auto btnBounds = juce::Rectangle<int>(x, y, btnSize, btnSize);
        if (btnBounds.contains(pos))
        {
            if (onAccentSelected)
                onAccentSelected(preset.hue, preset.saturation, preset.lightness);
            return;
        }
        x += btnSize + spacing;
    }
}

//==============================================================================
// ThemeEditorComponent action handlers
//==============================================================================

// NOLINTNEXTLINE(readability-function-size)
void ThemeEditorComponent::handleCreateTheme()
{
    // Raw new is intentional — JUCE's enterModalState(deleteWhenDismissed=true) takes ownership.
    auto* nameInput =
        new juce::AlertWindow("New Theme", "Enter a name for the new theme:", juce::MessageBoxIconType::QuestionIcon);

    nameInput->addTextEditor("name", "My Theme", "Name:");
    nameInput->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameInput->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    nameInput->enterModalState(true, juce::ModalCallbackFunction::create([this](int result) {
                                   if (result == 1)
                                   {
                                       auto* aw = dynamic_cast<juce::AlertWindow*>(
                                           juce::Component::getCurrentlyModalComponent());
                                       if (aw)
                                       {
                                           auto name = aw->getTextEditorContents("name").trim();
                                           if (name.isNotEmpty())
                                           {
                                               if (getThemeService().createTheme(name))
                                               {
                                                   refreshThemeList();
                                                   selectTheme(name);
                                               }
                                               else
                                               {
                                                   juce::AlertWindow::showMessageBoxAsync(
                                                       juce::MessageBoxIconType::WarningIcon, "Create Failed",
                                                       "Could not create theme. The name may already exist or "
                                                       "contain invalid characters.");
                                               }
                                           }
                                       }
                                   }
                               }),
                               true);
}

// NOLINTNEXTLINE(readability-function-size)
void ThemeEditorComponent::handleCloneTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    // Raw new is intentional — JUCE's enterModalState(deleteWhenDismissed=true) takes ownership.
    auto* nameInput = new juce::AlertWindow(
        "Clone Theme", "Enter a name for the cloned theme:", juce::MessageBoxIconType::QuestionIcon);

    nameInput->addTextEditor("name", selectedThemeName_ + " Copy", "Name:");
    nameInput->addButton("Clone", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameInput->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    auto sourceTheme = selectedThemeName_;
    nameInput->enterModalState(true, juce::ModalCallbackFunction::create([this, sourceTheme](int result) {
                                   if (result == 1)
                                   {
                                       auto* aw = dynamic_cast<juce::AlertWindow*>(
                                           juce::Component::getCurrentlyModalComponent());
                                       if (aw)
                                       {
                                           auto name = aw->getTextEditorContents("name").trim();
                                           if (name.isNotEmpty())
                                           {
                                               if (getThemeService().cloneTheme(sourceTheme, name))
                                               {
                                                   refreshThemeList();
                                                   selectTheme(name);
                                               }
                                               else
                                               {
                                                   juce::AlertWindow::showMessageBoxAsync(
                                                       juce::MessageBoxIconType::WarningIcon, "Clone Failed",
                                                       "Could not clone theme. The name may already exist or "
                                                       "contain invalid characters.");
                                               }
                                           }
                                       }
                                   }
                               }),
                               true);
}

void ThemeEditorComponent::handleDeleteTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    if (getThemeService().isSystemTheme(selectedThemeName_))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Cannot Delete",
                                               "System themes cannot be deleted.");
        return;
    }

    auto themeName = selectedThemeName_;
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Delete Theme",
                                       "Are you sure you want to delete '" + themeName + "'?", "Delete", "Cancel",
                                       nullptr, juce::ModalCallbackFunction::create([this, themeName](int result) {
                                           if (result == 1)
                                           {
                                               getThemeService().deleteTheme(themeName);
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
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Theme", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this, chooser](const juce::FileChooser& fc) {
                             auto file = fc.getResult();
                             if (file.existsAsFile())
                             {
                                 auto xmlContent = file.loadFileAsString();
                                 if (getThemeService().importTheme(xmlContent))
                                 {
                                     refreshThemeList();
                                     juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                                                            "Import Successful",
                                                                            "Theme imported successfully.");
                                 }
                                 else
                                 {
                                     juce::AlertWindow::showMessageBoxAsync(
                                         juce::MessageBoxIconType::WarningIcon, "Import Failed",
                                         "Failed to import theme. Check that the file is a valid Oscil theme.");
                                 }
                             }
                         });
}

void ThemeEditorComponent::handleExportTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    auto json = getThemeService().exportTheme(selectedThemeName_);
    if (json.isEmpty())
        return;

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Theme",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(selectedThemeName_ + ".xml"),
        "*.xml");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [json, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                if (file.replaceWithText(json))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Export Successful",
                                                           "Theme exported successfully.");
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Export Failed",
                                                           "Failed to save theme file.");
                }
            }
        });
}

void ThemeEditorComponent::handleApplyTheme()
{
    if (selectedThemeName_.isEmpty())
        return;

    bool const isSystem = getThemeService().isSystemTheme(selectedThemeName_);

    if (!isSystem)
    {
        auto newName = nameEditor_->getText().trim();
        if (newName.isNotEmpty() && newName != selectedThemeName_)
        {
            if (!getThemeService().renameTheme(selectedThemeName_, newName))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Rename Failed",
                                                       "Could not rename theme. The name may already be in use or "
                                                       "contain invalid characters.");
                return;
            }
            selectedThemeName_ = newName;
            refreshThemeList();
        }

        editingTheme_.name = selectedThemeName_;
        getThemeService().updateTheme(selectedThemeName_, editingTheme_);
    }

    getThemeService().setCurrentTheme(selectedThemeName_);
}

void ThemeEditorComponent::handleColorChanged() { repaint(); }

} // namespace oscil
