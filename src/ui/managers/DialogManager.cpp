/*
    Oscil - Dialog Manager Implementation
*/

#include "ui/managers/DialogManager.h"

namespace oscil
{

DialogManager::DialogManager(juce::Component& parent, IThemeService& themeService, IInstanceRegistry& instanceRegistry,
                             VisualPresetManager& presetManager)
    : parent_(parent)
    , themeService_(themeService)
    , instanceRegistry_(instanceRegistry)
    , presetManager_(presetManager)
{
    parent_.addComponentListener(this);

    // Initialize Add Oscillator Dialog
    addOscillatorDialogContent_ = std::make_unique<AddOscillatorDialog>(themeService_);
    juce::WeakReference<DialogManager> weakThis(this);
    addOscillatorDialogContent_->setOnBrowsePresets([weakThis](const juce::String& currentId, auto onSelected) {
        if (auto* manager = weakThis.get())
            manager->showPresetBrowserDialog(currentId, onSelected);
    });
    addOscillatorModal_ = std::make_unique<OscilModal>(themeService_, "Add Oscillator", "addOscillatorModal");
    addOscillatorModal_->setContent(addOscillatorDialogContent_.get());

    // Initialize Color Dialog
    colorDialogContent_ = std::make_unique<OscillatorColorDialog>(themeService_);
    colorModal_ = std::make_unique<OscilModal>(themeService_, "Select Color", "colorDialogModal");
    colorModal_->setContent(colorDialogContent_.get());

    // Initialize Select Pane Dialog
    selectPaneDialogContent_ = std::make_unique<SelectPaneDialog>(themeService_);
    selectPaneModal_ = std::make_unique<OscilModal>(themeService_, "Select Pane", "selectPaneModal");
    selectPaneModal_->setContent(selectPaneDialogContent_.get());

    // Initialize Config Popup (Content)
    configPopup_ = std::make_unique<OscillatorConfigDialog>(themeService_, instanceRegistry_);
    configPopup_->setOnBrowsePresets([weakThis](const juce::String& currentId, auto onSelected) {
        if (auto* manager = weakThis.get())
            manager->showPresetBrowserDialog(currentId, onSelected);
    });

    // Initialize Config Modal
    configModal_ = std::make_unique<OscilModal>(themeService_, "Configure Oscillator", "configModal");
    configModal_->setContent(configPopup_.get());

    // Connect internal close button/action to modal hide
    configPopup_->onClose = [weakThis]() {
        if (auto* manager = weakThis.get())
            manager->configModal_->hide();
    };

    // Initialize Preset Browser Dialog
    presetBrowserDialogContent_ = std::make_unique<PresetBrowserDialog>(themeService_, presetManager_);
    presetBrowserModal_ = std::make_unique<OscilModal>(themeService_, "Browse Presets", "presetBrowserModal");
    presetBrowserModal_->setContent(presetBrowserDialogContent_.get());
}

DialogManager::~DialogManager()
{
    parent_.removeComponentListener(this);
}

void DialogManager::showAddOscillatorDialog(const std::vector<SourceInfo>& sources,
                                            const std::vector<Pane>& panes,
                                            std::function<void(const AddOscillatorDialog::Result&)> onComplete)
{
    if (!addOscillatorModal_ || !addOscillatorDialogContent_)
        return;

    addOscillatorDialogContent_->setData(sources, panes);

    juce::WeakReference<DialogManager> weakThis(this);
    addOscillatorDialogContent_->setOnComplete([weakThis, onComplete](const AddOscillatorDialog::Result& result) {
        if (onComplete)
            onComplete(result);
        if (auto* manager = weakThis.get())
            manager->addOscillatorModal_->hide();
    });

    addOscillatorDialogContent_->setOnCancel([weakThis]() {
        if (auto* manager = weakThis.get())
            manager->addOscillatorModal_->hide();
    });

    addOscillatorModal_->show(&parent_);
}

void DialogManager::showColorDialog(juce::Colour initialColor,
                                    std::function<void(juce::Colour)> onColorSelected)
{
    if (!colorModal_ || !colorDialogContent_)
        return;

    colorDialogContent_->setColors(WaveformColorPalette::getAllColors());
    colorDialogContent_->setSelectedColor(initialColor);

    juce::WeakReference<DialogManager> weakThis(this);
    colorDialogContent_->setOnColorSelected([weakThis, onColorSelected](juce::Colour color) {
        if (onColorSelected)
            onColorSelected(color);
        if (auto* manager = weakThis.get())
            manager->colorModal_->hide();
    });

    colorDialogContent_->setOnCancel([weakThis]() {
        if (auto* manager = weakThis.get())
            manager->colorModal_->hide();
    });

    colorModal_->show(&parent_);
}

void DialogManager::showSelectPaneDialog(const std::vector<Pane>& availablePanes,
                                         std::function<void(const SelectPaneDialog::Result&)> onComplete,
                                         std::function<void()> onCancel)
{
    if (!selectPaneModal_ || !selectPaneDialogContent_)
        return;

    selectPaneDialogContent_->setAvailablePanes(availablePanes);

    juce::WeakReference<DialogManager> weakThis(this);
    selectPaneDialogContent_->setOnComplete([weakThis, onComplete](const SelectPaneDialog::Result& result) {
        if (onComplete)
            onComplete(result);
        if (auto* manager = weakThis.get())
            manager->selectPaneModal_->hide();
    });

    selectPaneDialogContent_->setOnCancel([weakThis, onCancel]() {
        if (onCancel)
            onCancel();
        if (auto* manager = weakThis.get())
            manager->selectPaneModal_->hide();
    });

    selectPaneModal_->show(&parent_);
}

void DialogManager::showConfigPopup(const Oscillator& oscillator,
                                    const std::vector<std::pair<PaneId, juce::String>>& availablePanes)
{
    if (!configPopup_ || !configModal_)
        return;

    configPopup_->setAvailablePanes(availablePanes);
    configPopup_->showForOscillator(oscillator);
    
    configModal_->show(&parent_);
}

void DialogManager::closeConfigPopup()
{
    if (configModal_)
        configModal_->hide();
}

bool DialogManager::isConfigPopupVisibleFor(const OscillatorId& oscillatorId) const
{
    if (configModal_ && configModal_->isShowing() && configPopup_)
    {
        return configPopup_->getOscillatorId() == oscillatorId;
    }
    return false;
}

void DialogManager::addConfigPopupListener(OscillatorConfigDialog::Listener* listener)
{
    if (configPopup_)
        configPopup_->addListener(listener);
}

void DialogManager::removeConfigPopupListener(OscillatorConfigDialog::Listener* listener)
{
    if (configPopup_)
        configPopup_->removeListener(listener);
}

void DialogManager::showPresetBrowserDialog(const juce::String& currentPresetId,
                                            std::function<void(const juce::String& selectedPresetId)> onPresetSelected)
{
    if (!presetBrowserModal_ || !presetBrowserDialogContent_)
        return;

    presetBrowserDialogContent_->refreshPresets();
    presetBrowserDialogContent_->setSelectedPresetId(currentPresetId);

    juce::WeakReference<DialogManager> weakThis(this);
    presetBrowserDialogContent_->setOnPresetSelected([weakThis, onPresetSelected](const juce::String& presetId) {
        if (onPresetSelected)
            onPresetSelected(presetId);
        if (auto* manager = weakThis.get())
            manager->presetBrowserModal_->hide();
    });

    presetBrowserDialogContent_->setOnCancel([weakThis]() {
        if (auto* manager = weakThis.get())
            manager->presetBrowserModal_->hide();
    });

    // Show on top of everything (including other modals)
    presetBrowserModal_->show(&parent_);
    presetBrowserModal_->toFront(true);
}

void DialogManager::componentBeingDeleted(juce::Component& component)
{
    if (&component == &parent_)
    {
        // Parent is dying, ensure modals are closed/detached
        if (addOscillatorModal_) addOscillatorModal_->hide();
        if (colorModal_) colorModal_->hide();
        if (selectPaneModal_) selectPaneModal_->hide();
        if (configModal_) configModal_->hide();
        if (presetBrowserModal_) presetBrowserModal_->hide();
    }
}

} // namespace oscil
