/*
    Oscil - Modal Component Implementation
    Migrated to JUCE Animator for VBlank-synced animations
*/

#include "ui/components/OscilModal.h"
#include "ui/components/OscilButton.h"
#include "ui/components/ListItemIcons.h"
#include "plugin/PluginEditor.h"

namespace oscil
{

OscilModal::OscilModal(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    setVisible(false);
    showProgress_ = 0.0f;
    scaleProgress_ = 0.0f;
}

OscilModal::OscilModal(IThemeService& themeService, const juce::String& title)
    : OscilModal(themeService)
{
    title_ = title;
}

OscilModal::OscilModal(IThemeService& themeService, const juce::String& title, const juce::String& testId)
    : OscilModal(themeService, title)
{
    setTestId(testId);
}

void OscilModal::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilModal::~OscilModal()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    showAnimator_.reset();
    hoverAnimator_.reset();
}

void OscilModal::setTitle(const juce::String& title)
{
    if (title_ != title)
    {
        title_ = title;
        repaint();
    }
}

void OscilModal::setContent(juce::Component* content)
{
    if (content_.getComponent() != content)
    {
        if (auto* oldContent = content_.getComponent())
            removeChildComponent(oldContent);

        content_ = content;

        if (auto* newContent = content_.getComponent())
        {
            contentPreferredWidth_ = newContent->getWidth();
            contentPreferredHeight_ = newContent->getHeight();
            addAndMakeVisible(newContent);
        }

        resized();
    }
}

void OscilModal::setSize(ModalSize size)
{
    modalSize_ = size;
    customWidth_ = 0;
    customHeight_ = 0;
    resized();
}

void OscilModal::setCustomSize(int width, int height)
{
    customWidth_ = width;
    customHeight_ = height;
    resized();
}

void OscilModal::setShowCloseButton(bool show)
{
    showCloseButton_ = show;
    repaint();
}

void OscilModal::setCloseOnEscape(bool close)
{
    closeOnEscape_ = close;
}

void OscilModal::setCloseOnBackdropClick(bool close)
{
    closeOnBackdropClick_ = close;
}

void OscilModal::show(juce::Component* parent)
{
    // Store previous focus
    previousFocus_ = juce::Component::getCurrentlyFocusedComponent();

    // Register focus change listener for focus trap
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    juce::Desktop::getInstance().addFocusChangeListener(this);

    if (parent)
    {
        parent->addAndMakeVisible(this);
        setBounds(parent->getLocalBounds());
        
        // Try to get animation service from editor
        if (auto* editor = dynamic_cast<OscilPluginEditor*>(parent->getTopLevelComponent()))
            animService_ = &editor->getAnimationService();
    }
    else
    {
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        auto displayBounds = juce::Desktop::getInstance().getDisplays()
            .getPrimaryDisplay()->userArea;
        setBounds(displayBounds);
    }

    setVisible(true);
    grabKeyboardFocus();

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        startShowAnimation();
    }
    else
    {
        // Instant show (reduced motion or no animation service)
        showProgress_ = 1.0f;
        scaleProgress_ = 1.0f;

        if (auto* c = content_.getComponent())
        {
            c->setAlpha(1.0f);
            c->setTransform({});
        }

        repaint();
    }
}

void OscilModal::hide()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        startHideAnimation();
    }
    else
    {
        // Instant hide
        showProgress_ = 0.0f;
        scaleProgress_ = 0.0f;
        setVisible(false);

        if (auto* c = content_.getComponent())
        {
            c->setAlpha(1.0f);
            c->setTransform({});
        }

        if (previousFocus_ && previousFocus_->isShowing())
            previousFocus_->grabKeyboardFocus();

        if (onClose)
            onClose();
    }
}

