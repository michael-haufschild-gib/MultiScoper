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
};

// This test simulates the race condition between UI modification and DAW autosave.
// It is expected to crash or fail with ThreadSanitizer if OscilState is not thread-safe.
TEST_F(OscilStateConcurrencyTest, ConcurrentReadWrite)
{
    // Thread 1: UI Thread - Adding/Removing/Updating oscillators
    std::thread uiThread([this]() {
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
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Thread 2: Audio Thread (Simulated) - Reading state for serialization
    // This mimics getStateInformation -> updateCachedState -> toXmlString
    std::thread audioThread([this]() {
        while (running)
        {
            // Serialize state (reads ValueTree)
            juce::String xml = state.toXmlString();
            
            // Access properties directly
            int cols = static_cast<int>(state.getColumnLayout());
            (void)cols;
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Run for a short duration
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running = false;

    uiThread.join();
    audioThread.join();
}
