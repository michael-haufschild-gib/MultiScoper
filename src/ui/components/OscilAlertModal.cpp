/*
    Oscil - Alert Modal Implementation
*/

#include "ui/components/OscilModal.h"
#include "ui/components/OscilButton.h"

namespace oscil
{

namespace
{
    // Helper component that owns its children via unique_ptr for RAII safety
    class AlertContent : public juce::Component
    {
    public:
        AlertContent(IThemeService& themeService, const juce::String& message)
        {
            setSize(400, 100);

            label_ = std::make_unique<juce::Label>();
            label_->setText(message, juce::dontSendNotification);
            label_->setJustificationType(juce::Justification::topLeft);
            label_->setBounds(0, 0, 400, 60);
            addAndMakeVisible(label_.get());

            okButton_ = std::make_unique<OscilButton>(themeService, "OK");
            okButton_->setVariant(ButtonVariant::Primary);
            okButton_->setBounds(400 - 80, 70, 80, 30);
            addAndMakeVisible(okButton_.get());
        }

        OscilButton* getOkButton() { return okButton_.get(); }

    private:
        std::unique_ptr<juce::Label> label_;
        std::unique_ptr<OscilButton> okButton_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlertContent)
    };
}

void OscilAlertModal::show(IThemeService& themeService,
                            const juce::String& title,
                            const juce::String& message,
                            [[maybe_unused]] Type type,
                            std::function<void()> onOk)
{
    // Create content with proper RAII ownership of children
    auto content = std::make_unique<AlertContent>(themeService, message);
    auto* okButton = content->getOkButton();

    // Create modal with unique_ptr - will be moved to prevent leaks
    auto modal = std::make_unique<OscilModal>(themeService, title);
    modal->setContentOwned(std::move(content));
    modal->setSize(ModalSize::Small);

    // Get raw pointer for lambdas before moving ownership
    auto* modalPtr = modal.get();

    // Set up self-deletion when modal is closed
    modal->onClose = [modalPtr, onOk]() {
        // Call user callback if provided
        if (onOk)
            onOk();

        // Schedule deletion on message thread to avoid deleting during callback
        // Modal owns content via ownedContent_, so deleting modal cleans up everything
        juce::MessageManager::callAsync([modalPtr]() {
            delete modalPtr;
        });
    };

    // Wire up OK button to close the modal
    okButton->onClick = [modalPtr]() {
        modalPtr->hide();
    };

    // Release ownership - modal will self-delete via onClose callback
    modal.release();
    modalPtr->show();
}

} // namespace oscil