void OscilModal::startShowAnimation()
{
    // Initialize state
    showProgress_ = 0.0f;
    scaleProgress_ = 0.0f;

    if (auto* c = content_.getComponent())
    {
        c->setAlpha(0.0f);
        c->setTransform(juce::AffineTransform::scale(0.95f));
    }

    // Create and start the show animation
    auto safeThis = juce::Component::SafePointer<OscilModal>(this);
    auto animator = animService_->createValueAnimation(
        AnimationPresets::MODAL_DURATION_MS,
        [safeThis](float progress) {
            if (safeThis)
            {
                safeThis->showProgress_ = progress;
                // Scale goes from 0.95 to 1.0
                safeThis->scaleProgress_ = 0.95f + 0.05f * progress;
                safeThis->updateVisualState();
                safeThis->repaint();
            }
        },
        juce::Easings::createEaseOut(),
        [safeThis]() {
            if (safeThis)
            {
                // Ensure final state is clean
                safeThis->showProgress_ = 1.0f;
                safeThis->scaleProgress_ = 1.0f;
                if (auto* c = safeThis->content_.getComponent())
                {
                    c->setAlpha(1.0f);
                    c->setTransform({});
                }
            }
        });

    showAnimator_.set(std::move(animator));
    showAnimator_.start();
}

void OscilModal::startHideAnimation()
{
    auto safeThis = juce::Component::SafePointer<OscilModal>(this);
    auto animator = animService_->createValueAnimation(
        AnimationPresets::MODAL_DURATION_MS,
        [safeThis](float progress) {
            if (safeThis)
            {
                safeThis->showProgress_ = 1.0f - progress;
                // Scale goes from 1.0 to 0.95
                safeThis->scaleProgress_ = 1.0f - 0.05f * progress;
                safeThis->updateVisualState();
                safeThis->repaint();
            }
        },
        juce::Easings::createEaseOut(),
        [safeThis]() {
            if (safeThis)
            {
                safeThis->setVisible(false);

                if (auto* c = safeThis->content_.getComponent())
                {
                    c->setAlpha(1.0f);
                    c->setTransform({});
                }

                if (safeThis->previousFocus_ && safeThis->previousFocus_->isShowing())
                    safeThis->previousFocus_->grabKeyboardFocus();

                if (safeThis->onClose)
                    safeThis->onClose();
            }
        });

    showAnimator_.set(std::move(animator));
    showAnimator_.start();
}

void OscilModal::updateVisualState()
{
    if (auto* c = content_.getComponent())
    {
        c->setAlpha(showProgress_);

        auto modalBounds = getModalBounds();
        auto pivot = modalBounds.getCentre().toFloat() - c->getPosition().toFloat();
        c->setTransform(juce::AffineTransform::scale(scaleProgress_, scaleProgress_, pivot.x, pivot.y));
    }
}

bool OscilModal::requestClose()
{
    if (onCloseRequested)
    {
        if (!onCloseRequested())
            return false;
    }

    hide();
    return true;
}

void OscilModal::paint(juce::Graphics& g)
{
    float alpha = showProgress_;
    if (alpha < 0.01f)
        return;

    paintBackdrop(g);

    auto modalBounds = getModalBounds();

    // Apply scale transform
    float scale = scaleProgress_;
    auto centreX = modalBounds.getCentreX();
    auto centreY = modalBounds.getCentreY();

    auto scaledWidth = static_cast<int>(static_cast<float>(modalBounds.getWidth()) * scale);
    auto scaledHeight = static_cast<int>(static_cast<float>(modalBounds.getHeight()) * scale);

    auto scaledBounds = juce::Rectangle<int>(
        centreX - scaledWidth / 2,
        centreY - scaledHeight / 2,
        scaledWidth,
        scaledHeight
    );

    paintModal(g, scaledBounds);
}

void OscilModal::paintBackdrop(juce::Graphics& g)
{
    float alpha = showProgress_ * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.fillRect(getLocalBounds());
}

