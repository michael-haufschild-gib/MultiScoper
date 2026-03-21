/*
    Oscil - Memory Budget Manager Tests (Edge Cases)
    Quality overrides, auto-quality, listeners, global config, pruning
*/

#include <gtest/gtest.h>
#include "core/MemoryBudgetManager.h"
#include "core/DecimatingCaptureBuffer.h"

using namespace oscil;

//==============================================================================
// Test Listener (duplicated for standalone compilation)
//==============================================================================

class MockMemoryBudgetListenerEdge : public MemoryBudgetManager::Listener
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
// Base Test Fixture
//==============================================================================

class MemoryBudgetManagerEdgeTestBase : public ::testing::Test
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
// Quality Override Tests
//==============================================================================

class MemoryBudgetQualityOverrideEdgeTest : public MemoryBudgetManagerEdgeTestBase {};

TEST_F(MemoryBudgetQualityOverrideEdgeTest, DefaultOverrideIsUseGlobal)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(manager().getBufferQualityOverride("buffer1"), QualityOverride::UseGlobal);
}

TEST_F(MemoryBudgetQualityOverrideEdgeTest, SetOverrideChangesValue)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    manager().setBufferQualityOverride("buffer1", QualityOverride::High);

    EXPECT_EQ(manager().getBufferQualityOverride("buffer1"), QualityOverride::High);
}

TEST_F(MemoryBudgetQualityOverrideEdgeTest, OverrideAffectsEffectiveQuality)
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

TEST_F(MemoryBudgetQualityOverrideEdgeTest, UseGlobalReturnsGlobalPreset)
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

TEST_F(MemoryBudgetQualityOverrideEdgeTest, UseGlobalOverrideUsesRecommendedWhenAutoAdjustEnabled)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = true;
    config.memoryBudget.totalBudgetBytes = 1024;  // Force low recommended quality
    manager().setGlobalConfig(config, 44100);

    auto buffer = createBuffer(QualityPreset::High);
    manager().registerBuffer("buffer1", buffer);

    manager().setBufferQualityOverride("buffer1", QualityOverride::UseGlobal);

    const auto recommended = manager().getRecommendedQuality();
    EXPECT_EQ(manager().getEffectiveQuality("buffer1"), recommended);
    EXPECT_EQ(buffer->getConfig().qualityPreset, recommended);
}

//==============================================================================
// Auto-Quality Adjustment Tests
//==============================================================================

class MemoryBudgetAutoQualityEdgeTest : public MemoryBudgetManagerEdgeTestBase {};

TEST_F(MemoryBudgetAutoQualityEdgeTest, RecommendedQualityForFewBuffers)
{
    // With few buffers and default budget, should recommend High
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    QualityPreset recommended = manager().getRecommendedQuality();

    // With only 1 buffer and 50MB budget, High should be affordable
    EXPECT_EQ(recommended, QualityPreset::High);
}

TEST_F(MemoryBudgetAutoQualityEdgeTest, RecommendedQualityDecreasesWithManyBuffers)
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

TEST_F(MemoryBudgetAutoQualityEdgeTest, ApplyRecommendedQualityUpdatesBuffers)
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

class MemoryBudgetListenerEdgeTest : public MemoryBudgetManagerEdgeTestBase
{
protected:
    void SetUp() override
    {
        MemoryBudgetManagerEdgeTestBase::SetUp();
        listener = std::make_unique<MockMemoryBudgetListenerEdge>();
        manager().addListener(listener.get());
    }

    void TearDown() override
    {
        manager().removeListener(listener.get());
        MemoryBudgetManagerEdgeTestBase::TearDown();
    }

    std::unique_ptr<MockMemoryBudgetListenerEdge> listener;
};

TEST_F(MemoryBudgetListenerEdgeTest, BufferCountChangedCalledOnRegister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_EQ(listener->bufferCountChangedCount, 1);
    EXPECT_EQ(listener->lastBufferCount, 1);
}

TEST_F(MemoryBudgetListenerEdgeTest, BufferCountChangedCalledOnUnregister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);
    listener->reset();

    manager().unregisterBuffer("buffer1");

    EXPECT_EQ(listener->bufferCountChangedCount, 1);
    EXPECT_EQ(listener->lastBufferCount, 0);
}

TEST_F(MemoryBudgetListenerEdgeTest, MemoryUsageChangedCalledOnRegister)
{
    auto buffer = createBuffer();
    manager().registerBuffer("buffer1", buffer);

    EXPECT_GE(listener->memoryUsageChangedCount, 1);
    EXPECT_GT(listener->lastSnapshot.totalUsageBytes, 0u);
}

TEST_F(MemoryBudgetListenerEdgeTest, MemoryUsageChangedCalledOnConfigChange)
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

class MemoryBudgetGlobalConfigEdgeTest : public MemoryBudgetManagerEdgeTestBase {};

TEST_F(MemoryBudgetGlobalConfigEdgeTest, SetGlobalConfigUpdatesConfig)
{
    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::Eco;
    newConfig.bufferDuration = BufferDuration::Long;

    manager().setGlobalConfig(newConfig, 96000);

    EXPECT_EQ(manager().getGlobalConfig().qualityPreset, QualityPreset::Eco);
    EXPECT_EQ(manager().getGlobalConfig().bufferDuration, BufferDuration::Long);
    EXPECT_EQ(manager().getSourceRate(), 96000);
}

TEST_F(MemoryBudgetGlobalConfigEdgeTest, SetGlobalConfigReconfiguresBuffers)
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

class MemoryBudgetPruneEdgeTest : public MemoryBudgetManagerEdgeTestBase {};

TEST_F(MemoryBudgetPruneEdgeTest, PruneRemovesExpiredBuffers)
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

TEST_F(MemoryBudgetPruneEdgeTest, ExpiredBuffersArePrunedAutomaticallyByOperations)
{
    {
        CaptureQualityConfig config;
        auto buffer = std::make_shared<DecimatingCaptureBuffer>(config, 44100);
        manager().registerBuffer("buffer1", buffer);
        EXPECT_EQ(manager().getBufferCount(), 1);
    }

    // getBufferCount() should trigger automatic prune of expired weak_ptr entries.
    EXPECT_EQ(manager().getBufferCount(), 0);
    EXPECT_FALSE(manager().isBufferRegistered("buffer1"));
}
