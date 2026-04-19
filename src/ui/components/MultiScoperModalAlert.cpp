/*
    MultiScoper - Alert Modal Static Helpers
    RAII-based MultiScoperAlertModal::show and confirm implementations
*/

#include "ui/components/MultiScoperButton.h"
#include "ui/components/MultiScoperModal.h"

namespace multiscoper
{

// RAII container for alert modal resources.
// shared_ptr captured by the onClose lambda guarantees cleanup even if
// the deferred callback never runs (e.g. during MessageManager shutdown).
struct AlertResources
{
    std::unique_ptr<juce::Label> label;
    std::unique_ptr<MultiScoperButton> okButton;
    std::unique_ptr<juce::Component> content;
    std::unique_ptr<MultiScoperModal> modal;

    ~AlertResources()
    {
        if (content)
            content->removeAllChildren();
    }
};

void MultiScoperAlertModal::show(IThemeService& themeService, const juce::String& title, const juce::String& message,
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

    res->okButton = std::make_unique<MultiScoperButton>(themeService, "OK");
    res->okButton->setVariant(ButtonVariant::Primary);
    res->okButton->setBounds(400 - 80, 70, 80, 30);
    res->content->addAndMakeVisible(res->okButton.get());

    res->modal = std::make_unique<MultiScoperModal>(themeService, title);
    res->modal->setContent(res->content.get());
    res->modal->setSize(ModalSize::Small);

    auto* modalPtr = res->modal.get();
    res->modal->onClose = [res, onOk]() mutable {
        if (onOk)
            onOk();
        // Break the circular reference (AlertResources→modal→onClose→AlertResources)
        // by moving the shared_ptr out of the lambda capture first. This ensures
        // AlertResources cannot be destroyed while still inside the callAsync
        // argument construction (which would cascade-destroy this lambda).
        auto prevent_destruction = std::move(res);
        if (juce::MessageManager::getInstanceWithoutCreating() == nullptr)
            return;
        // Defer destruction to the next message loop iteration.  If callAsync fails
        // (shutdown), the argument lambda is destroyed synchronously — safe because
        // MultiScoperModal copies onClose before invoking, so this lambda lives on the
        // caller's stack.
        juce::MessageManager::callAsync([prevent = std::move(prevent_destruction)]() { juce::ignoreUnused(prevent); });
    };

    res->okButton->onClick = [modalPtr]() { modalPtr->hide(); };
    res->modal->show();
}

} // namespace multiscoper
