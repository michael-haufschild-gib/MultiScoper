#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include "core/OscilState.h"
#include "core/Oscillator.h"

using namespace oscil;

class OscilStateConcurrencyTest : public ::testing::Test
{
protected:
    OscilState state;
    std::atomic<bool> running{ true };
    std::atomic<int> uiOperations{ 0 };
    std::atomic<int> audioOperations{ 0 };
    std::atomic<bool> exceptionCaught{ false };
};

// This test simulates the race condition between UI modification and DAW autosave.
// It verifies that concurrent operations complete without exceptions and produce valid state.
TEST_F(OscilStateConcurrencyTest, ConcurrentReadWrite)
{
    // Thread 1: UI Thread - Adding/Removing/Updating oscillators
    std::thread uiThread([this]() {
        try {
            while (running)
            {
                // Add an oscillator
                Oscillator osc;
                osc.setSourceId(SourceId::generate());
                state.addOscillator(osc);

                // Update it
                osc.setName("Updated Name");
                state.updateOscillator(osc);

                // Read back (like UI repaint)
                auto oscillators = state.getOscillators();

                // Remove it
                if (!oscillators.empty())
                {
                    state.removeOscillator(oscillators.back().getId());
                }

                uiOperations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        } catch (...) {
            exceptionCaught.store(true, std::memory_order_relaxed);
        }
    });

    // Thread 2: Audio Thread (Simulated) - Reading state for serialization
    // This mimics getStateInformation -> updateCachedState -> toXmlString
    std::thread audioThread([this]() {
        try {
            while (running)
            {
                // Serialize state (reads ValueTree)
                juce::String xml = state.toXmlString();
                // Verify XML is not empty (basic sanity check)
                EXPECT_FALSE(xml.isEmpty());

                // Access properties directly
                int cols = static_cast<int>(state.getColumnLayout());
                EXPECT_GE(cols, 0);
                EXPECT_LE(cols, 4);  // Valid column layout range

                audioOperations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        } catch (...) {
            exceptionCaught.store(true, std::memory_order_relaxed);
        }
    });

    // Run for a short duration
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running = false;

    uiThread.join();
    audioThread.join();

    // Verify operations completed successfully
    EXPECT_FALSE(exceptionCaught.load()) << "Exception caught during concurrent operations";
    EXPECT_GT(uiOperations.load(), 0) << "UI thread should have performed operations";
    EXPECT_GT(audioOperations.load(), 0) << "Audio thread should have performed operations";

    // Verify final state is valid
    auto finalOscillators = state.getOscillators();
    juce::String finalXml = state.toXmlString();
    EXPECT_FALSE(finalXml.isEmpty()) << "Final state should serialize to valid XML";
}
