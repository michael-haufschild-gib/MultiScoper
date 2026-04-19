/*
    MultiScoper - RippleManager Tests
    Tests for ripple spawn, expiration, and lifecycle management
*/

#include "ui/components/SurfacePainter.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace multiscoper;

class RippleManagerTest : public ::testing::Test
{
protected:
    RippleManager manager;
};

// =============================================================================
// Spawn behavior
// =============================================================================

TEST_F(RippleManagerTest, SpawnCreatesRipple)
{
    EXPECT_TRUE(manager.empty());

    manager.spawn(50.0f, 30.0f, 200.0f, 100.0f);

    EXPECT_FALSE(manager.empty());
    EXPECT_EQ(manager.getRipples().size(), 1u);
}

TEST_F(RippleManagerTest, SpawnSetsMaxRadiusToDiagonal)
{
    float width = 200.0f;
    float height = 100.0f;
    manager.spawn(50.0f, 30.0f, width, height);

    float expectedDiagonal = std::sqrt(width * width + height * height);
    EXPECT_NEAR(manager.getRipples()[0].maxRadius, expectedDiagonal, 0.01f);
}

TEST_F(RippleManagerTest, SpawnSetsOriginCoordinates)
{
    manager.spawn(42.0f, 17.0f, 100.0f, 100.0f);

    const auto& ripple = manager.getRipples()[0];
    EXPECT_NEAR(ripple.originX, 42.0f, 0.001f);
    EXPECT_NEAR(ripple.originY, 17.0f, 0.001f);
}

TEST_F(RippleManagerTest, SpawnSetsBirthTimeToNonZero)
{
    manager.spawn(0.0f, 0.0f, 100.0f, 100.0f);

    // birthTime is set from juce::Time, should be positive
    EXPECT_GT(manager.getRipples()[0].birthTime, 0.0);
}

TEST_F(RippleManagerTest, MultipleSpawnsCreateMultipleRipples)
{
    manager.spawn(10.0f, 10.0f, 100.0f, 100.0f);
    manager.spawn(20.0f, 20.0f, 100.0f, 100.0f);
    manager.spawn(30.0f, 30.0f, 100.0f, 100.0f);

    EXPECT_EQ(manager.getRipples().size(), 3u);
}

// =============================================================================
// hasActiveRipples
// =============================================================================

TEST_F(RippleManagerTest, EmptyManagerHasNoActiveRipples) { EXPECT_FALSE(manager.hasActiveRipples()); }

TEST_F(RippleManagerTest, HasActiveRipplesAfterSpawn)
{
    manager.spawn(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_TRUE(manager.hasActiveRipples());
}

// =============================================================================
// RippleState progress / radius / alpha / expiry
// =============================================================================

TEST_F(RippleManagerTest, RippleProgressIncreasesWithTime)
{
    RippleState ripple;
    ripple.birthTime = 100.0;
    ripple.maxRadius = 200.0f;

    float progress1 = ripple.getProgress(100.1);
    float progress2 = ripple.getProgress(100.3);

    EXPECT_GT(progress2, progress1);
    EXPECT_GE(progress1, 0.0f);
    EXPECT_LE(progress2, 1.0f);
}

TEST_F(RippleManagerTest, RippleRadiusExpandsOverTime)
{
    RippleState ripple;
    ripple.birthTime = 100.0;
    ripple.maxRadius = 200.0f;

    float r1 = ripple.getRadius(100.1);
    float r2 = ripple.getRadius(100.3);
    float rEnd = ripple.getRadius(100.0 + RippleState::DURATION);

    EXPECT_GT(r2, r1);
    EXPECT_NEAR(rEnd, 200.0f, 0.1f);
}

TEST_F(RippleManagerTest, RippleAlphaDecreasesOverTime)
{
    RippleState ripple;
    ripple.birthTime = 100.0;
    ripple.maxRadius = 200.0f;

    float a1 = ripple.getAlpha(100.05);
    float a2 = ripple.getAlpha(100.3);

    EXPECT_GT(a1, a2) << "Alpha should decrease over time";
    EXPECT_GT(a1, 0.0f);
}

TEST_F(RippleManagerTest, RippleExpiresAfterDuration)
{
    RippleState ripple;
    ripple.birthTime = 100.0;
    ripple.maxRadius = 200.0f;

    EXPECT_FALSE(ripple.isExpired(100.3));
    EXPECT_TRUE(ripple.isExpired(100.0 + static_cast<double>(RippleState::DURATION) + 0.001));
}

TEST_F(RippleManagerTest, RippleAlphaIsZeroAtExpiry)
{
    RippleState ripple;
    ripple.birthTime = 100.0;
    ripple.maxRadius = 200.0f;

    float alphaAtEnd = ripple.getAlpha(100.0 + static_cast<double>(RippleState::DURATION));
    EXPECT_NEAR(alphaAtEnd, 0.0f, 0.001f);
}

// =============================================================================
// removeExpired
// =============================================================================

TEST_F(RippleManagerTest, RemoveExpiredClearsOldRipples)
{
    manager.spawn(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_FALSE(manager.empty());

    // Simulate time past ripple duration without sleeping
    double const birthTime = manager.getRipples()[0].birthTime;
    double const expiredTime = birthTime + static_cast<double>(RippleState::DURATION) + 0.1;

    manager.removeExpired(expiredTime);
    EXPECT_TRUE(manager.empty());
}

TEST_F(RippleManagerTest, RemoveExpiredKeepsFreshRipples)
{
    manager.spawn(0.0f, 0.0f, 100.0f, 100.0f);

    // Immediately call removeExpired - ripple is fresh, should survive
    manager.removeExpired();
    EXPECT_FALSE(manager.empty());
    EXPECT_EQ(manager.getRipples().size(), 1u);
}

// =============================================================================
// Edge case: zero-size component
// =============================================================================

TEST_F(RippleManagerTest, SpawnWithZeroSizeComponentDoesNotCrash)
{
    manager.spawn(0.0f, 0.0f, 0.0f, 0.0f);

    EXPECT_EQ(manager.getRipples().size(), 1u);
    // maxRadius will be 0 (diagonal of 0x0 rect)
    EXPECT_NEAR(manager.getRipples()[0].maxRadius, 0.0f, 0.001f);
}
