/*
    MultiScoper - Select Pane Dialog Implementation
    Modal dialog for selecting a pane to assign to an oscillator
*/

#include "ui/dialogs/SelectPaneDialog.h"

#include "ui/theme/Typography.h"

namespace multiscoper
{

SelectPaneDialog::SelectPaneDialog(IThemeService& themeService) : ThemedComponent(themeService)
{
    MULTISCOPER_REGISTER_TEST_ID("selectPaneDialog");
    setupComponents();
}

SelectPaneDialog::~SelectPaneDialog() = default;

void SelectPaneDialog::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

void SelectPaneDialog::setupComponents()
{
    // Instruction label
    instructionLabel_ = std::make_unique<juce::Label>("", "Select a pane to display the oscillator:");
    addAndMakeVisible(*instructionLabel_);

    // Pane selector (with "New pane" option)
    paneSelector_ = std::make_unique<PaneSelectorComponent>(getThemeService(), true, "selectPaneDialog_paneSelector");
    paneSelector_->onSelectionChanged = [this](const PaneId& paneId, bool isNewPane) {
        handlePaneSelectionChange(paneId, isNewPane);
    };
    addAndMakeVisible(*paneSelector_);

    // Error label (hidden by default)
    errorLabel_ = std::make_unique<juce::Label>("", "");
    errorLabel_->setVisible(false);
    addAndMakeVisible(*errorLabel_);

    // OK button (Primary)
    okButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "OK", "selectPaneDialog_okBtn");
    okButton_->setVariant(ButtonVariant::Primary);
    okButton_->onClick = [this]() { handleOkClick(); };
    addAndMakeVisible(*okButton_);

    // Cancel button (Secondary)
    cancelButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Cancel", "selectPaneDialog_cancelBtn");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() { handleCancelClick(); };
    addAndMakeVisible(*cancelButton_);

    // Apply initial theme
    themeChanged(getThemeService().getCurrentTheme());

    // Set size for MultiScoperModal
    setSize(getPreferredWidth(), getPreferredHeight());
}

void SelectPaneDialog::paint(juce::Graphics& /*g*/)
{
    // No custom painting - MultiScoperModal handles backdrop/frame
    // Child components handle their own painting
}

void SelectPaneDialog::resized()
{
    auto bounds = getLocalBounds();

    // Instruction label
    instructionLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT * 2));
    bounds.removeFromTop(SECTION_SPACING);

    // Pane selector
    paneSelector_->setBounds(bounds.removeFromTop(CONTROL_HEIGHT));
    bounds.removeFromTop(SECTION_SPACING);

    // Error label
    errorLabel_->setBounds(bounds.removeFromTop(LABEL_HEIGHT));
    bounds.removeFromTop(8);

    // Footer buttons at bottom
    auto footerRow = bounds.removeFromBottom(BUTTON_HEIGHT);
    int const buttonWidth = (footerRow.getWidth() - 8) / 2;
    cancelButton_->setBounds(footerRow.removeFromLeft(buttonWidth));
    footerRow.removeFromLeft(8);
    okButton_->setBounds(footerRow);
}

void SelectPaneDialog::onThemeChanged(const ColorTheme& newTheme)
{
    // Style instruction label
    instructionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    instructionLabel_->setFont(Typography::small());

    // Error label uses error color
    errorLabel_->setColour(juce::Label::textColourId, newTheme.statusError);
    errorLabel_->setFont(Typography::small());
}

void SelectPaneDialog::setAvailablePanes(const std::vector<Pane>& panes)
{
    paneSelector_->setAvailablePanes(panes);
    reset();
}

void SelectPaneDialog::setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes)
{
    paneSelector_->setAvailablePanes(panes);
    reset();
}

void SelectPaneDialog::reset()
{
    clearError();
    paneSelector_->selectNewPane(false);
}

void SelectPaneDialog::handleOkClick()
{
    if (!validateSelection())
    {
        return;
    }

    // Build result
    Result result;
    result.createNewPane = paneSelector_->isNewPaneSelected();
    result.paneId = paneSelector_->getSelectedPaneId();

    // Call callback
    if (callback_)
    {
        callback_(result);
    }
}

void SelectPaneDialog::handleCancelClick()
{
    if (cancelCallback_)
    {
        cancelCallback_();
    }
}

void SelectPaneDialog::handlePaneSelectionChange(const PaneId& /*unused*/, bool /*unused*/) { clearError(); }

void SelectPaneDialog::showError(const juce::String& message)
{
    errorLabel_->setText(message, juce::dontSendNotification);
    errorLabel_->setVisible(true);
}

void SelectPaneDialog::clearError()
{
    errorLabel_->setText("", juce::dontSendNotification);
    errorLabel_->setVisible(false);
}

bool SelectPaneDialog::validateSelection()
{
    if (!paneSelector_->hasValidSelection())
    {
        showError("Please select a pane.");
        return false;
    }

    return true;
}

} // namespace multiscoper
