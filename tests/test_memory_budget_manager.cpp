/*
    Oscil - Memory Budget Manager Unit Tests
    Tests for buffer tracking, memory calculations, and auto-quality adjustment
*/

#include <gtest/gtest.h>
#include "core/MemoryBudgetManager.h"
#include "core/DecimatingCaptureBuffer.h"

using namespace oscil;

//==============================================================================
// Test Listener
//==============================================================================

class MockMemoryBudgetListener : public MemoryBudgetManager::Listener
{
public:
    void memoryUsageChanged(const MemoryUsageSnapshot& snapshot) override
    {
        lastSnapshot = snapshot;
        memoryUsageChangedCount++;
    }

    void bufferCountChanged(int newCount) override
    {
        lastBufferCount = newCount;
        bufferCountChangedCount++;
    }

    void effectiveQualityChanged(QualityPreset newQuality) override
    {
        lastEffectiveQuality = newQuality;
        effectiveQualityChangedCount++;
    }

    void reset()
    {
        memoryUsageChangedCount = 0;
        bufferCountChangedCount = 0;
        effectiveQualityChangedCount = 0;
    }

    MemoryUsageSnapshot lastSnapshot;
    int lastBufferCount = 0;
    QualityPreset lastEffectiveQuality = QualityPreset::Standard;

    int memoryUsageChangedCount = 0;
    int bufferCountChangedCount = 0;
    int effectiveQualityChangedCount = 0;
};

//==============================================================================
// Base Test Fixture - creates its own MemoryBudgetManager instance
//==============================================================================

class MemoryBudgetManagerTestBase : public ::testing::Test
{
protected:
    std::unique_ptr<MemoryBudgetManager> manager_;

    void SetUp() override
    {
        manager_ = std::make_unique<MemoryBudgetManager>();
    }

    void TearDown() override
    {
        manager_.reset();
    }

    MemoryBudgetManager& manager() { return *manager_; }

    std::shared_ptr<DecimatingCaptureBuffer> createBuffer(
        QualityPreset preset = QualityPreset::Standard)
    {
        CaptureQualityConfig config;
        config.qualityPreset = preset;
        config.bufferDuration = BufferDuration::Medium;
        return std::make_shared<DecimatingCaptureBuffer>(config, 44100);
    }
};

//==============================================================================
// Construction Tests
//==============================================================================

class MemoryBudgetManagerConstructionTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetManagerConstructionTest, DefaultConfigIsStandard)
{
    EXPECT_EQ(manager().getGlobalConfig().qualityPreset, QualityPreset::Standard);
}

TEST_F(MemoryBudgetManagerConstructionTest, DefaultSourceRateIs44100)
{
    EXPECT_EQ(manager().getSourceRate(), 44100);
}

TEST_F(MemoryBudgetManagerConstructionTest, InitialBufferCountIsZero)
{
    EXPECT_EQ(manager().getBufferCount(), 0);
}

TEST_F(MemoryBudgetManagerConstructionTest, InitialMemoryUsageIsZero)
{
    EXPECT_EQ(manager().getTotalMemoryUsage(), 0u);
}

//==============================================================================
// Buffer Registration Tests
//==============================================================================

class MemoryBudgetBufferRegistrationTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetBufferRegistrationTest, RegisterBufferIncreasesCount)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(manager().getBufferCount(), 1);
}

TEST_F(MemoryBudgetBufferRegistrationTest, RegisterMultipleBuffers)
{
    auto buffer1 = createBuffer();
    auto buffer2 = createBuffer();
    auto buffer3 = createBuffer();

    manager().registerBuffer("buffer1", buffer1);
    manager().registerBuffer("buffer2", buffer2);
    manager().registerBuffer("buffer3", buffer3);

    EXPECT_EQ(manager().getBufferCount(), 3);
}

TEST_F(MemoryBudgetBufferRegistrationTest, UnregisterBufferDecreasesCount)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);
    EXPECT_EQ(manager().getBufferCount(), 1);

    manager().unregisterBuffer("buffer1");
    EXPECT_EQ(manager().getBufferCount(), 0);
}

TEST_F(MemoryBudgetBufferRegistrationTest, UnregisterNonExistentBufferIsNoOp)
{
    manager().unregisterBuffer("nonexistent");
    EXPECT_EQ(manager().getBufferCount(), 0);
}

TEST_F(MemoryBudgetBufferRegistrationTest, IsBufferRegisteredReturnsTrueForRegistered)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_TRUE(manager().isBufferRegistered("buffer1"));
    EXPECT_FALSE(manager().isBufferRegistered("buffer2"));
}

TEST_F(MemoryBudgetBufferRegistrationTest, RegisterEmptyIdIsIgnored)
{
    auto buffer = createBuffer();
    manager().registerBuffer("", buffer);

    EXPECT_EQ(manager().getBufferCount(), 0);
}

