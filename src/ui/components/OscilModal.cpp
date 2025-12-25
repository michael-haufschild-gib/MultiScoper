/*
    Oscil - Modal Component Implementation
*/

#include "ui/components/OscilModal.h"
#include "ui/components/OscilButton.h"
#include "ui/components/ListItemIcons.h"

namespace oscil
{

OscilModal::OscilModal(IThemeService& themeService)
    : ThemedComponent(themeService)
    ,  showSpring_(SpringPresets::snappy())
    , scaleSpring_(SpringPresets::bouncy())
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    setVisible(false);
    showSpring_.position = 0.0f;
    scaleSpring_.position = 0.8f;  // Start scaled down
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

    // Register focus change listener for focus trap (ensure we don't double-register)
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    juce::Desktop::getInstance().addFocusChangeListener(this);

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
        // Initialize state before showing
        if (content_)
        {
            content_->setAlpha(showSpring_.position);
            
            // Apply initial scale transform
            float scale = scaleSpring_.position;
            auto modalBounds = getModalBounds();
            auto pivot = modalBounds.getCentre().toFloat() - content_->getPosition().toFloat();
            content_->setTransform(juce::AffineTransform::scale(scale, scale, pivot.x, pivot.y));
        }

        showSpring_.setTarget(1.0f);
        scaleSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        showSpring_.position = 1.0f;
        scaleSpring_.position = 1.0f;
        
        if (content_)
        {
            content_->setAlpha(1.0f);
            content_->setTransform({});
        }
        
        repaint();
    }
}

void OscilModal::hide()
{
    // Unregister focus change listener
    juce::Desktop::getInstance().removeFocusChangeListener(this);

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
        
        if (content_)
        {
            content_->setAlpha(1.0f);
            content_->setTransform({});
        }

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
    float alpha = showSpring_.position;

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

        // Animated hover effect using spring
        float hoverAlpha = closeHoverSpring_.position;
        if (hoverAlpha > 0.01f)
        {
            g.setColour(getTheme().backgroundSecondary.withAlpha(alpha * hoverAlpha));
            g.fillRoundedRectangle(closeBounds, ComponentLayout::RADIUS_SM);
        }

        // Close icon - interpolate color from secondary to primary on hover
        auto iconColor = getTheme().textSecondary.interpolatedWith(
            getTheme().textPrimary, hoverAlpha);
        g.setColour(iconColor.withAlpha(alpha));
        float iconSize = 14.0f;  // Icon size within the button
        auto iconPath = ListItemIcons::createCloseIcon(iconSize);

        // Center the icon within the close button bounds
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

        // Calculate height based on content
        int titleHeight = title_.isNotEmpty() ? TITLE_BAR_HEIGHT : 0;
        int contentHeight = content_ ? contentPreferredHeight_ : 200;
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

void OscilModal::mouseMove(const juce::MouseEvent& e)
{
    if (!showCloseButton_)
        return;

    bool wasHovering = isHoveringClose_;
    isHoveringClose_ = getCloseButtonBounds().contains(e.getPosition());

    if (isHoveringClose_ != wasHovering)
    {
        closeHoverSpring_.setTarget(isHoveringClose_ ? 1.0f : 0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
}

void OscilModal::mouseExit(const juce::MouseEvent&)
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
    if (!parent) return;

    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        auto* child = parent->getChildComponent(i);
        if (child->isVisible())
        {
            if (child->getWantsKeyboardFocus())
                result.add(child);
            // Recursively collect from nested components
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

    // Tab focus trap - recursively find all focusable children
    if (key == juce::KeyPress::tabKey && content_)
    {
        juce::Array<juce::Component*> focusableChildren;
        collectFocusableChildren(content_, focusableChildren);

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
    // Focus trap: if focus moves outside the modal, bring it back
    if (isVisible() && focusedComponent != nullptr)
    {
        if (!isParentOf(focusedComponent) && focusedComponent != this)
        {
            // Allow focus if it's a known popup (or child of one)
            auto* comp = focusedComponent;
            while (comp != nullptr)
            {
                if (comp->getProperties().contains("isOscilPopup"))
                    return;
                comp = comp->getParentComponent();
            }

            // Focus escaped - bring it back to first focusable child
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
            
            if (content_)
            {
                content_->setAlpha(1.0f);
                content_->setTransform({});
            }

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
    closeHoverSpring_.update(dt);

    if (content_)
    {
        // Sync content alpha
        content_->setAlpha(showSpring_.position);
        
        // Sync content scale
        float scale = scaleSpring_.position;
        // Calculate pivot relative to content position
        // Modal background scales around its center
        auto modalBounds = getModalBounds();
        auto pivot = modalBounds.getCentre().toFloat() - content_->getPosition().toFloat();
        
        content_->setTransform(juce::AffineTransform::scale(scale, scale, pivot.x, pivot.y));
    }
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
// OscilAlertModal Implementation (static helper)
//==============================================================================

void OscilAlertModal::show(IThemeService& themeService,
                            const juce::String& title,
                            const juce::String& message,
                            [[maybe_unused]] Type type,
                            std::function<void()> onOk)
{
    // Use unique_ptr for safe ownership management
    auto modal = std::make_unique<OscilModal>(themeService, title);
    auto content = std::make_unique<juce::Component>();
    auto label = std::make_unique<juce::Label>();
    auto okButton = std::make_unique<OscilButton>(themeService, "OK");

    // Configure content
    content->setSize(400, 100);

    // Configure label
    label->setText(message, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::topLeft);
    label->setBounds(0, 0, 400, 60);

    // Configure button
    okButton->setVariant(ButtonVariant::Primary);
    okButton->setBounds(400 - 80, 70, 80, 30);

    // Get raw pointers for wiring before releasing ownership
    auto* modalPtr = modal.get();
    auto* okButtonPtr = okButton.get();

    // Add children to content (no ownership transfer, just visibility)
    content->addAndMakeVisible(label.get());
    content->addAndMakeVisible(okButton.get());

    // Set content on modal
    modal->setContent(content.get());
    modal->setSize(ModalSize::Small);

    // Wire up OK button to close the modal
    okButtonPtr->onClick = [modalPtr]() {
        modalPtr->hide();
    };

    // Transfer ownership to shared_ptr for safe capture in lambda
    // All components will be deleted when the shared_ptr is destroyed
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

    // Set up cleanup when modal is closed
    modalPtr->onClose = [components, onOk]() {
        // Call user callback if provided
        if (onOk)
            onOk();

        // Schedule deletion on message thread to avoid deleting during callback
        // The shared_ptr capture ensures components live until this runs
        juce::MessageManager::callAsync([components]() {
            // Components will be deleted when this lambda completes
            // and the shared_ptr goes out of scope
        });
    };

    modalPtr->show();
}

} // namespace oscil
