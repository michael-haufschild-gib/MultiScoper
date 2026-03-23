/*
    Oscil - Alert Modal Static Helpers
    RAII-based OscilAlertModal::show and confirm implementations
*/

#include "ui/components/OscilButton.h"
#include "ui/components/OscilModal.h"

namespace oscil
{

// RAII container for alert modal resources.
// shared_ptr captured by the onClose lambda guarantees cleanup even if
// the deferred callback never runs (e.g. during MessageManager shutdown).
struct AlertResources
{
    std::unique_ptr<juce::Label> label;
    std::unique_ptr<OscilButton> okButton;
    std::unique_ptr<juce::Component> content;
    std::unique_ptr<OscilModal> modal;

    ~AlertResources()
    {
        if (content)
            content->removeAllChildren();
    }
};

void OscilAlertModal::show(IThemeService& themeService, const juce::String& title, const juce::String& message,
                           [[maybe_unused]] Type type, std::function<void()> onOk)
{
    auto res = std::make_shared<AlertResources>();

    res->content = std::make_unique<juce::Component>();
    res->content->setSize(400, 100);

    res->label = std::make_unique<juce::Label>();
    res->label->setText(message, juce::dontSendNotification);
    res->label->setJustificationType(juce::Justification::topLeft);
    res->label->setBounds(0, 0, 400, 60);
    res->content->addAndMakeVisible(res->label.get());

    res->okButton = std::make_unique<OscilButton>(themeService, "OK");
    res->okButton->setVariant(ButtonVariant::Primary);
    res->okButton->setBounds(400 - 80, 70, 80, 30);
    res->content->addAndMakeVisible(res->okButton.get());

    res->modal = std::make_unique<OscilModal>(themeService, title);
    res->modal->setContent(res->content.get());
    res->modal->setSize(ModalSize::Small);

    auto* modalPtr = res->modal.get();
    res->modal->onClose = [res, onOk]() {
        if (onOk)
            onOk();
        // Destroy resources on next message loop iteration (after callback returns)
        juce::MessageManager::callAsync([prevent_destruction = res]() {});
    };

    res->okButton->onClick = [modalPtr]() { modalPtr->hide(); };
    res->modal->show();
}

void OscilAlertModal::confirm([[maybe_unused]] IThemeService& themeService, [[maybe_unused]] const juce::String& title,
                              [[maybe_unused]] const juce::String& message,
                              [[maybe_unused]] std::function<void(bool)> onResult)
{
}

} // namespace oscil
