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
// Base Test Fixture - resets singleton between tests
//==============================================================================

class MemoryBudgetManagerTestBase : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MemoryBudgetManager::resetInstance();
    }

    void TearDown() override
    {
        MemoryBudgetManager::resetInstance();
    }

    MemoryBudgetManager& manager() { return MemoryBudgetManager::getInstance(); }

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

//==============================================================================
// Quality Override Tests
//==============================================================================

class MemoryBudgetQualityOverrideTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetQualityOverrideTest, DefaultOverrideIsUseGlobal)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(manager().getBufferQualityOverride("buffer1"), QualityOverride::UseGlobal);
}

TEST_F(MemoryBudgetQualityOverrideTest, SetOverrideChangesValue)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    manager().setBufferQualityOverride("buffer1", QualityOverride::High);

    EXPECT_EQ(manager().getBufferQualityOverride("buffer1"), QualityOverride::High);
}

TEST_F(MemoryBudgetQualityOverrideTest, OverrideAffectsEffectiveQuality)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::Eco;
    manager().setGlobalConfig(config, 44100);

    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);
    manager().setBufferQualityOverride("buffer1", QualityOverride::High);

    QualityPreset effective = manager().getEffectiveQuality("buffer1");

    EXPECT_EQ(effective, QualityPreset::High);
}

TEST_F(MemoryBudgetQualityOverrideTest, UseGlobalReturnsGlobalPreset)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::Eco;
    config.autoAdjustQuality = false;
    manager().setGlobalConfig(config, 44100);

    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    QualityPreset effective = manager().getEffectiveQuality("buffer1");

    EXPECT_EQ(effective, QualityPreset::Eco);
}

//==============================================================================
// Auto-Quality Adjustment Tests
//==============================================================================

class MemoryBudgetAutoQualityTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetAutoQualityTest, RecommendedQualityForFewBuffers)
{
    // With few buffers and default budget, should recommend High
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    QualityPreset recommended = manager().getRecommendedQuality();

    // With only 1 buffer and 50MB budget, High should be affordable
    EXPECT_EQ(recommended, QualityPreset::High);
}

TEST_F(MemoryBudgetAutoQualityTest, RecommendedQualityDecreasesWithManyBuffers)
{
    // Register many buffers to stress the budget
    std::vector<std::shared_ptr<DecimatingCaptureBuffer>> buffers;

    for (int i = 0; i < 100; ++i)
    {
        auto buffer = createBuffer();
        buffers.push_back(buffer);
        manager().registerBuffer("buffer" + juce::String(i), buffer);
    }

    QualityPreset recommended = manager().getRecommendedQuality();

    // With 100 buffers, should recommend lower quality
    EXPECT_NE(recommended, QualityPreset::High);
}

TEST_F(MemoryBudgetAutoQualityTest, ApplyRecommendedQualityUpdatesBuffers)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    manager().applyRecommendedQuality();

    // Buffer should have been reconfigured
    QualityPreset effective = manager().getEffectiveQuality("buffer1");
    EXPECT_EQ(effective, manager().getRecommendedQuality());
}

//==============================================================================
// Listener Tests
//==============================================================================

class MemoryBudgetListenerTest : public MemoryBudgetManagerTestBase
{
protected:
    void SetUp() override
    {
        MemoryBudgetManagerTestBase::SetUp();
        listener = std::make_unique<MockMemoryBudgetListener>();
        manager().addListener(listener.get());
    }

    void TearDown() override
    {
        manager().removeListener(listener.get());
        MemoryBudgetManagerTestBase::TearDown();
    }

    std::unique_ptr<MockMemoryBudgetListener> listener;
};

TEST_F(MemoryBudgetListenerTest, BufferCountChangedCalledOnRegister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(listener->bufferCountChangedCount, 1);
    EXPECT_EQ(listener->lastBufferCount, 1);
}

TEST_F(MemoryBudgetListenerTest, BufferCountChangedCalledOnUnregister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);
    listener->reset();

    manager().unregisterBuffer("buffer1");

    EXPECT_EQ(listener->bufferCountChangedCount, 1);
    EXPECT_EQ(listener->lastBufferCount, 0);
}

TEST_F(MemoryBudgetListenerTest, MemoryUsageChangedCalledOnRegister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_GE(listener->memoryUsageChangedCount, 1);
    EXPECT_GT(listener->lastSnapshot.totalUsageBytes, 0u);
}

TEST_F(MemoryBudgetListenerTest, MemoryUsageChangedCalledOnConfigChange)
{
    listener->reset();

    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::High;
    manager().setGlobalConfig(newConfig, 48000);

    EXPECT_GE(listener->memoryUsageChangedCount, 1);
}

//==============================================================================
// Global Config Tests
//==============================================================================

class MemoryBudgetGlobalConfigTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetGlobalConfigTest, SetGlobalConfigUpdatesConfig)
{
    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::Eco;
    newConfig.bufferDuration = BufferDuration::Long;

    manager().setGlobalConfig(newConfig, 96000);

    EXPECT_EQ(manager().getGlobalConfig().qualityPreset, QualityPreset::Eco);
    EXPECT_EQ(manager().getGlobalConfig().bufferDuration, BufferDuration::Long);
    EXPECT_EQ(manager().getSourceRate(), 96000);
}

TEST_F(MemoryBudgetGlobalConfigTest, SetGlobalConfigReconfiguresBuffers)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::Eco;
    newConfig.autoAdjustQuality = false;
    manager().setGlobalConfig(newConfig, 44100);

    // Buffer should have been reconfigured
    EXPECT_EQ(buffer->getConfig().qualityPreset, QualityPreset::Eco);
}

//==============================================================================
// Prune Expired Buffers Tests
//==============================================================================

class MemoryBudgetPruneTest : public MemoryBudgetManagerTestBase {};

TEST_F(MemoryBudgetPruneTest, PruneRemovesExpiredBuffers)
{
    {
        CaptureQualityConfig config;
        auto buffer = std::make_shared<DecimatingCaptureBuffer>(config, 44100);
        manager().registerBuffer("buffer1", buffer);
        EXPECT_EQ(manager().getBufferCount(), 1);
        // buffer goes out of scope here
    }

    manager().pruneExpiredBuffers();

    // Buffer was expired, should be removed
    EXPECT_EQ(manager().getBufferCount(), 0);
}
