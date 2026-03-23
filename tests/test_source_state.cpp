/*
    Oscil - Source State Tests
    Tests for Source state machine, transitions, validation, and activity tracking
*/

#include "core/Source.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace oscil;

class SourceStateTest : public ::testing::Test
{
protected:
    std::unique_ptr<Source> source;
    SourceId sourceId;
    InstanceId instanceId;

    void SetUp() override
    {
        sourceId = SourceId::generate();
        instanceId = InstanceId::generate();
        source = std::make_unique<Source>(sourceId);
    }

    void TearDown() override { source.reset(); }
};

// === Basic State Machine Tests ===

TEST_F(SourceStateTest, DefaultSourceState)
{
    EXPECT_EQ(source->getState(), SourceState::DISCOVERED);
    EXPECT_FALSE(source->isActive());
}

TEST_F(SourceStateTest, ValidStateTransitions)
{
    // DISCOVERED -> ACTIVE
    EXPECT_TRUE(source->canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
    EXPECT_TRUE(source->isActive());

    // ACTIVE -> INACTIVE
    EXPECT_TRUE(source->canTransitionTo(SourceState::INACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source->getState(), SourceState::INACTIVE);

    // INACTIVE -> ACTIVE
    EXPECT_TRUE(source->canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

TEST_F(SourceStateTest, InvalidStateTransitions)
{
    // DISCOVERED cannot transition to INACTIVE directly
    EXPECT_FALSE(source->canTransitionTo(SourceState::INACTIVE));
    EXPECT_FALSE(source->transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source->getState(), SourceState::DISCOVERED); // State unchanged

    // DISCOVERED cannot transition to STALE directly
    EXPECT_FALSE(source->canTransitionTo(SourceState::STALE));
    EXPECT_FALSE(source->transitionTo(SourceState::STALE));
}

TEST_F(SourceStateTest, ActiveToOrphaned)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));

    EXPECT_TRUE(source->canTransitionTo(SourceState::ORPHANED));
    EXPECT_TRUE(source->transitionTo(SourceState::ORPHANED));
    EXPECT_EQ(source->getState(), SourceState::ORPHANED);
}

TEST_F(SourceStateTest, OrphanedToActive)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ORPHANED));

    // Can recover from ORPHANED to ACTIVE
    EXPECT_TRUE(source->canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

TEST_F(SourceStateTest, ActiveToStaleTransition)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));

    // ACTIVE -> STALE is valid
    EXPECT_TRUE(source->canTransitionTo(SourceState::STALE));
    EXPECT_TRUE(source->transitionTo(SourceState::STALE));
    EXPECT_EQ(source->getState(), SourceState::STALE);
}

TEST_F(SourceStateTest, StaleToOrphanedTransition)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::STALE));

    // STALE -> ORPHANED is valid
    EXPECT_TRUE(source->canTransitionTo(SourceState::ORPHANED));
    EXPECT_TRUE(source->transitionTo(SourceState::ORPHANED));
}

TEST_F(SourceStateTest, StaleToActiveTransition)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::STALE));

    // STALE -> ACTIVE is valid (recovery)
    EXPECT_TRUE(source->canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

// === Edge Case State Transitions ===

TEST_F(SourceStateTest, InactiveToStaleTransitionInvalid)
{
    // Get to INACTIVE state
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source->getState(), SourceState::INACTIVE);

    // INACTIVE -> STALE is NOT a valid transition per state machine
    EXPECT_FALSE(source->canTransitionTo(SourceState::STALE));
    EXPECT_FALSE(source->transitionTo(SourceState::STALE));
    EXPECT_EQ(source->getState(), SourceState::INACTIVE); // State unchanged
}

TEST_F(SourceStateTest, AllInvalidTransitionsFromDiscovered)
{
    EXPECT_EQ(source->getState(), SourceState::DISCOVERED);

    // Can only go to ACTIVE from DISCOVERED
    EXPECT_FALSE(source->canTransitionTo(SourceState::DISCOVERED));
    EXPECT_FALSE(source->canTransitionTo(SourceState::INACTIVE));
    EXPECT_FALSE(source->canTransitionTo(SourceState::ORPHANED));
    EXPECT_FALSE(source->canTransitionTo(SourceState::STALE));
}

TEST_F(SourceStateTest, AllInvalidTransitionsFromOrphaned)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ORPHANED));

    // Can only go to ACTIVE from ORPHANED
    EXPECT_FALSE(source->canTransitionTo(SourceState::DISCOVERED));
    EXPECT_FALSE(source->canTransitionTo(SourceState::INACTIVE));
    EXPECT_FALSE(source->canTransitionTo(SourceState::ORPHANED));
    EXPECT_FALSE(source->canTransitionTo(SourceState::STALE));
}

