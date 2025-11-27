/*
    Oscil - Modal Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilModal.h"

using namespace oscil;

class OscilModalTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction - closed
TEST_F(OscilModalTest, DefaultConstruction)
{
    OscilModal modal;

    EXPECT_FALSE(modal.isOpen());
}

// Test: Show modal
TEST_F(OscilModalTest, ShowModal)
{
    OscilModal modal;

    modal.show();
    EXPECT_TRUE(modal.isOpen());
}

// Test: Close modal
TEST_F(OscilModalTest, CloseModal)
{
    OscilModal modal;
    modal.show();

    modal.close();
    EXPECT_FALSE(modal.isOpen());
}

// Test: Set title
TEST_F(OscilModalTest, SetTitle)
{
    OscilModal modal;
    modal.setTitle("Confirm Action");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm Action"));
}

// Test: Set content
TEST_F(OscilModalTest, SetContent)
{
    OscilModal modal;

    auto content = std::make_unique<juce::Component>();
    auto* contentPtr = content.get();

    modal.setContent(std::move(content));
    EXPECT_EQ(modal.getContent(), contentPtr);
}

// Test: Size variants
TEST_F(OscilModalTest, SizeVariants)
{
    OscilModal modal;

    modal.setSizeVariant(ModalSize::Small);
    EXPECT_EQ(modal.getSizeVariant(), ModalSize::Small);

    modal.setSizeVariant(ModalSize::Medium);
    EXPECT_EQ(modal.getSizeVariant(), ModalSize::Medium);

    modal.setSizeVariant(ModalSize::Large);
    EXPECT_EQ(modal.getSizeVariant(), ModalSize::Large);

    modal.setSizeVariant(ModalSize::FullScreen);
    EXPECT_EQ(modal.getSizeVariant(), ModalSize::FullScreen);
}

// Test: Close on backdrop click
TEST_F(OscilModalTest, CloseOnBackdropClick)
{
    OscilModal modal;

    modal.setCloseOnBackdropClick(true);
    EXPECT_TRUE(modal.getCloseOnBackdropClick());

    modal.setCloseOnBackdropClick(false);
    EXPECT_FALSE(modal.getCloseOnBackdropClick());
}

// Test: Close on escape key
TEST_F(OscilModalTest, CloseOnEscape)
{
    OscilModal modal;

    modal.setCloseOnEscape(true);
    EXPECT_TRUE(modal.getCloseOnEscape());

    modal.setCloseOnEscape(false);
    EXPECT_FALSE(modal.getCloseOnEscape());
}

// Test: Show close button
TEST_F(OscilModalTest, ShowCloseButton)
{
    OscilModal modal;

    modal.setShowCloseButton(true);
    EXPECT_TRUE(modal.getShowCloseButton());

    modal.setShowCloseButton(false);
    EXPECT_FALSE(modal.getShowCloseButton());
}

// Test: Show header
TEST_F(OscilModalTest, ShowHeader)
{
    OscilModal modal;

    modal.setShowHeader(true);
    EXPECT_TRUE(modal.getShowHeader());

    modal.setShowHeader(false);
    EXPECT_FALSE(modal.getShowHeader());
}

// Test: On close callback
TEST_F(OscilModalTest, OnCloseCallback)
{
    OscilModal modal;

    bool closeCalled = false;
    modal.onClose = [&closeCalled]() {
        closeCalled = true;
    };

    modal.show();
    modal.close();

    EXPECT_TRUE(closeCalled);
}

// Test: On show callback
TEST_F(OscilModalTest, OnShowCallback)
{
    OscilModal modal;

    bool showCalled = false;
    modal.onShow = [&showCalled]() {
        showCalled = true;
    };

    modal.show();

    EXPECT_TRUE(showCalled);
}

// Test: Add footer button
TEST_F(OscilModalTest, AddFooterButton)
{
    OscilModal modal;

    modal.addFooterButton("OK", []() {});
    modal.addFooterButton("Cancel", []() {});

    EXPECT_EQ(modal.getNumFooterButtons(), 2);
}

// Test: Clear footer buttons
TEST_F(OscilModalTest, ClearFooterButtons)
{
    OscilModal modal;
    modal.addFooterButton("OK", []() {});
    modal.addFooterButton("Cancel", []() {});

    modal.clearFooterButtons();
    EXPECT_EQ(modal.getNumFooterButtons(), 0);
}

// Test: Set custom size
TEST_F(OscilModalTest, CustomSize)
{
    OscilModal modal;

    modal.setCustomSize(400, 300);
    EXPECT_EQ(modal.getCustomWidth(), 400);
    EXPECT_EQ(modal.getCustomHeight(), 300);
}

// Test: Animation enabled
TEST_F(OscilModalTest, AnimationEnabled)
{
    OscilModal modal;

    modal.setAnimated(true);
    EXPECT_TRUE(modal.isAnimated());

    modal.setAnimated(false);
    EXPECT_FALSE(modal.isAnimated());
}

// Test: Backdrop opacity
TEST_F(OscilModalTest, BackdropOpacity)
{
    OscilModal modal;

    modal.setBackdropOpacity(0.7f);
    EXPECT_NEAR(modal.getBackdropOpacity(), 0.7f, 0.01f);
}

// Test: Focus trap
TEST_F(OscilModalTest, FocusTrap)
{
    OscilModal modal;

    modal.setFocusTrap(true);
    EXPECT_TRUE(modal.getFocusTrap());

    modal.setFocusTrap(false);
    EXPECT_FALSE(modal.getFocusTrap());
}

// Test: Theme changes
TEST_F(OscilModalTest, ThemeChanges)
{
    OscilModal modal;
    modal.setTitle("Test Modal");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    // Title should be preserved
    EXPECT_EQ(modal.getTitle(), juce::String("Test Modal"));
}

// Test: Accessibility
TEST_F(OscilModalTest, Accessibility)
{
    OscilModal modal;
    modal.setTitle("Test Modal");

    auto* handler = modal.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::dialog);
}

// Test: Center positioning
TEST_F(OscilModalTest, CenterPositioning)
{
    OscilModal modal;

    modal.setCentered(true);
    EXPECT_TRUE(modal.isCentered());

    modal.setCentered(false);
    EXPECT_FALSE(modal.isCentered());
}

// Test: Prevent body scroll
TEST_F(OscilModalTest, PreventBodyScroll)
{
    OscilModal modal;

    modal.setPreventBodyScroll(true);
    EXPECT_TRUE(modal.getPreventBodyScroll());

    modal.setPreventBodyScroll(false);
    EXPECT_FALSE(modal.getPreventBodyScroll());
}