void OscilModal::paintModal(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float alpha = showProgress_;

    // Shadow
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.3f * alpha), 20, {0, 4});
    shadow.drawForRectangle(g, bounds);

    // Background
    g.setColour(getTheme().backgroundPrimary.withAlpha(alpha));
    g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_LG);

    // Border
    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f),
                           ComponentLayout::RADIUS_LG, 1.0f);

    // Title bar
    if (title_.isNotEmpty())
    {
        auto titleBounds = bounds.removeFromTop(TITLE_BAR_HEIGHT);
        paintTitleBar(g, titleBounds);
    }
}

void OscilModal::paintTitleBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float alpha = showProgress_;

    // Title text
    auto textBounds = bounds.reduced(MODAL_PADDING, 0);
    if (showCloseButton_)
        textBounds.removeFromRight(CLOSE_BUTTON_SIZE + 8);

    g.setColour(getTheme().textPrimary.withAlpha(alpha));
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f)).boldened());
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    // Close button
    if (showCloseButton_)
    {
        auto closeBounds = getCloseButtonBounds().toFloat();

        // Animated hover effect
        float hoverAlpha = closeHoverProgress_;
        if (hoverAlpha > 0.01f)
        {
            g.setColour(getTheme().backgroundSecondary.withAlpha(alpha * hoverAlpha));
            g.fillRoundedRectangle(closeBounds, ComponentLayout::RADIUS_SM);
        }

        // Close icon
        auto iconColor = getTheme().textSecondary.interpolatedWith(
            getTheme().textPrimary, hoverAlpha);
        g.setColour(iconColor.withAlpha(alpha));
        float iconSize = 14.0f;
        auto iconPath = ListItemIcons::createCloseIcon(iconSize);

        auto iconBounds = iconPath.getBounds();
        float offsetX = closeBounds.getCentreX() - iconBounds.getCentreX();
        float offsetY = closeBounds.getCentreY() - iconBounds.getCentreY();
        iconPath.applyTransform(juce::AffineTransform::translation(offsetX, offsetY));

        g.fillPath(iconPath);
    }

    // Separator
    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.3f));
    g.fillRect(bounds.getX() + 1, bounds.getBottom() - 1, bounds.getWidth() - 2, 1);
}

juce::Rectangle<int> OscilModal::getModalBounds() const
{
    auto parentBounds = getLocalBounds();

    int width = 0, height = 0;

    if (customWidth_ > 0 && customHeight_ > 0)
    {
        width = customWidth_;
        height = customHeight_;
    }
    else
    {
        switch (modalSize_)
        {
            case ModalSize::Small:
                width = SIZE_SMALL;
                break;
            case ModalSize::Medium:
                width = SIZE_MEDIUM;
                break;
            case ModalSize::Large:
                width = SIZE_LARGE;
                break;
            case ModalSize::FullScreen:
                return parentBounds.reduced(MODAL_MARGIN);
            case ModalSize::Auto:
                width = SIZE_MEDIUM;
                break;
        }

        int titleHeight = title_.isNotEmpty() ? TITLE_BAR_HEIGHT : 0;
        int contentHeight = content_.getComponent() ? contentPreferredHeight_ : 200;
        height = titleHeight + contentHeight + MODAL_PADDING * 2;

        int maxHeight = parentBounds.getHeight() - MODAL_MARGIN * 2;
        height = std::min(height, maxHeight);
    }

    return parentBounds.withSizeKeepingCentre(width, height);
}

juce::Rectangle<int> OscilModal::getTitleBarBounds() const
{
    if (title_.isEmpty())
        return {};

    auto modalBounds = getModalBounds();
    return modalBounds.removeFromTop(TITLE_BAR_HEIGHT);
}

juce::Rectangle<int> OscilModal::getContentBounds() const
{
    auto modalBounds = getModalBounds();

    if (title_.isNotEmpty())
        modalBounds.removeFromTop(TITLE_BAR_HEIGHT);

    return modalBounds.reduced(MODAL_PADDING);
}

