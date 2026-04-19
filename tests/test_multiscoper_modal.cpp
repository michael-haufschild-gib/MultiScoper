/*
    MultiScoper - Modal Component Tests
    Tests for MultiScoperModal UI component
*/

#include "ui/components/AnimationSettings.h"
#include "ui/components/MultiScoperModal.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperModalTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MultiScoperModalTest, DefaultConstruction)
{
    MultiScoperModal modal(getThemeManager());

    EXPECT_FALSE(modal.isShowing());
    EXPECT_TRUE(modal.getTitle().isEmpty());
}

TEST_F(MultiScoperModalTest, ConstructionWithTitle)
{
    MultiScoperModal modal(getThemeManager(), "Confirm Action");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm Action"));
}

TEST_F(MultiScoperModalTest, ConstructionWithTitleAndTestId)
{
    MultiScoperModal modal(getThemeManager(), "Confirm", "modal-1");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm"));
}

// =============================================================================
// Title Tests
// =============================================================================

TEST_F(MultiScoperModalTest, SetTitle)
{
    MultiScoperModal modal(getThemeManager());
    modal.setTitle("Confirm Action");

    EXPECT_EQ(modal.getTitle(), juce::String("Confirm Action"));
}

TEST_F(MultiScoperModalTest, SetEmptyTitle)
{
    MultiScoperModal modal(getThemeManager(), "Initial");
    modal.setTitle("");

    EXPECT_TRUE(modal.getTitle().isEmpty());
}

// =============================================================================
// Content Tests
// =============================================================================

TEST_F(MultiScoperModalTest, SetContent)
{
    MultiScoperModal modal(getThemeManager());

    juce::Component content;
    modal.setContent(&content);
    EXPECT_EQ(modal.getContent(), &content);
}

TEST_F(MultiScoperModalTest, ClearContent)
{
    MultiScoperModal modal(getThemeManager());

    juce::Component content;
    modal.setContent(&content);
    modal.setContent(nullptr);
    EXPECT_EQ(modal.getContent(), nullptr);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(MultiScoperModalTest, SetSizeSmall)
{
    MultiScoperModal modal(getThemeManager());

    modal.setSize(ModalSize::Small);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Small);
}

TEST_F(MultiScoperModalTest, SetSizeMedium)
{
    MultiScoperModal modal(getThemeManager());

    modal.setSize(ModalSize::Medium);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Medium);
}

TEST_F(MultiScoperModalTest, SetSizeLarge)
{
    MultiScoperModal modal(getThemeManager());

    modal.setSize(ModalSize::Large);
    EXPECT_EQ(modal.getModalSize(), ModalSize::Large);
}

TEST_F(MultiScoperModalTest, SetSizeFullScreen)
{
    MultiScoperModal modal(getThemeManager());

    modal.setSize(ModalSize::FullScreen);
    EXPECT_EQ(modal.getModalSize(), ModalSize::FullScreen);
}

