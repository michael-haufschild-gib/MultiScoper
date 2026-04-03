/*
    Oscil - Theme Editor Actions
    Dialog-based action handlers for ThemeEditorComponent
*/

#include "ui/theme/ThemeEditorComponent.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

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
                                               if (themeService_.createTheme(name))
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
                                               if (themeService_.cloneTheme(sourceTheme, name))
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

    if (themeService_.isSystemTheme(selectedThemeName_))
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
                                               themeService_.deleteTheme(themeName);
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
                                 if (themeService_.importTheme(xmlContent))
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

    auto json = themeService_.exportTheme(selectedThemeName_);
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

    bool const isSystem = themeService_.isSystemTheme(selectedThemeName_);

    if (!isSystem)
    {
        auto newName = nameEditor_->getText().trim();
        if (newName.isNotEmpty() && newName != selectedThemeName_)
        {
            if (!themeService_.renameTheme(selectedThemeName_, newName))
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
        themeService_.updateTheme(selectedThemeName_, editingTheme_);
    }

    themeService_.setCurrentTheme(selectedThemeName_);
}

void ThemeEditorComponent::handleColorChanged() { repaint(); }

} // namespace oscil