juce::Rectangle<int> OscilModal::getCloseButtonBounds() const
{
    auto titleBounds = getTitleBarBounds();
    return juce::Rectangle<int>(
        titleBounds.getRight() - MODAL_PADDING - CLOSE_BUTTON_SIZE,
        titleBounds.getCentreY() - CLOSE_BUTTON_SIZE / 2,
        CLOSE_BUTTON_SIZE,
        CLOSE_BUTTON_SIZE
    );
}

void OscilModal::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
    {
        juce::Logger::writeToLog("OscilModal::resized() - Invalid dimensions: "
            + juce::String(getWidth()) + "x" + juce::String(getHeight()));
        return;
    }

    if (auto* c = content_.getComponent())
        c->setBounds(getContentBounds());
}

void OscilModal::mouseDown(const juce::MouseEvent& e)
{
    auto modalBounds = getModalBounds();

    if (showCloseButton_ && getCloseButtonBounds().contains(e.getPosition()))
    {
        requestClose();
        return;
    }

    if (closeOnBackdropClick_ && !modalBounds.contains(e.getPosition()))
    {
        requestClose();
    }
}

void OscilModal::mouseMove(const juce::MouseEvent& e)
{
    if (!showCloseButton_)
        return;

    bool wasHovering = isHoveringClose_;
    isHoveringClose_ = getCloseButtonBounds().contains(e.getPosition());

    if (isHoveringClose_ != wasHovering && animService_)
    {
        float targetValue = isHoveringClose_ ? 1.0f : 0.0f;
        float startValue = closeHoverProgress_;
        auto safeThis = juce::Component::SafePointer<OscilModal>(this);

        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [safeThis, startValue, targetValue](float progress) {
                if (safeThis)
                {
                    safeThis->closeHoverProgress_ = startValue + (targetValue - startValue) * progress;
                    safeThis->repaint();
                }
            },
            juce::Easings::createEase());

        hoverAnimator_.set(std::move(animator));
        hoverAnimator_.start();
    }
}

void OscilModal::mouseExit(const juce::MouseEvent&)
{
    if (isHoveringClose_ && animService_)
    {
        isHoveringClose_ = false;
        float startValue = closeHoverProgress_;
        auto safeThis = juce::Component::SafePointer<OscilModal>(this);

        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [safeThis, startValue](float progress) {
                if (safeThis)
                {
                    safeThis->closeHoverProgress_ = startValue * (1.0f - progress);
                    safeThis->repaint();
                }
            },
            juce::Easings::createEase());

        hoverAnimator_.set(std::move(animator));
        hoverAnimator_.start();
    }
}

void OscilModal::collectFocusableChildren(juce::Component* parent, juce::Array<juce::Component*>& result)
{
    if (!parent) return;

    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        auto* child = parent->getChildComponent(i);
        if (child->isVisible())
        {
            if (child->getWantsKeyboardFocus())
                result.add(child);
            collectFocusableChildren(child, result);
        }
    }
}

bool OscilModal::keyPressed(const juce::KeyPress& key)
{
    if (closeOnEscape_ && key == juce::KeyPress::escapeKey)
    {
        requestClose();
        return true;
    }

    if (key == juce::KeyPress::tabKey)
    {
        auto* c = content_.getComponent();
        if (!c) return false;

        juce::Array<juce::Component*> focusableChildren;
        collectFocusableChildren(c, focusableChildren);

        if (focusableChildren.isEmpty())
            return false;

        auto* currentFocus = juce::Component::getCurrentlyFocusedComponent();
        int currentIndex = focusableChildren.indexOf(currentFocus);

        int nextIndex;
        if (key.getModifiers().isShiftDown())
            nextIndex = (currentIndex <= 0) ? focusableChildren.size() - 1 : currentIndex - 1;
        else
            nextIndex = (currentIndex >= focusableChildren.size() - 1) ? 0 : currentIndex + 1;

        focusableChildren[nextIndex]->grabKeyboardFocus();
        return true;
    }

    return false;
}

