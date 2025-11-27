/*
    Oscil - Oscillator Configuration Popup Implementation
    Modal popup for comprehensive oscillator configuration
*/

#include "ui/OscillatorConfigPopup.h"
#include "ui/SourceSelectorComponent.h"

namespace oscil
{

OscillatorConfigPopup::OscillatorConfigPopup()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

OscillatorConfigPopup::~OscillatorConfigPopup()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscillatorConfigPopup::setupComponents()
{
    // Header: Name editor
    nameEditor_ = std::make_unique<juce::TextEditor>();
    nameEditor_->setMultiLine(false);
    nameEditor_->setReturnKeyStartsNewLine(false);
    nameEditor_->onReturnKey = [this]() { handleNameEdit(); };
    nameEditor_->onFocusLost = [this]() { handleNameEdit(); };
    addAndMakeVisible(*nameEditor_);

    // Header: Visibility button
    visibilityButton_ = std::make_unique<juce::TextButton>();
    visibilityButton_->setButtonText(juce::String::charToString(0x1F441));  // Eye emoji
    visibilityButton_->setTooltip("Toggle Visibility");
    visibilityButton_->onClick = [this]() { handleVisibilityToggle(); };
    addAndMakeVisible(*visibilityButton_);

    // Header: Close button
    closeButton_ = std::make_unique<juce::TextButton>();
    closeButton_->setButtonText(juce::String::charToString(0x2715));  // X symbol
    closeButton_->setTooltip("Close");
    closeButton_->onClick = [this]() { handleClose(); };
    addAndMakeVisible(*closeButton_);

    // Source section
    sourceLabel_ = std::make_unique<juce::Label>("", "Source");
    addAndMakeVisible(*sourceLabel_);

    sourceSelector_ = std::make_unique<SourceSelectorComponent>();
    sourceSelector_->onSelectionChanged([this](const SourceId& id) { handleSourceChange(id); });
    addAndMakeVisible(*sourceSelector_);

    // Processing mode section
    modeLabel_ = std::make_unique<juce::Label>("", "Processing Mode");
    addAndMakeVisible(*modeLabel_);

    modeButtons_ = std::make_unique<SegmentedButtonBar>();
    modeButtons_->addButton("Full Stereo", static_cast<int>(ProcessingMode::FullStereo));
    modeButtons_->addButton("Mono", static_cast<int>(ProcessingMode::Mono));
    modeButtons_->addButton("Mid", static_cast<int>(ProcessingMode::Mid));
    modeButtons_->addButton("Side", static_cast<int>(ProcessingMode::Side));
    modeButtons_->addButton("Left", static_cast<int>(ProcessingMode::Left));
    modeButtons_->addButton("Right", static_cast<int>(ProcessingMode::Right));
    modeButtons_->onSelectionChanged = [this](int id) { handleProcessingModeChange(id); };
    addAndMakeVisible(*modeButtons_);

    // Color swatches (per design: "Color Picker")
    colorLabel_ = std::make_unique<juce::Label>("", "Color Picker");
    addAndMakeVisible(*colorLabel_);

    for (int i = 0; i < NUM_COLOR_SWATCHES; ++i)
    {
        auto swatch = std::make_unique<juce::TextButton>();
        swatch->setColour(juce::TextButton::buttonColourId, defaultColors_[i]);
        swatch->onClick = [this, i]() { handleColorSelect(defaultColors_[i]); };
        addAndMakeVisible(*swatch);
        colorSwatches_.push_back(std::move(swatch));
    }

    // Line Width slider
    lineWidthLabel_ = std::make_unique<juce::Label>("", "Line Width");
    addAndMakeVisible(*lineWidthLabel_);

    lineWidthSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    lineWidthSlider_->setRange(Oscillator::MIN_LINE_WIDTH, Oscillator::MAX_LINE_WIDTH, 0.1);
    lineWidthSlider_->setValue(Oscillator::DEFAULT_LINE_WIDTH);
    lineWidthSlider_->setTextValueSuffix(" px");
    lineWidthSlider_->onValueChange = [this]() { handleLineWidthChange(); };
    addAndMakeVisible(*lineWidthSlider_);

    // Opacity slider
    opacityLabel_ = std::make_unique<juce::Label>("", "Opacity");
    addAndMakeVisible(*opacityLabel_);

    opacitySlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    opacitySlider_->setRange(0.0, 100.0, 1.0);
    opacitySlider_->setValue(100.0);
    opacitySlider_->setTextValueSuffix(" %");
    opacitySlider_->onValueChange = [this]() { handleOpacityChange(); };
    addAndMakeVisible(*opacitySlider_);

    // Vertical Scale slider
    scaleLabel_ = std::make_unique<juce::Label>("", "Vertical Scale");
    addAndMakeVisible(*scaleLabel_);

    scaleSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    scaleSlider_->setRange(Oscillator::MIN_VERTICAL_SCALE, Oscillator::MAX_VERTICAL_SCALE, 0.1);
    scaleSlider_->setValue(Oscillator::DEFAULT_VERTICAL_SCALE);
    scaleSlider_->setTextValueSuffix("x");
    scaleSlider_->onValueChange = [this]() { handleVerticalScaleChange(); };
    addAndMakeVisible(*scaleSlider_);

    // Vertical Offset slider
    offsetLabel_ = std::make_unique<juce::Label>("", "Vertical Offset");
    addAndMakeVisible(*offsetLabel_);

    offsetSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    offsetSlider_->setRange(Oscillator::MIN_VERTICAL_OFFSET, Oscillator::MAX_VERTICAL_OFFSET, 0.01);
    offsetSlider_->setValue(Oscillator::DEFAULT_VERTICAL_OFFSET);
    offsetSlider_->onValueChange = [this]() { handleVerticalOffsetChange(); };
    addAndMakeVisible(*offsetSlider_);

    // Pane selector (per design: just "Pane")
    paneLabel_ = std::make_unique<juce::Label>("", "Pane");
    addAndMakeVisible(*paneLabel_);

    paneSelector_ = std::make_unique<juce::ComboBox>();
    paneSelector_->onChange = [this]() { handlePaneChange(); };
    addAndMakeVisible(*paneSelector_);

    // Footer buttons (per design: Delete Oscillator + Close)
    deleteButton_ = std::make_unique<juce::TextButton>("Delete Oscillator");
    deleteButton_->onClick = [this]() { handleDelete(); };
    addAndMakeVisible(*deleteButton_);

    footerCloseButton_ = std::make_unique<juce::TextButton>("Close");
    footerCloseButton_->onClick = [this]() { handleClose(); };
    addAndMakeVisible(*footerCloseButton_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void OscillatorConfigPopup::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Semi-transparent overlay for modal effect
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRect(bounds);

    // Popup background
    auto popupBounds = bounds.reduced(
        (bounds.getWidth() - POPUP_WIDTH) / 2,
        (bounds.getHeight() - POPUP_HEIGHT) / 2
    );

    g.setColour(theme.backgroundPrimary);
    g.fillRoundedRectangle(popupBounds, 8.0f);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(popupBounds, 8.0f, 1.0f);

    // Header background
    auto headerBounds = popupBounds.removeFromTop(48);
    g.setColour(theme.backgroundSecondary);
    g.fillRoundedRectangle(headerBounds.getX(), headerBounds.getY(),
                           headerBounds.getWidth(), headerBounds.getHeight() + 8, 8.0f);
    g.fillRect(headerBounds.getX(), headerBounds.getY() + 8,
               headerBounds.getWidth(), headerBounds.getHeight() - 8);

    // Section dividers
    g.setColour(theme.controlBorder.withAlpha(0.3f));

    // Draw color indicator next to name
    auto colorIndicatorBounds = juce::Rectangle<float>(
        popupBounds.getX() + 16,
        getHeight() / 2.0f - POPUP_HEIGHT / 2.0f + 12,
        24, 24
    );
    g.setColour(colour_);
    g.fillEllipse(colorIndicatorBounds);
}

void OscillatorConfigPopup::resized()
{
    auto bounds = getLocalBounds();

    // Center the popup content
    auto popupBounds = bounds.reduced(
        (bounds.getWidth() - POPUP_WIDTH) / 2,
        (bounds.getHeight() - POPUP_HEIGHT) / 2
    );

    auto contentBounds = popupBounds.reduced(16);

    // Header row
    auto headerRow = contentBounds.removeFromTop(32);
    headerRow.removeFromLeft(32);  // Space for color indicator

    closeButton_->setBounds(headerRow.removeFromRight(28).reduced(2));
    visibilityButton_->setBounds(headerRow.removeFromRight(28).reduced(2));
    headerRow.removeFromRight(8);
    nameEditor_->setBounds(headerRow);

    contentBounds.removeFromTop(16);

    // Source section
    sourceLabel_->setBounds(contentBounds.removeFromTop(20));
    sourceSelector_->setBounds(contentBounds.removeFromTop(28));
    contentBounds.removeFromTop(12);

    // Processing mode section
    modeLabel_->setBounds(contentBounds.removeFromTop(20));
    modeButtons_->setBounds(contentBounds.removeFromTop(28));
    contentBounds.removeFromTop(12);

    // Color swatches
    colorLabel_->setBounds(contentBounds.removeFromTop(20));
    auto colorRow = contentBounds.removeFromTop(28);
    int swatchWidth = colorRow.getWidth() / NUM_COLOR_SWATCHES;
    for (int i = 0; i < NUM_COLOR_SWATCHES; ++i)
    {
        colorSwatches_[i]->setBounds(colorRow.removeFromLeft(swatchWidth).reduced(2));
    }
    contentBounds.removeFromTop(12);

    // Slider rows (label + slider)
    auto makeSliderRow = [&contentBounds](juce::Label* label, juce::Slider* slider) {
        auto row = contentBounds.removeFromTop(32);
        label->setBounds(row.removeFromLeft(100));
        slider->setBounds(row);
        contentBounds.removeFromTop(8);
    };

    makeSliderRow(lineWidthLabel_.get(), lineWidthSlider_.get());
    makeSliderRow(opacityLabel_.get(), opacitySlider_.get());
    makeSliderRow(scaleLabel_.get(), scaleSlider_.get());
    makeSliderRow(offsetLabel_.get(), offsetSlider_.get());

    contentBounds.removeFromTop(4);

    // Pane selector
    auto paneRow = contentBounds.removeFromTop(32);
    paneLabel_->setBounds(paneRow.removeFromLeft(100));
    paneSelector_->setBounds(paneRow);
    contentBounds.removeFromTop(16);

    // Footer buttons at bottom (Delete Oscillator + Close per design)
    auto footerRow = contentBounds.removeFromBottom(36);
    int buttonWidth = (footerRow.getWidth() - 8) / 2;
    deleteButton_->setBounds(footerRow.removeFromLeft(buttonWidth).reduced(0, 4));
    footerRow.removeFromLeft(8);
    footerCloseButton_->setBounds(footerRow.reduced(0, 4));
}

void OscillatorConfigPopup::themeChanged(const ColorTheme& newTheme)
{
    // Name editor
    nameEditor_->setColour(juce::TextEditor::backgroundColourId, newTheme.controlBackground);
    nameEditor_->setColour(juce::TextEditor::textColourId, newTheme.textPrimary);
    nameEditor_->setColour(juce::TextEditor::outlineColourId, newTheme.controlBorder);

    // Visibility button
    visibilityButton_->setColour(juce::TextButton::buttonColourId, newTheme.controlBackground);
    visibilityButton_->setColour(juce::TextButton::textColourOffId,
                                  visible_ ? newTheme.textPrimary : newTheme.textSecondary);

    // Close button
    closeButton_->setColour(juce::TextButton::buttonColourId, newTheme.controlBackground);
    closeButton_->setColour(juce::TextButton::textColourOffId, newTheme.textSecondary);

    // Labels
    auto styleLabel = [&newTheme](juce::Label* label, bool isHeader = false) {
        label->setColour(juce::Label::textColourId, isHeader ? newTheme.textPrimary : newTheme.textSecondary);
        label->setFont(juce::FontOptions(isHeader ? 13.0f : 12.0f));
    };

    styleLabel(sourceLabel_.get(), true);
    styleLabel(modeLabel_.get(), true);
    styleLabel(colorLabel_.get(), true);
    styleLabel(lineWidthLabel_.get());
    styleLabel(opacityLabel_.get());
    styleLabel(scaleLabel_.get());
    styleLabel(offsetLabel_.get());
    styleLabel(paneLabel_.get());

    // Sliders
    auto styleSlider = [&newTheme](juce::Slider* slider) {
        slider->setColour(juce::Slider::backgroundColourId, newTheme.controlBackground);
        slider->setColour(juce::Slider::trackColourId, newTheme.controlActive);
        slider->setColour(juce::Slider::thumbColourId, newTheme.controlHighlight);
        slider->setColour(juce::Slider::textBoxTextColourId, newTheme.textPrimary);
        slider->setColour(juce::Slider::textBoxBackgroundColourId, newTheme.controlBackground);
        slider->setColour(juce::Slider::textBoxOutlineColourId, newTheme.controlBorder);
    };

    styleSlider(lineWidthSlider_.get());
    styleSlider(opacitySlider_.get());
    styleSlider(scaleSlider_.get());
    styleSlider(offsetSlider_.get());

    // Pane selector
    paneSelector_->setColour(juce::ComboBox::backgroundColourId, newTheme.controlBackground);
    paneSelector_->setColour(juce::ComboBox::textColourId, newTheme.textPrimary);
    paneSelector_->setColour(juce::ComboBox::outlineColourId, newTheme.controlBorder);
    paneSelector_->setColour(juce::ComboBox::arrowColourId, newTheme.textSecondary);

    // Delete button (red/destructive)
    deleteButton_->setColour(juce::TextButton::buttonColourId, newTheme.statusError.withAlpha(0.2f));
    deleteButton_->setColour(juce::TextButton::textColourOffId, newTheme.statusError);

    // Footer Close button (neutral style)
    footerCloseButton_->setColour(juce::TextButton::buttonColourId, newTheme.controlBackground);
    footerCloseButton_->setColour(juce::TextButton::textColourOffId, newTheme.textPrimary);

    repaint();
}

void OscillatorConfigPopup::showForOscillator(const Oscillator& oscillator)
{
    updateFromOscillator(oscillator);
    setVisible(true);
    toFront(true);
    nameEditor_->grabKeyboardFocus();
}

void OscillatorConfigPopup::updateFromOscillator(const Oscillator& oscillator)
{
    oscillatorId_ = oscillator.getId();
    sourceId_ = oscillator.getSourceId();
    processingMode_ = oscillator.getProcessingMode();
    colour_ = oscillator.getColour();
    opacity_ = oscillator.getOpacity();
    visible_ = oscillator.isVisible();
    name_ = oscillator.getName();
    lineWidth_ = oscillator.getLineWidth();
    verticalScale_ = oscillator.getVerticalScale();
    verticalOffset_ = oscillator.getVerticalOffset();
    paneId_ = oscillator.getPaneId();
    orderIndex_ = oscillator.getOrderIndex();

    // Update controls
    nameEditor_->setText(name_, juce::dontSendNotification);
    sourceSelector_->setSelectedSourceId(sourceId_);
    modeButtons_->setSelectedId(static_cast<int>(processingMode_));
    lineWidthSlider_->setValue(lineWidth_, juce::dontSendNotification);
    opacitySlider_->setValue(opacity_ * 100.0, juce::dontSendNotification);
    scaleSlider_->setValue(verticalScale_, juce::dontSendNotification);
    offsetSlider_->setValue(verticalOffset_, juce::dontSendNotification);

    // Update pane selector
    for (int i = 0; i < paneSelector_->getNumItems(); ++i)
    {
        // Find matching pane ID
        if (static_cast<size_t>(i) < availablePanes_.size() && availablePanes_[i].first == paneId_)
        {
            paneSelector_->setSelectedItemIndex(i, juce::dontSendNotification);
            break;
        }
    }

    // Update visibility button state
    visibilityButton_->setColour(juce::TextButton::textColourOffId,
        visible_ ? ThemeManager::getInstance().getCurrentTheme().textPrimary
                 : ThemeManager::getInstance().getCurrentTheme().textSecondary);

    repaint();
}

void OscillatorConfigPopup::setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes)
{
    availablePanes_ = panes;
    paneSelector_->clear(juce::dontSendNotification);

    for (size_t i = 0; i < panes.size(); ++i)
    {
        paneSelector_->addItem(panes[i].second, static_cast<int>(i + 1));
    }
}

void OscillatorConfigPopup::notifyConfigChanged()
{
    // Build an Oscillator with all current configuration values
    // Use a ValueTree round-trip to properly set all properties

    juce::ValueTree state("Oscillator");
    state.setProperty("id", oscillatorId_.id, nullptr);
    state.setProperty("sourceId", sourceId_.id, nullptr);
    state.setProperty("oscillatorState", sourceId_.isValid() ? 0 : 1, nullptr);  // ACTIVE=0, NO_SOURCE=1
    state.setProperty("processingMode", processingModeToString(processingMode_), nullptr);
    state.setProperty("colour", static_cast<int>(colour_.getARGB()), nullptr);
    state.setProperty("opacity", opacity_, nullptr);
    state.setProperty("visible", visible_, nullptr);
    state.setProperty("name", name_, nullptr);
    state.setProperty("lineWidth", lineWidth_, nullptr);
    state.setProperty("verticalScale", verticalScale_, nullptr);
    state.setProperty("verticalOffset", verticalOffset_, nullptr);
    state.setProperty("paneId", paneId_.id, nullptr);
    state.setProperty("order", orderIndex_, nullptr);
    state.setProperty("schemaVersion", Oscillator::CURRENT_SCHEMA_VERSION, nullptr);

    Oscillator updated(state);

    listeners_.call([this, &updated](Listener& l) {
        l.oscillatorConfigChanged(oscillatorId_, updated);
    });
}

void OscillatorConfigPopup::handleClose()
{
    setVisible(false);
    listeners_.call([](Listener& l) { l.configPopupClosed(); });
}

void OscillatorConfigPopup::handleDelete()
{
    setVisible(false);
    listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); });
}

