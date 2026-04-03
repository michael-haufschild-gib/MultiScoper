/*
    Oscil - Modal Component Implementation
    (Painting and layout bounds are in OscilModalPainting.cpp)
*/

#include "ui/components/OscilModal.h"

#include "ui/components/OscilButton.h"

namespace oscil
{

OscilModal::OscilModal(IThemeService& themeService)
    : ThemedComponent(themeService)
    , showSpring_(SpringPresets::medium())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    setVisible(false);
    showSpring_.position = 0.0f;
}

OscilModal::OscilModal(IThemeService& themeService, const juce::String& title) : OscilModal(themeService)
{
    title_ = title;
}

OscilModal::OscilModal(IThemeService& themeService, const juce::String& title, const juce::String& testId)
    : OscilModal(themeService, title)
{
    setTestId(testId);
}

void OscilModal::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilModal::~OscilModal()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    stopTimer();
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
    if (content_ != content)
    {
        if (content_)
            removeChildComponent(content_);

        content_ = content;

        if (content_)
        {
            contentPreferredWidth_ = content_->getWidth();
            contentPreferredHeight_ = content_->getHeight();
            addAndMakeVisible(content_);
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

void OscilModal::setCloseOnEscape(bool close) { closeOnEscape_ = close; }

void OscilModal::setCloseOnBackdropClick(bool close) { closeOnBackdropClick_ = close; }

void OscilModal::show(juce::Component* parent)
{
    previousFocus_ = juce::Component::getCurrentlyFocusedComponent();

    juce::Desktop::getInstance().removeFocusChangeListener(this);
    juce::Desktop::getInstance().addFocusChangeListener(this);

    if (parent)
    {
        parent->addAndMakeVisible(this);
        setBounds(parent->getLocalBounds());
    }
    else
    {
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        auto displayBounds = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
        setBounds(displayBounds);
    }

    setVisible(true);
    grabKeyboardFocus();

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        if (content_)
            content_->setAlpha(showSpring_.position);

        showSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 1.0f;

        if (content_)
            content_->setAlpha(1.0f);

        repaint();
    }
}

void OscilModal::hide()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);

    stopTimer();
    showSpring_.position = 0.0f;
    showSpring_.target = 0.0f;
    setVisible(false);

    if (content_)
        content_->setAlpha(1.0f);

    if (previousFocus_ && previousFocus_->isShowing())
        previousFocus_->grabKeyboardFocus();

    if (onClose)
        onClose();
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

void OscilModal::resized()
{
    if (content_)
        content_->setBounds(getContentBounds());
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

    bool const wasHovering = isHoveringClose_;
    isHoveringClose_ = getCloseButtonBounds().contains(e.getPosition());

    if (isHoveringClose_ != wasHovering)
    {
        closeHoverSpring_.setTarget(isHoveringClose_ ? 1.0f : 0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
}

void OscilModal::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (isHoveringClose_)
    {
        isHoveringClose_ = false;
        closeHoverSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
}

void OscilModal::collectFocusableChildren(juce::Component* parent, juce::Array<juce::Component*>& result)
{
    if (!parent)
        return;

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

    if (key == juce::KeyPress::tabKey && content_)
    {
        juce::Array<juce::Component*> focusableChildren;
        collectFocusableChildren(content_, focusableChildren);

        if (focusableChildren.isEmpty())
            return false;

        auto* currentFocus = juce::Component::getCurrentlyFocusedComponent();
        int const currentIndex = focusableChildren.indexOf(currentFocus);

        int nextIndex = 0;
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

            if (content_)
            {
                juce::Array<juce::Component*> focusableChildren;
                collectFocusableChildren(content_, focusableChildren);
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

void OscilModal::focusGained(FocusChangeType /*cause*/)
{
    if (content_)
    {
        for (int i = 0; i < content_->getNumChildComponents(); ++i)
        {
            auto* child = content_->getChildComponent(i);
            if (child->getWantsKeyboardFocus())
            {
                child->grabKeyboardFocus();
                break;
            }
        }
    }
}

void OscilModal::timerCallback()
{
    updateAnimations();

    bool const isSettled = showSpring_.isSettled();

    if (isSettled)
    {
        stopTimer();

        if (showSpring_.position < 0.1f)
        {
            setVisible(false);

            if (content_)
                content_->setAlpha(1.0f);

            if (previousFocus_ && previousFocus_->isShowing())
                previousFocus_->grabKeyboardFocus();

            if (onClose)
                onClose();
        }
    }

    repaint();
    resized();
}

void OscilModal::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    showSpring_.update(dt);
    closeHoverSpring_.update(dt);

    if (content_)
        content_->setAlpha(showSpring_.position);
}

void OscilModal::updateFocusTrap()
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();

    if (focusedComp && !isParentOf(focusedComp) && focusedComp != this)
    {
        grabKeyboardFocus();
    }
}

// Accessibility handler
class OscilModalAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilModalAccessibilityHandler(OscilModal& modal)
        : juce::AccessibilityHandler(modal, juce::AccessibilityRole::window,
                                     juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press,
                                                                            [&modal] {
                                                                                if (modal.getCloseOnEscape())
                                                                                    modal.hide();
                                                                            }))
        , modal_(modal)
    {
    }

    juce::String getTitle() const override { return modal_.getTitle().isNotEmpty() ? modal_.getTitle() : "Dialog"; }

    juce::String getDescription() const override { return "Modal dialog"; }

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

// OscilAlertModal static methods are in OscilModalAlert.cpp

} // namespace oscil
