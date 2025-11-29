/*
    Oscil - Oscillator Configuration Popup Implementation
    Modal popup for comprehensive oscillator configuration
*/

#include "ui/OscillatorConfigPopup.h"
#include "ui/SourceSelectorComponent.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/components/ProcessingModeIcons.h"
#include "rendering/ShaderRegistry.h"

namespace oscil
{

OscillatorConfigPopup::OscillatorConfigPopup()
{
    OSCIL_REGISTER_TEST_ID("configPopup");
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

OscillatorConfigPopup::~OscillatorConfigPopup()
{
    OSCIL_UNREGISTER_CHILD_TEST_ID("configPopup_sourceDropdown");
    OSCIL_UNREGISTER_CHILD_TEST_ID("configPopup_modeSelector");
    ThemeManager::getInstance().removeListener(this);
}

void OscillatorConfigPopup::setupComponents()
{
    // Header: Name editor (OscilTextField)
    nameEditor_ = std::make_unique<OscilTextField>(TextFieldVariant::Text, "configPopup_nameField");
    nameEditor_->setPlaceholder("Oscillator Name");
    nameEditor_->onReturnPressed = [this]() { handleNameEdit(); };
    // OscilTextField doesn't have onFocusLost - use onTextChanged for immediate feedback
    nameEditor_->onTextChanged = [this](const juce::String&) { handleNameEdit(); };
    addAndMakeVisible(*nameEditor_);

    // Header: Visibility toggle (OscilToggle for visibility on/off)
    visibilityToggle_ = std::make_unique<OscilToggle>("Visible");
    visibilityToggle_->setValue(true, false);
    visibilityToggle_->onValueChanged = [this](bool) { handleVisibilityToggle(); };
    addAndMakeVisible(*visibilityToggle_);

    // Header: Close button (OscilButton)
    closeButton_ = std::make_unique<OscilButton>("X", "configPopup_closeBtn");
    closeButton_->setVariant(ButtonVariant::Ghost);
    closeButton_->onClick = [this]() { handleClose(); };
    addAndMakeVisible(*closeButton_);

    // Source section
    sourceLabel_ = std::make_unique<juce::Label>("", "Source");
    addAndMakeVisible(*sourceLabel_);

    sourceSelector_ = std::make_unique<SourceSelectorComponent>();
    sourceSelector_->onSelectionChanged([this](const SourceId& id) { handleSourceChange(id); });
    addAndMakeVisible(*sourceSelector_);
    OSCIL_REGISTER_CHILD_TEST_ID(*sourceSelector_, "configPopup_sourceDropdown");

    // Processing mode section
    modeLabel_ = std::make_unique<juce::Label>("", "Processing Mode");
    addAndMakeVisible(*modeLabel_);

    modeButtons_ = std::make_unique<SegmentedButtonBar>();
    modeButtons_->setMinButtonWidth(40);  // Compact buttons for 6 modes
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(16), static_cast<int>(ProcessingMode::FullStereo), "configPopup_modeSelector_stereo");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(16), static_cast<int>(ProcessingMode::Mono), "configPopup_modeSelector_mono");
    modeButtons_->addButton("M", static_cast<int>(ProcessingMode::Mid), "configPopup_modeSelector_mid");
    modeButtons_->addButton("S", static_cast<int>(ProcessingMode::Side), "configPopup_modeSelector_side");
    modeButtons_->addButton("L", static_cast<int>(ProcessingMode::Left), "configPopup_modeSelector_left");
    modeButtons_->addButton("R", static_cast<int>(ProcessingMode::Right), "configPopup_modeSelector_right");
    modeButtons_->onSelectionChanged = [this](int id) { handleProcessingModeChange(id); };
    addAndMakeVisible(*modeButtons_);
    OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, "configPopup_modeSelector");

    // Color swatches (OscilColorSwatches)
    colorLabel_ = std::make_unique<juce::Label>("", "Color Picker");
    addAndMakeVisible(*colorLabel_);

    colorSwatches_ = std::make_unique<OscilColorSwatches>("configPopup_colorPicker");
    std::vector<juce::Colour> colors(defaultColors_.begin(), defaultColors_.end());
    colorSwatches_->setColors(colors);
    colorSwatches_->onColorSelected = [this](int, juce::Colour color) { handleColorSelect(color); };
    addAndMakeVisible(*colorSwatches_);

    // Shader dropdown
    shaderLabel_ = std::make_unique<juce::Label>("", "Shader");
    addAndMakeVisible(*shaderLabel_);

    shaderDropdown_ = std::make_unique<OscilDropdown>("", "configPopup_shaderDropdown");
    // Populate from shader registry
    auto availableShaders = ShaderRegistry::getInstance().getAvailableShaders();
    for (const auto& shaderInfo : availableShaders)
    {
        shaderDropdown_->addItem(shaderInfo.displayName, shaderInfo.id);
    }
    shaderDropdown_->onSelectionChanged = [this](int) { handleShaderChange(); };
    addAndMakeVisible(*shaderDropdown_);

    // Line Width slider (OscilSlider with label)
    lineWidthSlider_ = std::make_unique<OscilSlider>("configPopup_lineWidthSlider");
    lineWidthSlider_->setLabel("Line Width");
    lineWidthSlider_->setRange(Oscillator::MIN_LINE_WIDTH, Oscillator::MAX_LINE_WIDTH);
    lineWidthSlider_->setStep(0.1);
    lineWidthSlider_->setValue(Oscillator::DEFAULT_LINE_WIDTH, false);
    lineWidthSlider_->setSuffix(" px");
    lineWidthSlider_->onValueChanged = [this](double) { handleLineWidthChange(); };
    addAndMakeVisible(*lineWidthSlider_);

    // Opacity slider (OscilSlider with label)
    opacitySlider_ = std::make_unique<OscilSlider>("configPopup_opacitySlider");
    opacitySlider_->setLabel("Opacity");
    opacitySlider_->setRange(0.0, 100.0);
    opacitySlider_->setStep(1.0);
    opacitySlider_->setValue(100.0, false);
    opacitySlider_->setSuffix(" %");
    opacitySlider_->onValueChanged = [this](double) { handleOpacityChange(); };
    addAndMakeVisible(*opacitySlider_);

    // Vertical Scale slider (OscilSlider with label)
    scaleSlider_ = std::make_unique<OscilSlider>("configPopup_verticalScaleSlider");
    scaleSlider_->setLabel("Vertical Scale");
    scaleSlider_->setRange(Oscillator::MIN_VERTICAL_SCALE, Oscillator::MAX_VERTICAL_SCALE);
    scaleSlider_->setStep(0.1);
    scaleSlider_->setValue(Oscillator::DEFAULT_VERTICAL_SCALE, false);
    scaleSlider_->setSuffix("x");
    scaleSlider_->onValueChanged = [this](double) { handleVerticalScaleChange(); };
    addAndMakeVisible(*scaleSlider_);

    // Vertical Offset slider (OscilSlider with label)
    offsetSlider_ = std::make_unique<OscilSlider>("configPopup_verticalOffsetSlider");
    offsetSlider_->setLabel("Vertical Offset");
    offsetSlider_->setRange(Oscillator::MIN_VERTICAL_OFFSET, Oscillator::MAX_VERTICAL_OFFSET);
    offsetSlider_->setStep(0.01);
    offsetSlider_->setValue(Oscillator::DEFAULT_VERTICAL_OFFSET, false);
    offsetSlider_->onValueChanged = [this](double) { handleVerticalOffsetChange(); };
    addAndMakeVisible(*offsetSlider_);

    // Pane selector (OscilDropdown)
    paneLabel_ = std::make_unique<juce::Label>("", "Pane");
    addAndMakeVisible(*paneLabel_);

    paneSelector_ = std::make_unique<OscilDropdown>("", "configPopup_paneDropdown");
    paneSelector_->onSelectionChanged = [this](int) { handlePaneChange(); };
    addAndMakeVisible(*paneSelector_);

    // Footer: Delete button (OscilButton with Danger variant)
    deleteButton_ = std::make_unique<OscilButton>("Delete Oscillator", "configPopup_deleteBtn");
    deleteButton_->setVariant(ButtonVariant::Danger);
    deleteButton_->onClick = [this]() { handleDelete(); };
    addAndMakeVisible(*deleteButton_);

    // Footer: Close button (OscilButton with secondary variant)
    footerCloseButton_ = std::make_unique<OscilButton>("Close");
    footerCloseButton_->setVariant(ButtonVariant::Secondary);
    footerCloseButton_->onClick = [this]() { handleClose(); };
    addAndMakeVisible(*footerCloseButton_);

    // Components handle their own theming - just trigger initial update
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
        static_cast<float>(getHeight()) / 2.0f - POPUP_HEIGHT / 2.0f + 12,
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
    visibilityToggle_->setBounds(headerRow.removeFromRight(60));  // OscilToggle needs more width
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

    // Color swatches (single OscilColorSwatches component)
    colorLabel_->setBounds(contentBounds.removeFromTop(20));
    colorSwatches_->setBounds(contentBounds.removeFromTop(32));
    contentBounds.removeFromTop(12);

    // Shader dropdown
    auto shaderRow = contentBounds.removeFromTop(32);
    shaderLabel_->setBounds(shaderRow.removeFromLeft(100));
    shaderDropdown_->setBounds(shaderRow);
    contentBounds.removeFromTop(8);

    // OscilSlider components have integrated labels - use full row height
    lineWidthSlider_->setBounds(contentBounds.removeFromTop(40));
    contentBounds.removeFromTop(4);

    opacitySlider_->setBounds(contentBounds.removeFromTop(40));
    contentBounds.removeFromTop(4);

    scaleSlider_->setBounds(contentBounds.removeFromTop(40));
    contentBounds.removeFromTop(4);

    offsetSlider_->setBounds(contentBounds.removeFromTop(40));
    contentBounds.removeFromTop(8);

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
    // Oscil components (OscilTextField, OscilToggle, OscilButton, OscilSlider,
    // OscilDropdown, OscilColorSwatches) handle their own theming automatically
    // via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    auto styleLabel = [&newTheme](juce::Label* label, bool isHeader = false) {
        label->setColour(juce::Label::textColourId, isHeader ? newTheme.textPrimary : newTheme.textSecondary);
        label->setFont(juce::FontOptions(isHeader ? 13.0f : 12.0f));
    };

    styleLabel(sourceLabel_.get(), true);
    styleLabel(modeLabel_.get(), true);
    styleLabel(colorLabel_.get(), true);
    styleLabel(shaderLabel_.get());
    styleLabel(paneLabel_.get());

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
    shaderId_ = oscillator.getShaderId();

    // Update controls using new Oscil component APIs
    nameEditor_->setText(name_, false);  // Don't notify
    sourceSelector_->setSelectedSourceId(sourceId_);
    modeButtons_->setSelectedId(static_cast<int>(processingMode_));
    lineWidthSlider_->setValue(lineWidth_, false);  // Don't notify
    opacitySlider_->setValue(opacity_ * 100.0, false);  // Don't notify
    scaleSlider_->setValue(verticalScale_, false);  // Don't notify
    offsetSlider_->setValue(verticalOffset_, false);  // Don't notify

    // Update visibility toggle
    visibilityToggle_->setValue(visible_, false);  // Don't animate on load

    // Update color swatches selection
    colorSwatches_->setSelectedColor(colour_);

    // Update shader dropdown selection
    auto availableShaders = ShaderRegistry::getInstance().getAvailableShaders();
    for (size_t i = 0; i < availableShaders.size(); ++i)
    {
        if (availableShaders[i].id == shaderId_)
        {
            shaderDropdown_->setSelectedIndex(static_cast<int>(i), false);  // Don't notify
            break;
        }
    }

    // Update pane selector (OscilDropdown uses setSelectedIndex)
    for (int i = 0; i < paneSelector_->getNumItems(); ++i)
    {
        if (static_cast<size_t>(i) < availablePanes_.size() && availablePanes_[static_cast<size_t>(i)].first == paneId_)
        {
            paneSelector_->setSelectedIndex(i, false);  // Don't notify
            break;
        }
    }

    repaint();
}

void OscillatorConfigPopup::setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes)
{
    availablePanes_ = panes;
    paneSelector_->clearItems();

    for (size_t i = 0; i < panes.size(); ++i)
    {
        // OscilDropdown addItem takes (label, id) where id is optional string
        paneSelector_->addItem(panes[i].second, panes[i].first.id);
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
    state.setProperty("shaderId", shaderId_, nullptr);
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
    // Get current state from the OscilToggle component
    visible_ = visibilityToggle_->getValue();
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
    // OscilDropdown uses getSelectedIndex() instead of getSelectedItemIndex()
    int index = paneSelector_->getSelectedIndex();
    if (index >= 0 && static_cast<size_t>(index) < availablePanes_.size())
    {
        paneId_ = availablePanes_[static_cast<size_t>(index)].first;
        notifyConfigChanged();
    }
}

void OscillatorConfigPopup::handleShaderChange()
{
    int index = shaderDropdown_->getSelectedIndex();
    auto availableShaders = ShaderRegistry::getInstance().getAvailableShaders();
    if (index >= 0 && static_cast<size_t>(index) < availableShaders.size())
    {
        shaderId_ = availableShaders[static_cast<size_t>(index)].id;
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
