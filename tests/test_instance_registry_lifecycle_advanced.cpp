#include <gtest/gtest.h>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/DecimatingCaptureBuffer.h"

using namespace oscil;

class InstanceRegistryLifecycleAdvancedTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::mutex dispatcherMutex;

    InstanceRegistry& getRegistry() { return *registry_; }

    void SetUp() override
    {
        // Create owned instance
        registry_ = std::make_unique<InstanceRegistry>();

        // Force synchronous dispatch for tests
        getRegistry().setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });
    }

    void TearDown() override
    {
        registry_.reset();
    }
};

// Test Scenario 1: "Freeze" (Duplicate active track)
// The host creates a new instance (offline renderer) while the old one might still exist.
// We expect the new instance to get a DIFFERENT ID to avoid conflict/pollution.
TEST_F(InstanceRegistryLifecycleAdvancedTest, HandlesFreezeDuplicateInstance)
{
    juce::String trackId = "track-uuid-123";

    // 1. Create Instance A (Original Track)
    auto bufferA = std::make_shared<DecimatingCaptureBuffer>();
    SourceId idA = getRegistry().registerInstance(trackId, bufferA, "Track A", 2, 44100.0, nullptr);
    
    EXPECT_TRUE(idA.isValid());
    auto sourceA = getRegistry().getSource(idA);
    ASSERT_TRUE(sourceA.has_value());
    EXPECT_EQ(sourceA->trackIdentifier, trackId);

    // 2. Create Instance B (Freeze/Render Instance)
    // It attempts to register with the SAME trackId
    auto bufferB = std::make_shared<DecimatingCaptureBuffer>();
    SourceId idB = getRegistry().registerInstance(trackId, bufferB, "Track A (Render)", 2, 44100.0, nullptr);

    // 3. Verify B got a NEW ID and modified trackIdentifier
    EXPECT_TRUE(idB.isValid());
    EXPECT_NE(idA, idB);
    
    auto sourceB = getRegistry().getSource(idB);
    ASSERT_TRUE(sourceB.has_value());
    // Verify that the registry generated a NEW trackIdentifier for the duplicate
    EXPECT_NE(sourceB->trackIdentifier, trackId); 
    EXPECT_NE(sourceB->trackIdentifier, sourceA->trackIdentifier);

    // 4. Verify A is untouched
    auto sourceA_After = getRegistry().getSource(idA);
    EXPECT_EQ(sourceA_After->buffer.lock(), bufferA);
}

// Test Scenario 2: "Reload" (Replace track)
// The host destroys the old instance and creates a new one (e.g., changing sample rate, reloading project).
// We expect the new instance to REUSE the ID to maintain connections.
TEST_F(InstanceRegistryLifecycleAdvancedTest, HandlesReloadReplaceInstance)
{
    juce::String trackId = "track-uuid-456";

    // 1. Create Instance A
    auto bufferA = std::make_shared<DecimatingCaptureBuffer>();
    SourceId idA = getRegistry().registerInstance(trackId, bufferA, "Track A", 2, 44100.0, nullptr);
    
    // 2. Destroy Instance A (Buffer A dies)
    // In real app, Processor destructor calls unregister, but sometimes
    // we just want to test the "expired weak ptr" logic of the registry.
    // If we DON'T unregister explicitly, the registry holds a weak ptr.
    bufferA.reset(); 

    // 3. Create Instance B (Reloaded Track)
    auto bufferB = std::make_shared<DecimatingCaptureBuffer>();
    SourceId idB = getRegistry().registerInstance(trackId, bufferB, "Track A", 2, 48000.0, nullptr);

    // 4. Verify B REUSED the entry (or at least mapped to the same SourceID logic if we implemented it that way)
    // Actually, current implementation:
    // "If expired: this is project RELOAD -> Reuse the existing source entry"
    
    EXPECT_EQ(idA, idB); // Should be same SourceID
    
    auto sourceB = getRegistry().getSource(idB);
    ASSERT_TRUE(sourceB.has_value());
    EXPECT_EQ(sourceB->trackIdentifier, trackId); // Should keep original ID
    EXPECT_EQ(sourceB->sampleRate, 48000.0); // Should update metadata
}

// Test Scenario 3: Async Callback Safety
// Verify that if Registry dies, pending callbacks don't crash.
TEST_F(InstanceRegistryLifecycleAdvancedTest, ShutdownPreventsCallbacks)
{
    // Use unique_ptr to control lifetime explicitly
    auto localRegistry = std::make_unique<InstanceRegistry>();
    bool callbackCalled = false;

    // Create a listener
    class MockListener : public InstanceRegistryListener {
    public:
        bool* called;
        MockListener(bool* c) : called(c) {}
        void sourceAdded(const SourceId&) override { *called = true; }
        void sourceRemoved(const SourceId&) override {}
        void sourceUpdated(const SourceId&) override {}
    };

    MockListener listener(&callbackCalled);
    localRegistry->addListener(&listener);

    // We need to hijack the dispatcher to run MANUALLY
    std::function<void()> pendingTask;
    localRegistry->setDispatcher([&](std::function<void()> f) {
        pendingTask = f;
    });

    // Register triggers notification (queues task)
    auto buffer = std::make_shared<DecimatingCaptureBuffer>();
    localRegistry->registerInstance("id", buffer, "Name", 2, 44100.0, nullptr);

    ASSERT_TRUE(pendingTask != nullptr);

    // DESTROY the registry (simulating app exit/unload)
    // In a real scenario, this happens before the message loop processes the task.
    localRegistry.reset(); 

    // Now execute the task (simulating message thread running it later)
    // This SHOULD crash if not protected.
    // However, since we are in a test process, a crash will fail the test.
    // Ideally, we want it NOT to crash.
    
    // NOTE: This test is expected to CRASH or fail AddressSanitizer if the code is buggy.
    // If we fix it with WeakReference, it should run safely.
    pendingTask();

    // Should NOT have called the listener because registry is gone
    EXPECT_FALSE(callbackCalled);
}