TEST_F(MultiScoperModalTest, SetCustomSize)
{
    MultiScoperModal modal(getThemeManager());

    modal.setCustomSize(400, 300);
    // After setting custom size, modal should still have valid configuration
    EXPECT_TRUE(modal.getShowCloseButton());
    EXPECT_TRUE(modal.getCloseOnEscape());
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(MultiScoperModalTest, SetShowCloseButton)
{
    MultiScoperModal modal(getThemeManager());

    modal.setShowCloseButton(true);
    EXPECT_TRUE(modal.getShowCloseButton());

    modal.setShowCloseButton(false);
    EXPECT_FALSE(modal.getShowCloseButton());
}

TEST_F(MultiScoperModalTest, SetCloseOnEscape)
{
    MultiScoperModal modal(getThemeManager());

    modal.setCloseOnEscape(true);
    EXPECT_TRUE(modal.getCloseOnEscape());

    modal.setCloseOnEscape(false);
    EXPECT_FALSE(modal.getCloseOnEscape());
}

TEST_F(MultiScoperModalTest, SetCloseOnBackdropClick)
{
    MultiScoperModal modal(getThemeManager());

    modal.setCloseOnBackdropClick(true);
    EXPECT_TRUE(modal.getCloseOnBackdropClick());

    modal.setCloseOnBackdropClick(false);
    EXPECT_FALSE(modal.getCloseOnBackdropClick());
}

// =============================================================================
// Show/Hide Tests
// =============================================================================

// =============================================================================
// Show / Hide Lifecycle Tests
//
// These tests exercise the actual visibility state machine using a dummy
// parent component. The earlier version of this file contained placebo tests
// that set a callback, ran nothing, then asserted "not called" — which is
// coverage theater. The tests below verify the real flow: show + hide change
// visibility, onClose fires once per close, requestClose runs through the
// onCloseRequested gate, and hideImmediate is a synchronous shortcut path
// safe to use during teardown.
// =============================================================================

TEST_F(MultiScoperModalTest, ShowOnParentMakesComponentVisible)
{
    juce::Component parent;
    parent.setSize(800, 600);

    MultiScoperModal modal(getThemeManager());
    modal.show(&parent);

    EXPECT_TRUE(modal.isVisible());
    EXPECT_EQ(modal.getParentComponent(), &parent);
    EXPECT_EQ(modal.getBounds(), parent.getLocalBounds());
}

TEST_F(MultiScoperModalTest, HideImmediateFiresOnCloseAndHides)
{
    juce::Component parent;
    parent.setSize(400, 300);

    MultiScoperModal modal(getThemeManager());

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    modal.show(&parent);
    ASSERT_TRUE(modal.isVisible());

    modal.hideImmediate();

    EXPECT_FALSE(modal.isVisible());
    EXPECT_EQ(closeCount, 1) << "hideImmediate must invoke onClose exactly once";
}

TEST_F(MultiScoperModalTest, HideImmediateOnUnshownModalIsNoOp)
{
    MultiScoperModal modal(getThemeManager());

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    // Unshown modal: hideNotified_ starts false, so the first hideImmediate
    // fires onClose exactly once.  Second call is a no-op (hideNotified_ guard).
    modal.hideImmediate();
    int const countAfterFirst = closeCount;
    modal.hideImmediate();

    EXPECT_FALSE(modal.isVisible());
    EXPECT_EQ(countAfterFirst, 1) << "First hideImmediate on unshown modal fires onClose exactly once";
    EXPECT_EQ(closeCount, countAfterFirst) << "Second hideImmediate must be a no-op (no additional onClose)";
}

TEST_F(MultiScoperModalTest, HideOnNotVisibleModalFallsThroughToImmediate)
{
    MultiScoperModal modal(getThemeManager());

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    // hide() on a modal that was never shown should take the synchronous fallback
    // path (see MultiScoperModal::hide — isVisible() false routes to hideImmediate),
    // so onClose fires deterministically without a live message loop.
    modal.hide();

    EXPECT_FALSE(modal.isVisible());
    EXPECT_EQ(closeCount, 1) << "hide() fallback path must fire onClose exactly once";
}

// =============================================================================
// Close Request Gate Tests
// =============================================================================

TEST_F(MultiScoperModalTest, EscapeKeyFiresOnCloseWhenCloseOnEscapeIsTrue)
{
    // Force the synchronous hide path — the animated path relies on a JUCE timer
    // that never fires in unit tests (no message loop pump). Reduced-motion is a
    // real code branch, not a test hack: hide() routes to hideImmediate() under it.
    ScopedReducedMotion reducedMotion;

    juce::Component parent;
    parent.setSize(400, 300);

    MultiScoperModal modal(getThemeManager());
    modal.setCloseOnEscape(true);

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    modal.show(&parent);
    ASSERT_TRUE(modal.isVisible());

    // Direct invocation mirrors what JUCE dispatches for real keypresses.
    bool const handled = modal.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

    EXPECT_TRUE(handled);
    EXPECT_FALSE(modal.isVisible()) << "Escape must dismiss the modal";
    EXPECT_EQ(closeCount, 1) << "onClose must fire exactly once per dismissal";
}

TEST_F(MultiScoperModalTest, EscapeKeyIgnoredWhenCloseOnEscapeIsFalse)
{
    juce::Component parent;
    parent.setSize(400, 300);

    MultiScoperModal modal(getThemeManager());
    modal.setCloseOnEscape(false);

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    modal.show(&parent);
    ASSERT_TRUE(modal.isVisible());

    bool const handled = modal.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

    EXPECT_FALSE(handled) << "keyPressed must decline Escape when closeOnEscape is disabled";
    EXPECT_TRUE(modal.isVisible()) << "Modal must remain visible";
    EXPECT_EQ(closeCount, 0) << "onClose must not fire when Escape is disabled";
}

TEST_F(MultiScoperModalTest, OnCloseRequestedReturningFalseVetosHide)
{
    // Force synchronous hide so a regression that calls hide() (instead of
    // requestClose()) is immediately observable — the animated path would
    // keep isVisible() true and defer onClose until a timer runs.
    ScopedReducedMotion reducedMotion;

    juce::Component parent;
    parent.setSize(400, 300);

    MultiScoperModal modal(getThemeManager());
    modal.setCloseOnEscape(true);

    int vetoCount = 0;
    modal.onCloseRequested = [&vetoCount]() {
        ++vetoCount;
        return false; // veto
    };

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    modal.show(&parent);
    ASSERT_TRUE(modal.isVisible());

    // Escape drives requestClose → onCloseRequested → veto.
    bool const handled = modal.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

    EXPECT_TRUE(handled) << "Escape key is still consumed even when close is vetoed";
    EXPECT_EQ(vetoCount, 1) << "onCloseRequested must be consulted once";
    EXPECT_TRUE(modal.isVisible()) << "Veto must keep the modal open";
    EXPECT_EQ(closeCount, 0) << "onClose must not fire when close is vetoed";
}

TEST_F(MultiScoperModalTest, OnCloseRequestedReturningTrueAllowsHide)
{
    // Force synchronous hide; see EscapeKeyFiresOnCloseWhenCloseOnEscapeIsTrue.
    ScopedReducedMotion reducedMotion;

    juce::Component parent;
    parent.setSize(400, 300);

    MultiScoperModal modal(getThemeManager());
    modal.setCloseOnEscape(true);

    int requestCount = 0;
    modal.onCloseRequested = [&requestCount]() {
        ++requestCount;
        return true; // allow
    };

    int closeCount = 0;
    modal.onClose = [&closeCount]() { ++closeCount; };

    modal.show(&parent);
    modal.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

    EXPECT_EQ(requestCount, 1);
    EXPECT_EQ(closeCount, 1);
    EXPECT_FALSE(modal.isVisible());
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(MultiScoperModalTest, ThemeChangeDoesNotThrow)
{
    MultiScoperModal modal(getThemeManager());
    modal.setTitle("Test Modal");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    // Title should be preserved
    EXPECT_EQ(modal.getTitle(), juce::String("Test Modal"));
}

TEST_F(MultiScoperModalTest, ThemeChangePreservesSize)
{
    MultiScoperModal modal(getThemeManager());
    modal.setSize(ModalSize::Large);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    modal.themeChanged(newTheme);

    EXPECT_EQ(modal.getModalSize(), ModalSize::Large);
}

TEST_F(MultiScoperModalTest, ThemeChangePreservesConfiguration)
{
    MultiScoperModal modal(getThemeManager());
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

TEST_F(MultiScoperModalTest, WantsKeyboardFocus)
{
    MultiScoperModal modal(getThemeManager());

    // Modal should capture focus for keyboard navigation
    EXPECT_TRUE(modal.getWantsKeyboardFocus());
}
