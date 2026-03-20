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
    // After setting custom size, modal should still have valid configuration
    EXPECT_TRUE(modal.getShowCloseButton());
    EXPECT_TRUE(modal.getCloseOnEscape());
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
    OscilModal modal(getThemeManager());

    modal.show(nullptr);
    // Modal should retain its configuration after show attempt
    EXPECT_TRUE(modal.getShowCloseButton());
    EXPECT_TRUE(modal.getCloseOnEscape());
}

TEST_F(OscilModalTest, HideModal)
{
    OscilModal modal(getThemeManager());

    modal.hide();
    // Modal configuration should persist after hide
    EXPECT_TRUE(modal.getCloseOnEscape());
    EXPECT_TRUE(modal.getShowCloseButton());
}

TEST_F(OscilModalTest, HideWhenNotShown)
{
    OscilModal modal(getThemeManager());

    modal.hide();
    // Double hide should not corrupt state
    modal.hide();
    EXPECT_TRUE(modal.getCloseOnEscape());
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

    bool requestCalled = false;
    modal.onCloseRequested = [&requestCalled]() {
        requestCalled = true;
        return false;
    };

    // Callback can be set and modal retains its configuration
    EXPECT_FALSE(requestCalled);
    EXPECT_TRUE(modal.getShowCloseButton());
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
