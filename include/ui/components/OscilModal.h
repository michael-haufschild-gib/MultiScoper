/*
    Oscil - Modal Component
    Animated modal dialogs with focus trap
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

// Note: ModalSize is defined in ComponentTypes.h

/**
 * A modal dialog component with animated show/hide
 *
 * Features:
 * - Animated backdrop and content
 * - Multiple size presets
 * - Optional title bar with close button
 * - Focus trap (Tab cycles within modal)
 * - Escape key dismissal
 * - Click outside to dismiss (optional)
 * - Accessibility support
 */
class OscilModal : public juce::Component,
                   public ThemeManagerListener,
                   public TestIdSupport,
                   private juce::Timer,
                   private juce::FocusChangeListener
{
public:
    OscilModal();
    explicit OscilModal(const juce::String& title);
    OscilModal(const juce::String& title, const juce::String& testId);
    ~OscilModal() override;

    // Configuration
    void setTitle(const juce::String& title);
    juce::String getTitle() const { return title_; }

    void setContent(juce::Component* content);
    juce::Component* getContent() const { return content_; }

    void setSize(ModalSize size);
    ModalSize getModalSize() const { return modalSize_; }

    void setCustomSize(int width, int height);

    void setShowCloseButton(bool show);
    bool getShowCloseButton() const { return showCloseButton_; }

    void setCloseOnEscape(bool close);
    bool getCloseOnEscape() const { return closeOnEscape_; }

    void setCloseOnBackdropClick(bool close);
    bool getCloseOnBackdropClick() const { return closeOnBackdropClick_; }

    // Show/Hide
    void show(juce::Component* parent = nullptr);
    void hide();
    bool isShowing() const { return isVisible() && showSpring_.position > 0.01f; }

    // Callbacks
    std::function<void()> onClose;
    std::function<bool()> onCloseRequested;  // Return false to prevent close

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // FocusChangeListener
    void globalFocusChanged(juce::Component* focusedComponent) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void updateAnimations();

    void paintBackdrop(juce::Graphics& g);
    void paintModal(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintTitleBar(juce::Graphics& g, juce::Rectangle<int> bounds);

    juce::Rectangle<int> getModalBounds() const;
    juce::Rectangle<int> getTitleBarBounds() const;
    juce::Rectangle<int> getContentBounds() const;
    juce::Rectangle<int> getCloseButtonBounds() const;

    bool requestClose();
    void updateFocusTrap();
    void collectFocusableChildren(juce::Component* parent, juce::Array<juce::Component*>& result);

    juce::String title_;
    juce::Component* content_ = nullptr;
    ModalSize modalSize_ = ModalSize::Medium;
    int customWidth_ = 0;
    int customHeight_ = 0;

    bool showCloseButton_ = true;
    bool closeOnEscape_ = true;
    bool closeOnBackdropClick_ = true;

    bool isHoveringClose_ = false;
    juce::Component* previousFocus_ = nullptr;

    SpringAnimation showSpring_;
    SpringAnimation scaleSpring_;

    ColorTheme theme_;

    static constexpr int TITLE_BAR_HEIGHT = 48;
    static constexpr int CLOSE_BUTTON_SIZE = 28;
    static constexpr int MODAL_PADDING = 24;
    static constexpr int MODAL_MARGIN = 40;  // For fullscreen

    static constexpr int SIZE_SMALL = 320;
    static constexpr int SIZE_MEDIUM = 480;
    static constexpr int SIZE_LARGE = 640;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilModal)
};

/**
 * Helper class to show simple alert/confirm dialogs
 */
class OscilAlertModal
{
public:
    enum class Type
    {
        Info,
        Warning,
        Error,
        Confirm
    };

    static void show(const juce::String& title,
                     const juce::String& message,
                     Type type = Type::Info,
                     std::function<void()> onOk = nullptr);

    static void confirm(const juce::String& title,
                        const juce::String& message,
                        std::function<void(bool)> onResult);

private:
    OscilAlertModal() = delete;
};

} // namespace oscil
