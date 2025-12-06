/*
    Oscil - Modal Component Tests
    Tests for OscilModal UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilModal.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilModalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilModalTest, DefaultConstruction)
{
    OscilModal modal(getThemeManager());

    EXPECT_FALSE(modal.isShowing());
    EXPECT_TRUE(modal.getTitle().isEmpty());
}

TEST_F(OscilModalTest, ConstructionWithTitle)
{
    OscilModal modal(getThemeManager(), "Confirm Action");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm Action"));
}

TEST_F(OscilModalTest, ConstructionWithTitleAndTestId)
{
    OscilModal modal(getThemeManager(), "Confirm", "modal-1");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm"));
}

// =============================================================================
// Title Tests
// =============================================================================

TEST_F(OscilModalTest, SetTitle)
{
    OscilModal modal(getThemeManager());
    modal.setTitle("Confirm Action");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm Action"));
}

TEST_F(OscilModalTest, SetEmptyTitle)
{
    OscilModal modal(getThemeManager(), "Initial");
    modal.setTitle("");

    EXPECT_TRUE(modal.getTitle().isEmpty());
}

// =============================================================================
// Content Tests
// =============================================================================

TEST_F(OscilModalTest, SetContent)
{
    OscilModal modal(getThemeManager());

    juce::Component content;
    modal.setContent(&content);
    EXPECT_EQ(modal.getContent(), &content);
}

TEST_F(OscilModalTest, ClearContent)
{
    OscilModal modal(getThemeManager());

    juce::Component content;
    modal.setContent(&content);
    modal.setContent(nullptr);
    EXPECT_EQ(modal.getContent(), nullptr);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilModalTest, SetSizeSmall)
{
    OscilModal modal(getThemeManager());

    modal.setSize(ModalSize::Small);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Small);
}

TEST_F(OscilModalTest, SetSizeMedium)
{
    OscilModal modal(getThemeManager());

    modal.setSize(ModalSize::Medium);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Medium);
}

TEST_F(OscilModalTest, SetSizeLarge)
{
    OscilModal modal(getThemeManager());

    modal.setSize(ModalSize::Large);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Large);
}

TEST_F(OscilModalTest, SetSizeFullScreen)
{
    OscilModal modal(getThemeManager());

    modal.setSize(ModalSize::FullScreen);
    EXPECT_EQ(modal.getModalSize(), ModalSize::FullScreen);
}

TEST_F(OscilModalTest, SetCustomSize)
{
    OscilModal modal(getThemeManager());

    modal.setCustomSize(400, 300);
    // Just verify it doesn't crash - custom size is internal
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(OscilModalTest, SetShowCloseButton)
{
    OscilModal modal(getThemeManager());

    modal.setShowCloseButton(true);
    EXPECT_TRUE(modal.getShowCloseButton());

    modal.setShowCloseButton(false);
    EXPECT_FALSE(modal.getShowCloseButton());
}

TEST_F(OscilModalTest, SetCloseOnEscape)
{
    OscilModal modal(getThemeManager());

    modal.setCloseOnEscape(true);
    EXPECT_TRUE(modal.getCloseOnEscape());

    modal.setCloseOnEscape(false);
    EXPECT_FALSE(modal.getCloseOnEscape());
}

TEST_F(OscilModalTest, SetCloseOnBackdropClick)
{
    OscilModal modal(getThemeManager());

    modal.setCloseOnBackdropClick(true);
    EXPECT_TRUE(modal.getCloseOnBackdropClick());

    modal.setCloseOnBackdropClick(false);
    EXPECT_FALSE(modal.getCloseOnBackdropClick());
}

// =============================================================================
// Show/Hide Tests
// =============================================================================

TEST_F(OscilModalTest, ShowModal)
{
#if defined(__SANITIZE_REALTIME__) || defined(__has_feature) && __has_feature(realtime_sanitizer)
    // Skip this test when RTSan is enabled - showing modals in headless
    // environments causes JUCE XWindowSystem assertions that trigger SEGFAULT
    GTEST_SKIP() << "Skipping modal show test under RealtimeSanitizer";
#else
    OscilModal modal(getThemeManager());

    // Note: show() requires a parent, so just test without one
    // The modal animation system means isShowing() depends on spring state
    modal.show(nullptr);
    // Can't reliably test isShowing without a message loop
#endif
}

TEST_F(OscilModalTest, HideModal)
{
    OscilModal modal(getThemeManager());

    modal.hide();
    // Just verify it doesn't crash
}

TEST_F(OscilModalTest, HideWhenNotShown)
{
    OscilModal modal(getThemeManager());

    // Should not crash when hiding a non-visible modal
    modal.hide();
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilModalTest, OnCloseCallback)
{
    OscilModal modal(getThemeManager());

    bool closeCalled = false;
    modal.onClose = [&closeCalled]() {
        closeCalled = true;
    };

    // Callback would be triggered when modal closes
    // Just verify callback can be set
    EXPECT_FALSE(closeCalled);
}

TEST_F(OscilModalTest, OnCloseRequestedCallback)
{
    OscilModal modal(getThemeManager());

    bool requestCalled = false;
    modal.onCloseRequested = [&requestCalled]() {
        requestCalled = true;
        return true;  // Allow close
    };

    // Callback would be triggered when close is requested
    // Just verify callback can be set
    EXPECT_FALSE(requestCalled);
}

TEST_F(OscilModalTest, OnCloseRequestedPreventClose)
{
    OscilModal modal(getThemeManager());

    modal.onCloseRequested = []() {
        return false;  // Prevent close
    };

    // Callback returns false to prevent closing
    // Just verify callback can be set
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilModalTest, ThemeChangeDoesNotThrow)
{
    OscilModal modal(getThemeManager());
    modal.setTitle("Test Modal");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    // Title should be preserved
    EXPECT_EQ(modal.getTitle(), juce::String("Test Modal"));
}

TEST_F(OscilModalTest, ThemeChangePreservesSize)
{
    OscilModal modal(getThemeManager());
    modal.setSize(ModalSize::Large);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    EXPECT_EQ(modal.getModalSize(), ModalSize::Large);
}

TEST_F(OscilModalTest, ThemeChangePreservesConfiguration)
{
    OscilModal modal(getThemeManager());
    modal.setShowCloseButton(false);
    modal.setCloseOnEscape(false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    EXPECT_FALSE(modal.getShowCloseButton());
    EXPECT_FALSE(modal.getCloseOnEscape());
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilModalTest, WantsKeyboardFocus)
{
    OscilModal modal(getThemeManager());

    // Modal should capture focus for keyboard navigation
    EXPECT_TRUE(modal.getWantsKeyboardFocus());
}
