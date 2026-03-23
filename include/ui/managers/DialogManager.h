/*
    Oscil - Dialog Manager
    Manages modal dialogs and popups to reduce PluginEditor complexity
*/

#pragma once

#include "core/OscilState.h"
#include "ui/components/OscilModal.h"
#include "ui/dialogs/AddOscillatorDialog.h"
#include "ui/dialogs/OscillatorColorDialog.h"
#include "ui/dialogs/OscillatorConfigDialog.h"
#include "ui/dialogs/SelectPaneDialog.h"

namespace oscil

{

class DialogManager : public juce::ComponentListener

{
public:
    /// Construct with parent component and service dependencies.
    DialogManager(juce::Component& parent, IThemeService& themeService, IInstanceRegistry& instanceRegistry);
    ~DialogManager() override;

    // Dialog methods

    void showAddOscillatorDialog(const std::vector<SourceInfo>& sources,

                                 const std::vector<Pane>& panes,

                                 std::function<void(const AddOscillatorDialog::Result&)> onComplete);

    void showColorDialog(juce::Colour initialColor,

                         std::function<void(juce::Colour)> onColorSelected);

    void showSelectPaneDialog(const std::vector<Pane>& availablePanes,
                              std::function<void(const SelectPaneDialog::Result&)> onComplete,
                              std::function<void()> onCancel = nullptr);

    // Configuration Popup
    void showConfigPopup(const Oscillator& oscillator,
                         const std::vector<std::pair<PaneId, juce::String>>& availablePanes);

    /// Close the oscillator config popup if open.
    void closeConfigPopup();
    /// Check if the config popup is currently showing for the given oscillator.
    bool isConfigPopupVisibleFor(const OscillatorId& oscillatorId) const;

    /// Register a listener for config popup events.
    void addConfigPopupListener(OscillatorConfigDialog::Listener* listener);
    /// Remove a config popup listener.
    void removeConfigPopupListener(OscillatorConfigDialog::Listener* listener);

    /// Handle component deletion (cleanup config popup reference).
    void componentBeingDeleted(juce::Component& component) override;

private:
    juce::Component& parent_;

    IThemeService& themeService_;

    IInstanceRegistry& instanceRegistry_;

    // Dialogs

    std::unique_ptr<OscilModal> addOscillatorModal_;

    std::unique_ptr<AddOscillatorDialog> addOscillatorDialogContent_;

    std::unique_ptr<OscilModal> colorModal_;

    std::unique_ptr<OscillatorColorDialog> colorDialogContent_;

    std::unique_ptr<OscilModal> selectPaneModal_;

    std::unique_ptr<SelectPaneDialog> selectPaneDialogContent_;

    std::unique_ptr<OscilModal> configModal_;

    std::unique_ptr<OscillatorConfigDialog> configPopup_;
};

} // namespace oscil
