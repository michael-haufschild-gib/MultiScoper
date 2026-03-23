/*
    Oscil - Magnetic Snap Controller Tests
    Tests for the magnetic snapping logic used by sliders and knobs

    Bug targets:
    - Snap threshold calculated incorrectly for zero-width or negative ranges
    - Disabled controller still snapping values
    - consumeSnapFeedback returning true multiple times for single snap
    - Duplicate point insertion
    - Snapping to nearest point when multiple are within threshold
*/

#include "ui/components/MagneticSnapController.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace oscil;

class MagneticSnapControllerTest : public ::testing::Test
{
protected:
    MagneticSnapController controller;
    bool didSnap = false;

    void SetUp() override
    {
        controller.setEnabled(true);
        controller.setMagneticPoints({0.0, 25.0, 50.0, 75.0, 100.0});
    }
};

// =============================================================================
// Core Snapping Behavior
// =============================================================================

TEST_F(MagneticSnapControllerTest, SnapsToNearbyPoint)
{
    // 2% of range [0,100] = 2.0 threshold
    double result = controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 50.0);
}

TEST_F(MagneticSnapControllerTest, DoesNotSnapWhenBeyondThreshold)
{
    // 2% of 100 = 2.0; value is 3.0 away from nearest point (50)
    double result = controller.applySnapping(53.0, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);
    EXPECT_DOUBLE_EQ(result, 53.0);
}

TEST_F(MagneticSnapControllerTest, SnapsToExactPoint)
{
    double result = controller.applySnapping(25.0, 0.0, 100.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 25.0);
}

TEST_F(MagneticSnapControllerTest, SnapsAtBoundaryOfThreshold)
{
    // Threshold = 2.0 for range [0, 100]. Value at exactly threshold distance.
    // 50 + 1.99 = 51.99 should snap to 50
    double result = controller.applySnapping(51.99, 0.0, 100.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 50.0);

    // 50 + 2.01 = 52.01 should NOT snap
    result = controller.applySnapping(52.01, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);
    EXPECT_DOUBLE_EQ(result, 52.01);
}

// =============================================================================
// Disabled State
// =============================================================================

TEST_F(MagneticSnapControllerTest, DisabledControllerDoesNotSnap)
{
    controller.setEnabled(false);

    double result = controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);
    EXPECT_DOUBLE_EQ(result, 50.5);
}

TEST_F(MagneticSnapControllerTest, ReEnablingRestoresSnapping)
{
    controller.setEnabled(false);
    controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);

    controller.setEnabled(true);
    double result = controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 50.0);
}

// =============================================================================
// Feedback Consumption Pattern
// =============================================================================

TEST_F(MagneticSnapControllerTest, ConsumeSnapFeedbackReturnsTrueOnceAfterSnap)
{
    controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    ASSERT_TRUE(didSnap);

    EXPECT_TRUE(controller.consumeSnapFeedback());
    EXPECT_FALSE(controller.consumeSnapFeedback()); // Second call must be false
}

TEST_F(MagneticSnapControllerTest, ConsumeSnapFeedbackFalseWhenNoSnap)
{
    controller.applySnapping(53.0, 0.0, 100.0, didSnap);
    ASSERT_FALSE(didSnap);

    EXPECT_FALSE(controller.consumeSnapFeedback());
}

// =============================================================================
// Point Management
// =============================================================================

TEST_F(MagneticSnapControllerTest, AddDuplicatePointIsIgnored)
{
    size_t before = controller.getMagneticPoints().size();
    controller.addMagneticPoint(50.0); // Already exists
    EXPECT_EQ(controller.getMagneticPoints().size(), before);
}

TEST_F(MagneticSnapControllerTest, ClearPointsRemovesAllSnapping)
{
    controller.clearMagneticPoints();

    double result = controller.applySnapping(50.5, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);
    EXPECT_DOUBLE_EQ(result, 50.5);
}

TEST_F(MagneticSnapControllerTest, EmptyPointsWithEnabledDoesNotSnap)
{
    controller.clearMagneticPoints();
    controller.setEnabled(true);

    double result = controller.applySnapping(0.0, 0.0, 100.0, didSnap);
    EXPECT_FALSE(didSnap);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

// =============================================================================
// Range Edge Cases
// =============================================================================

TEST_F(MagneticSnapControllerTest, ZeroRangeDoesNotCrash)
{
    // Range = 0, threshold = 0% of 0 = 0
    // No snapping should occur (threshold is 0)
    controller.setMagneticPoints({50.0});
    double result = controller.applySnapping(50.0, 50.0, 50.0, didSnap);
    // With zero range, 2% * 0 = 0 threshold, so exact match still snaps (abs(0) < 0 is false)
    // Actually abs(50-50) = 0, and 0 < 0 is false, so no snap
    EXPECT_DOUBLE_EQ(result, 50.0);
}

TEST_F(MagneticSnapControllerTest, NarrowRangeHasTightThreshold)
{
    // Range = 1.0, threshold = 0.02
    controller.setMagneticPoints({0.5});

    double result = controller.applySnapping(0.51, 0.0, 1.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 0.5);

    // 0.03 away, beyond 0.02 threshold
    result = controller.applySnapping(0.53, 0.0, 1.0, didSnap);
    EXPECT_FALSE(didSnap);
}

TEST_F(MagneticSnapControllerTest, LargeRangeHasWideThreshold)
{
    // Range = 10000, threshold = 200
    controller.setMagneticPoints({5000.0});

    double result = controller.applySnapping(5100.0, 0.0, 10000.0, didSnap);
    EXPECT_TRUE(didSnap);
    EXPECT_DOUBLE_EQ(result, 5000.0);
}
