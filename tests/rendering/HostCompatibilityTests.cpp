/*
    Oscil - Host Compatibility Tests
    Tests for host detection and capability level determination
*/

#include <gtest/gtest.h>
#include "plugin/HostCompatibility.h"

namespace oscil::tests
{

class HostCompatibilityTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clear any previous override
        HostCompatibility::setOverride(nullptr);
    }

    void TearDown() override
    {
        // Clear override after each test
        HostCompatibility::setOverride(nullptr);
    }
};

// =============================================================================
// Host Detection Tests
// =============================================================================

TEST_F(HostCompatibilityTest, AdobeAuditionReturnsSafeMode)
{
    auto level = HostCompatibility::getCapabilityForHost("Adobe Audition 2023");
    EXPECT_EQ(level, HostCapabilityLevel::SafeMode);
}

TEST_F(HostCompatibilityTest, AdobePremiereReturnsSafeMode)
{
    auto level = HostCompatibility::getCapabilityForHost("Adobe Premiere Pro");
    EXPECT_EQ(level, HostCapabilityLevel::SafeMode);
}

TEST_F(HostCompatibilityTest, AnyAdobeProductReturnsSafeMode)
{
    auto level = HostCompatibility::getCapabilityForHost("Adobe Something");
    EXPECT_EQ(level, HostCapabilityLevel::SafeMode);
}

TEST_F(HostCompatibilityTest, AudacityReturnsSafeMode)
{
    auto level = HostCompatibility::getCapabilityForHost("Audacity 3.4.2");
    EXPECT_EQ(level, HostCapabilityLevel::SafeMode);
}

TEST_F(HostCompatibilityTest, OBSReturnsNoMetal)
{
    auto level = HostCompatibility::getCapabilityForHost("OBS Studio");
    EXPECT_EQ(level, HostCapabilityLevel::NoMetal);
}

TEST_F(HostCompatibilityTest, SoundForgeReturnsNoCompute)
{
    auto level = HostCompatibility::getCapabilityForHost("Sound Forge Pro 16");
    EXPECT_EQ(level, HostCapabilityLevel::NoCompute);

    // Also test uppercase variant
    auto level2 = HostCompatibility::getCapabilityForHost("SOUND FORGE Audio Studio");
    EXPECT_EQ(level2, HostCapabilityLevel::NoCompute);
}

TEST_F(HostCompatibilityTest, GarageBandReturnsNoCompute)
{
    auto level = HostCompatibility::getCapabilityForHost("GarageBand");
    EXPECT_EQ(level, HostCapabilityLevel::NoCompute);
}

TEST_F(HostCompatibilityTest, LogicProReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Logic Pro X");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, AbletonLiveReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Ableton Live 11 Suite");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, ProToolsReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Pro Tools 2023");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, CubaseReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Cubase Pro 13");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, FLStudioReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("FL Studio 21");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, ReaperReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("REAPER");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, BitwigReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Bitwig Studio 5");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, StudioOneReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Studio One Professional");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, UnknownHostReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("SomeUnknownDAW v1.0");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

TEST_F(HostCompatibilityTest, EmptyHostReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

// =============================================================================
// Case Insensitivity Tests
// =============================================================================

TEST_F(HostCompatibilityTest, CaseInsensitiveMatching)
{
    EXPECT_EQ(HostCompatibility::getCapabilityForHost("adobe audition"), 
              HostCapabilityLevel::SafeMode);
    EXPECT_EQ(HostCompatibility::getCapabilityForHost("ADOBE AUDITION"), 
              HostCapabilityLevel::SafeMode);
    EXPECT_EQ(HostCompatibility::getCapabilityForHost("obs studio"), 
              HostCapabilityLevel::NoMetal);
    EXPECT_EQ(HostCompatibility::getCapabilityForHost("garageband"), 
              HostCapabilityLevel::NoCompute);
}

// =============================================================================
// Override Tests
// =============================================================================

TEST_F(HostCompatibilityTest, OverrideReturnsOverriddenLevel)
{
    HostCapabilityLevel override = HostCapabilityLevel::SafeMode;
    HostCompatibility::setOverride(&override);
    
    auto level = HostCompatibility::detectHost(nullptr);
    EXPECT_EQ(level, HostCapabilityLevel::SafeMode);
}

TEST_F(HostCompatibilityTest, ClearingOverrideRestoresDetection)
{
    HostCapabilityLevel override = HostCapabilityLevel::SafeMode;
    HostCompatibility::setOverride(&override);
    HostCompatibility::setOverride(nullptr);
    
    // After clearing, it should use real detection
    // We can't easily test the actual result, but at least verify no crash
    auto level = HostCompatibility::detectHost(nullptr);
    (void)level;  // Just ensure it runs
}

// =============================================================================
// Standalone Tests
// =============================================================================

TEST_F(HostCompatibilityTest, StandaloneHostReturnsFull)
{
    auto level = HostCompatibility::getCapabilityForHost("Standalone");
    EXPECT_EQ(level, HostCapabilityLevel::Full);
}

// =============================================================================
// Restriction Reason Tests
// =============================================================================

TEST_F(HostCompatibilityTest, RestrictionReasonReturnsNonEmptyStrings)
{
    EXPECT_FALSE(HostCompatibility::getRestrictionReason(HostCapabilityLevel::Full).isEmpty());
    EXPECT_FALSE(HostCompatibility::getRestrictionReason(HostCapabilityLevel::NoCompute).isEmpty());
    EXPECT_FALSE(HostCompatibility::getRestrictionReason(HostCapabilityLevel::NoMetal).isEmpty());
    EXPECT_FALSE(HostCompatibility::getRestrictionReason(HostCapabilityLevel::SafeMode).isEmpty());
}

// =============================================================================
// String Conversion Tests
// =============================================================================

TEST_F(HostCompatibilityTest, CapabilityLevelToStringWorks)
{
    EXPECT_STREQ(hostCapabilityLevelToString(HostCapabilityLevel::Full), "Full");
    EXPECT_STREQ(hostCapabilityLevelToString(HostCapabilityLevel::NoCompute), "NoCompute");
    EXPECT_STREQ(hostCapabilityLevelToString(HostCapabilityLevel::NoMetal), "NoMetal");
    EXPECT_STREQ(hostCapabilityLevelToString(HostCapabilityLevel::SafeMode), "SafeMode");
}

} // namespace oscil::tests