// === State Conversion Tests ===

TEST_F(SourceStateTest, SourceStateStringConversion)
{
    EXPECT_EQ(sourceStateToString(SourceState::DISCOVERED), "DISCOVERED");
    EXPECT_EQ(sourceStateToString(SourceState::ACTIVE), "ACTIVE");
    EXPECT_EQ(sourceStateToString(SourceState::INACTIVE), "INACTIVE");
    EXPECT_EQ(sourceStateToString(SourceState::ORPHANED), "ORPHANED");
    EXPECT_EQ(sourceStateToString(SourceState::STALE), "STALE");

    EXPECT_EQ(stringToSourceState("DISCOVERED"), SourceState::DISCOVERED);
    EXPECT_EQ(stringToSourceState("ACTIVE"), SourceState::ACTIVE);
    EXPECT_EQ(stringToSourceState("INACTIVE"), SourceState::INACTIVE);
    EXPECT_EQ(stringToSourceState("ORPHANED"), SourceState::ORPHANED);
    EXPECT_EQ(stringToSourceState("STALE"), SourceState::STALE);
    EXPECT_EQ(stringToSourceState("INVALID"), SourceState::DISCOVERED); // Default
}

TEST_F(SourceStateTest, ChannelConfigStringConversion)
{
    EXPECT_EQ(channelConfigToString(ChannelConfig::MONO), "MONO");
    EXPECT_EQ(channelConfigToString(ChannelConfig::STEREO), "STEREO");

    EXPECT_EQ(stringToChannelConfig("MONO"), ChannelConfig::MONO);
    EXPECT_EQ(stringToChannelConfig("STEREO"), ChannelConfig::STEREO);
    EXPECT_EQ(stringToChannelConfig("INVALID"), ChannelConfig::STEREO); // Default
}

// === Activity State Tests ===

TEST_F(SourceStateTest, UpdateHeartbeat)
{
    auto initial = source->getLastHeartbeat();

    // Small delay
    juce::Thread::sleep(10);

    source->updateHeartbeat();

    EXPECT_GT(source->getLastHeartbeat().toMilliseconds(), initial.toMilliseconds());
}

TEST_F(SourceStateTest, GetTimeSinceLastAudio)
{
    // Just created, should be very small
    auto elapsed = source->getTimeSinceLastAudio();
    EXPECT_LT(elapsed, 100); // Less than 100ms since just created

    // Wait a bit
    juce::Thread::sleep(50);

    auto elapsed2 = source->getTimeSinceLastAudio();
    EXPECT_GE(elapsed2, 40); // At least 40ms (accounting for timing variance)
}

TEST_F(SourceStateTest, UpdateLastAudioTimeTransitionsToActive)
{
    EXPECT_EQ(source->getState(), SourceState::DISCOVERED);

    source->updateLastAudioTime();
    source->updateActivityState(); // Process state transition

    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

TEST_F(SourceStateTest, UpdateLastAudioTimeFromInactive)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source->getState(), SourceState::INACTIVE);

    source->updateLastAudioTime();
    source->updateActivityState(); // Process state transition

    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

TEST_F(SourceStateTest, UpdateLastAudioTimeFromStale)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::STALE));
    EXPECT_EQ(source->getState(), SourceState::STALE);

    source->updateLastAudioTime();
    source->updateActivityState(); // Process state transition

    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}

TEST_F(SourceStateTest, UpdateLastAudioTimeDoesNotAffectOrphaned)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source->transitionTo(SourceState::ORPHANED));
    EXPECT_EQ(source->getState(), SourceState::ORPHANED);

    // Orphaned sources don't auto-transition on audio update
    source->updateLastAudioTime();
    source->updateActivityState(); // Process state transition attempt

    // State unchanged - ORPHANED is not in the list of states that auto-transition
    EXPECT_EQ(source->getState(), SourceState::ORPHANED);
}

// === Concurrent State Tests ===

TEST_F(SourceStateTest, ConcurrentUpdateLastAudioTime)
{
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));

    std::atomic<int> callCount{0};
    std::vector<std::thread> threads;

    // Spawn multiple threads calling updateLastAudioTime concurrently
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([this, &callCount]() {
            for (int j = 0; j < 100; ++j)
            {
                source->updateLastAudioTime();
                callCount++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(callCount.load(), 1000);
    // Source should still be in a valid state
    EXPECT_EQ(source->getState(), SourceState::ACTIVE);
}