void OscillatorConfigPopup::handleNameEdit()
{
    auto newName = nameEditor_->getText();
    if (Oscillator::isValidName(newName) && newName != name_)
    {
        name_ = newName;
        notifyConfigChanged();
    }
}

void OscillatorConfigPopup::handleVisibilityToggle()
{
    visible_ = !visible_;
    visibilityButton_->setColour(juce::TextButton::textColourOffId,
        visible_ ? ThemeManager::getInstance().getCurrentTheme().textPrimary
                 : ThemeManager::getInstance().getCurrentTheme().textSecondary);
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleSourceChange(const SourceId& sourceId)
{
    sourceId_ = sourceId;
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleProcessingModeChange(int modeId)
{
    processingMode_ = static_cast<ProcessingMode>(modeId);
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleColorSelect(juce::Colour colour)
{
    colour_ = colour;
    repaint();
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleLineWidthChange()
{
    lineWidth_ = static_cast<float>(lineWidthSlider_->getValue());
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleOpacityChange()
{
    opacity_ = static_cast<float>(opacitySlider_->getValue()) / 100.0f;
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleVerticalScaleChange()
{
    verticalScale_ = static_cast<float>(scaleSlider_->getValue());
    notifyConfigChanged();
}

void OscillatorConfigPopup::handleVerticalOffsetChange()
{
    verticalOffset_ = static_cast<float>(offsetSlider_->getValue());
    notifyConfigChanged();
}

void OscillatorConfigPopup::handlePaneChange()
{
    int index = paneSelector_->getSelectedItemIndex();
    if (index >= 0 && static_cast<size_t>(index) < availablePanes_.size())
    {
        paneId_ = availablePanes_[index].first;
        notifyConfigChanged();
    }
}

void OscillatorConfigPopup::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OscillatorConfigPopup::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

} // namespace oscil
