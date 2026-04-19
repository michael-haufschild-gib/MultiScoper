/*
    MultiScoper - Oscillator Color Dialog Implementation
*/

#include "ui/dialogs/OscillatorColorDialog.h"

namespace multiscoper
{

OscillatorColorDialog::OscillatorColorDialog(IThemeService& themeService) : ThemedComponent(themeService)
{
    setupComponents();
}

OscillatorColorDialog::~OscillatorColorDialog() = default;

void OscillatorColorDialog::setupComponents()
{
    colorSwatches_ = std::make_unique<MultiScoperColorSwatches>(getThemeService(), "colorDialog_swatches");
    addAndMakeVisible(*colorSwatches_);

    okButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "OK", "colorDialog_okBtn");
    okButton_->setVariant(ButtonVariant::Primary);
    okButton_->onClick = [this]() {
        if (onColorSelected_)
            onColorSelected_(colorSwatches_->getSelectedColor());
    };
    addAndMakeVisible(*okButton_);

    cancelButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "Cancel", "colorDialog_cancelBtn");
    cancelButton_->setVariant(ButtonVariant::Secondary);
    cancelButton_->onClick = [this]() {
        if (onCancel_)
            onCancel_();
    };
    addAndMakeVisible(*cancelButton_);

    // Set size so modal can calculate bounds correctly
    setSize(getPreferredWidth(), getPreferredHeight());
}

void OscillatorColorDialog::setColors(const std::vector<juce::Colour>& colors) { colorSwatches_->setColors(colors); }

void OscillatorColorDialog::setSelectedColor(juce::Colour color)
{
    // Reset stale state so a missing color does not reuse a previous selection.
    colorSwatches_->setSelectedIndex(-1, false);
    colorSwatches_->setSelectedColor(color, false);

    if (colorSwatches_->getSelectedIndex() < 0 && !colorSwatches_->getColors().empty())
        colorSwatches_->setSelectedIndex(0, false);
}

void OscillatorColorDialog::setOnColorSelected(ColorSelectedCallback callback)
{
    onColorSelected_ = std::move(callback);
}

void OscillatorColorDialog::setOnCancel(std::function<void()> callback) { onCancel_ = std::move(callback); }

void OscillatorColorDialog::paint(juce::Graphics& /*g*/)
{
    // Background handled by Modal
}

void OscillatorColorDialog::resized()
{
    auto bounds = getLocalBounds();

    // Bottom buttons
    auto buttonRow = bounds.removeFromBottom(BUTTON_HEIGHT);

    int const buttonWidth = 80;
    okButton_->setBounds(buttonRow.removeFromRight(buttonWidth));
    buttonRow.removeFromRight(10); // Spacing
    cancelButton_->setBounds(buttonRow.removeFromRight(buttonWidth));

    bounds.removeFromBottom(10); // Spacing

    // Swatches take remaining space
    colorSwatches_->setBounds(bounds);
}

int OscillatorColorDialog::getPreferredWidth() const { return 300; }
int OscillatorColorDialog::getPreferredHeight() const { return 220; }

} // namespace multiscoper