TEST_F(MemoryBudgetBufferRegistrationTest, RegisterNullBufferIsIgnored)
{
    manager().registerBuffer("buffer1", nullptr);

    EXPECT_EQ(manager().getBufferCount(), 0);
}

//==============================================================================
// Memory Tracking Tests
//==============================================================================

class MemoryBudgetTrackingTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetTrackingTest, TotalMemoryUsageIncludesAllBuffers)
{
    auto buffer1 = createBuffer(QualityPreset::Standard);
    auto buffer2 = createBuffer(QualityPreset::Standard);

    manager().registerBuffer("buffer1", buffer1);
    manager().registerBuffer("buffer2", buffer2);

    size_t totalUsage = manager().getTotalMemoryUsage();
    size_t expectedUsage = buffer1->getMemoryUsageBytes() + buffer2->getMemoryUsageBytes();

    EXPECT_EQ(totalUsage, expectedUsage);
}

TEST_F(MemoryBudgetTrackingTest, GetBufferMemoryUsageReturnsCorrectValue)
{
    auto buffer = createBuffer(QualityPreset::Standard);
    size_t expectedUsage = buffer->getMemoryUsageBytes();

    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(manager().getBufferMemoryUsage("buffer1"), expectedUsage);
}

TEST_F(MemoryBudgetTrackingTest, GetBufferMemoryUsageReturnsZeroForUnknownId)
{
    EXPECT_EQ(manager().getBufferMemoryUsage("unknown"), 0u);
}

TEST_F(MemoryBudgetTrackingTest, MemoryUsageStringFormatsCorrectly)
{
    auto buffer = createBuffer(QualityPreset::Standard);
    manager().registerBuffer("buffer1", buffer);

    juce::String usageStr = manager().getTotalMemoryUsageString();

    // Should contain KB or MB
    EXPECT_TRUE(usageStr.contains("KB") || usageStr.contains("MB"));
}

TEST_F(MemoryBudgetTrackingTest, UsagePercentCalculatedCorrectly)
{
    auto buffer = createBuffer(QualityPreset::Standard);
    manager().registerBuffer("buffer1", buffer);

    float percent = manager().getUsagePercent();

    EXPECT_GT(percent, 0.0f);
    EXPECT_LT(percent, 100.0f);  // Single buffer shouldn't exceed budget
}

//==============================================================================
// Memory Snapshot Tests
//==============================================================================

class MemoryBudgetSnapshotTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetSnapshotTest, SnapshotContainsCorrectBufferCount)
{
    auto buffer1 = createBuffer();
    auto buffer2 = createBuffer();

    manager().registerBuffer("buffer1", buffer1);
    manager().registerBuffer("buffer2", buffer2);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_EQ(snapshot.numBuffers, 2);
}

TEST_F(MemoryBudgetSnapshotTest, SnapshotContainsPerBufferUsage)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_EQ(snapshot.perBufferUsage.size(), 1u);
    EXPECT_EQ(snapshot.perBufferUsage[0].id, "buffer1");
    EXPECT_GT(snapshot.perBufferUsage[0].memoryBytes, 0u);
}

TEST_F(MemoryBudgetSnapshotTest, SnapshotCalculatesUsagePercent)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_GT(snapshot.usagePercent, 0.0f);
}

TEST_F(MemoryBudgetSnapshotTest, SnapshotDetectsOverBudget)
{
    // Create many buffers to potentially exceed budget
    CaptureQualityConfig smallBudgetConfig;
    smallBudgetConfig.memoryBudget.totalBudgetBytes = 1024;  // Very small budget (1KB)
    manager().setGlobalConfig(smallBudgetConfig, 44100);

    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_TRUE(snapshot.isOverBudget);
}

TEST_F(MemoryBudgetSnapshotTest, SnapshotEffectiveQualityUsesGlobalWhenAutoAdjustDisabled)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::Eco;
    config.autoAdjustQuality = false;
    manager().setGlobalConfig(config, 44100);

    auto buffer = createBuffer(QualityPreset::High);
    manager().registerBuffer("buffer1", buffer);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_EQ(snapshot.effectiveQuality, QualityPreset::Eco);
}

TEST_F(MemoryBudgetSnapshotTest, SnapshotEffectiveQualityUsesRecommendedWhenAutoAdjustEnabled)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = true;
    config.memoryBudget.totalBudgetBytes = 1024; // Force downshift from High.
    manager().setGlobalConfig(config, 44100);

    auto buffer = createBuffer(QualityPreset::High);
    manager().registerBuffer("buffer1", buffer);

    auto snapshot = manager().getMemorySnapshot();

    EXPECT_EQ(snapshot.effectiveQuality, manager().getRecommendedQuality());
}