void OscilModal::globalFocusChanged(juce::Component* focusedComponent)
{
    if (isVisible() && focusedComponent != nullptr)
    {
        if (!isParentOf(focusedComponent) && focusedComponent != this)
        {
            auto* comp = focusedComponent;
            while (comp != nullptr)
            {
                if (comp->getProperties().contains("isOscilPopup"))
                    return;
                comp = comp->getParentComponent();
            }

            if (auto* c = content_.getComponent())
            {
                juce::Array<juce::Component*> focusableChildren;
                collectFocusableChildren(c, focusableChildren);
                if (!focusableChildren.isEmpty())
                {
                    focusableChildren.getFirst()->grabKeyboardFocus();
                    return;
                }
            }
            grabKeyboardFocus();
        }
    }
}

void OscilModal::focusGained(FocusChangeType)
{
    if (auto* c = content_.getComponent())
    {
        for (int i = 0; i < c->getNumChildComponents(); ++i)
        {
            auto* child = c->getChildComponent(i);
            if (child->getWantsKeyboardFocus())
            {
                child->grabKeyboardFocus();
                break;
            }
        }
    }
}

void OscilModal::updateFocusTrap()
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();

    if (focusedComp && !isParentOf(focusedComp) && focusedComp != this)
    {
        grabKeyboardFocus();
    }
}

//==============================================================================
// Accessibility Handler for OscilModal
//==============================================================================
class OscilModalAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilModalAccessibilityHandler(OscilModal& modal)
        : juce::AccessibilityHandler(modal, juce::AccessibilityRole::window,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press,
                    [&modal] { if (modal.getCloseOnEscape()) modal.hide(); })
          )
        , modal_(modal)
    {
    }

    juce::String getTitle() const override
    {
        return modal_.getTitle().isNotEmpty() ? modal_.getTitle() : "Dialog";
    }

    juce::String getDescription() const override
    {
        return "Modal dialog";
    }

    juce::String getHelp() const override
    {
        juce::String help = "Use Tab to navigate within this dialog.";
        if (modal_.getCloseOnEscape())
            help += " Press Escape to close.";
        return help;
    }

private:
    OscilModal& modal_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilModalAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> OscilModal::createAccessibilityHandler()
{
    return std::make_unique<OscilModalAccessibilityHandler>(*this);
}

//==============================================================================
// OscilAlertModal Implementation
//==============================================================================

void OscilAlertModal::show(IThemeService& themeService,
                            const juce::String& title,
                            const juce::String& message,
                            [[maybe_unused]] Type type,
                            std::function<void()> onOk)
{
    auto modal = std::make_unique<OscilModal>(themeService, title);
    auto content = std::make_unique<juce::Component>();
    auto label = std::make_unique<juce::Label>();
    auto okButton = std::make_unique<OscilButton>(themeService, "OK");

    content->setSize(400, 100);

    label->setText(message, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::topLeft);
    label->setBounds(0, 0, 400, 60);

    okButton->setVariant(ButtonVariant::Primary);
    okButton->setBounds(400 - 80, 70, 80, 30);

    auto* modalPtr = modal.get();
    auto* okButtonPtr = okButton.get();

    content->addAndMakeVisible(label.get());
    content->addAndMakeVisible(okButton.get());

    modal->setContent(content.get());
    modal->setSize(ModalSize::Small);

    okButtonPtr->onClick = [modalPtr]() {
        modalPtr->hide();
    };

    struct ModalComponents {
        std::unique_ptr<OscilModal> modal;
        std::unique_ptr<juce::Component> content;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<OscilButton> okButton;
    };
    auto components = std::make_shared<ModalComponents>(ModalComponents{
        std::move(modal),
        std::move(content),
        std::move(label),
        std::move(okButton)
    });

    modalPtr->onClose = [components, onOk]() {
        if (onOk)
            onOk();

        juce::MessageManager::callAsync([components]() {
        });
    };

    modalPtr->show();
}

} // namespace oscil
