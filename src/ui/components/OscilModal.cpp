/*
    Oscil - Modal Component Implementation
*/

#include "ui/components/OscilModal.h"
#include "ui/components/OscilButton.h"

namespace oscil
{

OscilModal::OscilModal()
    : showSpring_(SpringPresets::snappy())
    , scaleSpring_(SpringPresets::bouncy())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    setVisible(false);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    showSpring_.position = 0.0f;
    scaleSpring_.position = 0.8f;  // Start scaled down
}

OscilModal::OscilModal(const juce::String& title)
    : OscilModal()
{
    title_ = title;
}

OscilModal::~OscilModal()
{
    ThemeManager::getInstance().removeListener(this);
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
            addAndMakeVisible(content_);

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

    if (parent)
    {
        parent->addAndMakeVisible(this);
        setBounds(parent->getLocalBounds());
    }
    else
    {
        // Add to desktop if no parent
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        auto displayBounds = juce::Desktop::getInstance().getDisplays()
            .getPrimaryDisplay()->userArea;
        setBounds(displayBounds);
    }

    setVisible(true);
    grabKeyboardFocus();

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(1.0f);
        scaleSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 1.0f;
        scaleSpring_.position = 1.0f;
        repaint();
    }
}

void OscilModal::hide()
{
    if (AnimationSettings::shouldUseSpringAnimations())
    {
        showSpring_.setTarget(0.0f);
        scaleSpring_.setTarget(0.8f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 0.0f;
        scaleSpring_.position = 0.8f;
        setVisible(false);

        // Restore focus
        if (previousFocus_ && previousFocus_->isShowing())
            previousFocus_->grabKeyboardFocus();

        if (onClose)
            onClose();
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
    float alpha = showSpring_.position;
    if (alpha < 0.01f)
        return;

    paintBackdrop(g);

    auto modalBounds = getModalBounds();

    // Apply scale transform
    float scale = scaleSpring_.position;
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
    float alpha = showSpring_.position * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.fillRect(getLocalBounds());
}

void OscilModal::paintModal(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float alpha = showSpring_.position;

    // Shadow
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.3f * alpha), 20, {0, 4});
    shadow.drawForRectangle(g, bounds);

    // Background
    g.setColour(theme_.backgroundPrimary.withAlpha(alpha));
    g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_LG);

    // Border
    g.setColour(theme_.controlBorder.withAlpha(alpha * 0.3f));
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
    float alpha = showSpring_.position;

    // Title text
    auto textBounds = bounds.reduced(MODAL_PADDING, 0);
    if (showCloseButton_)
        textBounds.removeFromRight(CLOSE_BUTTON_SIZE + 8);

    g.setColour(theme_.textPrimary.withAlpha(alpha));
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f)).boldened());
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    // Close button
    if (showCloseButton_)
    {
        auto closeBounds = getCloseButtonBounds().toFloat();

        // Hover effect
        if (isHoveringClose_)
        {
            g.setColour(theme_.backgroundSecondary.withAlpha(alpha));
            g.fillRoundedRectangle(closeBounds, ComponentLayout::RADIUS_SM);
        }

        // X icon
        g.setColour(theme_.textSecondary.withAlpha(alpha));
        float cx = closeBounds.getCentreX();
        float cy = closeBounds.getCentreY();
        float size = 6.0f;

        g.drawLine(cx - size, cy - size, cx + size, cy + size, 1.5f);
        g.drawLine(cx + size, cy - size, cx - size, cy + size, 1.5f);
    }

    // Separator
    g.setColour(theme_.controlBorder.withAlpha(alpha * 0.3f));
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

        // Calculate height based on content
        int titleHeight = title_.isNotEmpty() ? TITLE_BAR_HEIGHT : 0;
        int contentHeight = content_ ? content_->getHeight() : 200;
        height = titleHeight + contentHeight + MODAL_PADDING * 2;

        // Limit height
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
    if (content_)
        content_->setBounds(getContentBounds());
}

void OscilModal::mouseDown(const juce::MouseEvent& e)
{
    auto modalBounds = getModalBounds();

    // Check close button
    if (showCloseButton_ && getCloseButtonBounds().contains(e.getPosition()))
    {
        requestClose();
        return;
    }

    // Check backdrop click
    if (closeOnBackdropClick_ && !modalBounds.contains(e.getPosition()))
    {
        requestClose();
    }
}

bool OscilModal::keyPressed(const juce::KeyPress& key)
{
    if (closeOnEscape_ && key == juce::KeyPress::escapeKey)
    {
        requestClose();
        return true;
    }

    // Tab focus trap
    if (key == juce::KeyPress::tabKey && content_)
    {
        // Get all focusable children
        juce::Array<juce::Component*> focusableChildren;
        for (int i = 0; i < content_->getNumChildComponents(); ++i)
        {
            auto* child = content_->getChildComponent(i);
            if (child->getWantsKeyboardFocus())
                focusableChildren.add(child);
        }

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

void OscilModal::focusGained(FocusChangeType)
{
    // Focus first focusable child
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

    bool isSettled = showSpring_.isSettled() && scaleSpring_.isSettled();

    if (isSettled)
    {
        stopTimer();

        if (showSpring_.position < 0.1f)
        {
            setVisible(false);

            // Restore focus
            if (previousFocus_ && previousFocus_->isShowing())
                previousFocus_->grabKeyboardFocus();

            if (onClose)
                onClose();
        }
    }

    repaint();
    resized();  // Update content position during scale animation
}

void OscilModal::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    showSpring_.update(dt);
    scaleSpring_.update(dt);
}

void OscilModal::updateFocusTrap()
{
    // Called when focus changes - ensure it stays within modal
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();

    if (focusedComp && !isParentOf(focusedComp) && focusedComp != this)
    {
        grabKeyboardFocus();
    }
}

void OscilModal::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilModal::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::window
    );
}

//==============================================================================
// OscilAlertModal Implementation (static helper)
//==============================================================================

void OscilAlertModal::show(const juce::String& title,
                            const juce::String& message,
                            [[maybe_unused]] Type type,
                            [[maybe_unused]] std::function<void()> onOk)
{
    // Create content
    auto content = std::make_unique<juce::Component>();
    content->setSize(400, 100);

    auto label = std::make_unique<juce::Label>();
    label->setText(message, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::topLeft);
    label->setBounds(0, 0, 400, 60);
    content->addAndMakeVisible(*label);

    auto okButton = std::make_unique<OscilButton>("OK");
    okButton->setVariant(ButtonVariant::Primary);
    okButton->setBounds(400 - 80, 70, 80, 30);
    content->addAndMakeVisible(*okButton);

    // Create and show modal
    auto modal = std::make_unique<OscilModal>(title);
    modal->setContent(content.get());
    modal->setSize(ModalSize::Small);

    // Note: In real usage, the modal and content ownership would need to be managed
    // This is a simplified example
    modal->show();
}

void OscilAlertModal::confirm([[maybe_unused]] const juce::String& title,
                               [[maybe_unused]] const juce::String& message,
                               [[maybe_unused]] std::function<void(bool)> onResult)
{
    // Similar implementation with Cancel and OK buttons
    // Would pass result to callback
}

} // namespace oscil
