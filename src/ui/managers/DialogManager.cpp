/*
    MultiScoper - Dialog Manager Implementation
*/

#include "ui/managers/DialogManager.h"

#include "core/MultiScoperLog.h"

namespace multiscoper
{

DialogManager::DialogManager(juce::Component& parent, IThemeService& themeService, IInstanceRegistry& instanceRegistry)
    : parent_(parent)
    , themeService_(themeService)
    , instanceRegistry_(instanceRegistry)
{
    parent_.addComponentListener(this);

    // Initialize Add Oscillator Dialog
    addOscillatorDialogContent_ = std::make_unique<AddOscillatorDialog>(themeService_);
    addOscillatorModal_ = std::make_unique<MultiScoperModal>(themeService_, "Add Oscillator", "addOscillatorModal");
    addOscillatorModal_->setContent(addOscillatorDialogContent_.get());

    // Initialize Color Dialog
    colorDialogContent_ = std::make_unique<OscillatorColorDialog>(themeService_);
    colorModal_ = std::make_unique<MultiScoperModal>(themeService_, "Select Color", "colorDialogModal");
    colorModal_->setContent(colorDialogContent_.get());

    // Initialize Select Pane Dialog
    selectPaneDialogContent_ = std::make_unique<SelectPaneDialog>(themeService_);
    selectPaneModal_ = std::make_unique<MultiScoperModal>(themeService_, "Select Pane", "selectPaneModal");
    selectPaneModal_->setContent(selectPaneDialogContent_.get());

    // Initialize Config Popup (Content)
    configPopup_ = std::make_unique<OscillatorConfigDialog>(themeService_, instanceRegistry_);

    // Initialize Config Modal
    configModal_ = std::make_unique<MultiScoperModal>(themeService_, "Configure Oscillator", "configModal");
    configModal_->setContent(configPopup_.get());

    // Close wiring: the content's footer Close button asks the modal to hide.
    // Every close path (footer / X / Escape / backdrop / animation-settle)
    // terminates at the modal's onClose, which we route back to the content's
    // onExternalClose(). That is the single point that flushes pending edits
    // and fires the configDialogClosed listener notification.
    configPopup_->onClose = [this]() { configModal_->hide(); };
    configModal_->onClose = [this]() {
        if (configPopup_)
            configPopup_->onExternalClose();
    };
}

DialogManager::~DialogManager()
{
    if (parentAlive_)
        parent_.removeComponentListener(this);
}

void DialogManager::showAddOscillatorDialog(const std::vector<SourceInfo>& sources, const std::vector<Pane>& panes,
                                            std::function<void(const AddOscillatorDialog::Result&)> onComplete)
{
    MULTISCOPER_LOG(DIALOG, "showAddOscillatorDialog: " << sources.size() << " sources, " << panes.size() << " panes");
    if (!addOscillatorModal_ || !addOscillatorDialogContent_)
        return;

    addOscillatorDialogContent_->setData(sources, panes);

    addOscillatorDialogContent_->setOnComplete([this, onComplete](const AddOscillatorDialog::Result& result) {
        if (onComplete)
            onComplete(result);
        addOscillatorModal_->hide();
    });

    addOscillatorDialogContent_->setOnCancel([this]() { addOscillatorModal_->hide(); });

    addOscillatorModal_->show(&parent_);
}

void DialogManager::showColorDialog(juce::Colour initialColor, std::function<void(juce::Colour)> onColorSelected)
{
    MULTISCOPER_LOG(DIALOG, "showColorDialog: initialColor=#" << initialColor.toDisplayString(false));
    if (!colorModal_ || !colorDialogContent_)
        return;

    colorDialogContent_->setColors(WaveformColorPalette::getAllColors());
    colorDialogContent_->setSelectedColor(initialColor);

    colorDialogContent_->setOnColorSelected([this, onColorSelected](juce::Colour color) {
        if (onColorSelected)
            onColorSelected(color);
        colorModal_->hide();
    });

    colorDialogContent_->setOnCancel([this]() { colorModal_->hide(); });

    colorModal_->show(&parent_);
}

void DialogManager::showSelectPaneDialog(const std::vector<Pane>& availablePanes,
                                         std::function<void(const SelectPaneDialog::Result&)> onComplete,
                                         std::function<void()> onCancel)
{
    MULTISCOPER_LOG(DIALOG, "showSelectPaneDialog: " << availablePanes.size() << " panes");
    if (!selectPaneModal_ || !selectPaneDialogContent_)
        return;

    selectPaneDialogContent_->setAvailablePanes(availablePanes);

    selectPaneDialogContent_->setOnComplete([this, onComplete](const SelectPaneDialog::Result& result) {
        if (onComplete)
            onComplete(result);
        selectPaneModal_->hide();
    });

    selectPaneDialogContent_->setOnCancel([this, onCancel]() {
        if (onCancel)
            onCancel();
        selectPaneModal_->hide();
    });

    selectPaneModal_->show(&parent_);
}

void DialogManager::showConfigPopup(const Oscillator& oscillator,
                                    const std::vector<std::pair<PaneId, juce::String>>& availablePanes)
{
    MULTISCOPER_LOG(DIALOG, "showConfigPopup: oscId=" << oscillator.getId().id << " name=" << oscillator.getName()
                                                      << " panes=" << availablePanes.size());
    if (!configPopup_ || !configModal_)
        return;

    configPopup_->setAvailablePanes(availablePanes);
    configPopup_->showForOscillator(oscillator);

    configModal_->show(&parent_);
}

void DialogManager::closeConfigPopup()
{
    MULTISCOPER_LOG(DIALOG, "closeConfigPopup");
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

void DialogManager::componentBeingDeleted(juce::Component& component)
{
    if (&component == &parent_)
    {
        // Parent is dying — force synchronous teardown. Animated hide() would
        // leave a timer alive on an orphaned component tree.
        if (addOscillatorModal_)
            addOscillatorModal_->hideImmediate();
        if (colorModal_)
            colorModal_->hideImmediate();
        if (selectPaneModal_)
            selectPaneModal_->hideImmediate();
        if (configModal_)
            configModal_->hideImmediate();

        // Unregister now so the destructor doesn't touch a dangling reference.
        parent_.removeComponentListener(this);
        parentAlive_ = false;
    }
}

} // namespace multiscoper
