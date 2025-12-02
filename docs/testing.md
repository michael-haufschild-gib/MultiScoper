# Testing Guide for LLM Coding Agents

**Purpose**: Instructions for writing and running tests in the Oscil codebase.
**Test Stack**: GoogleTest (gtest) v1.17.0

## Quick Reference

```bash
# Build with tests (dev preset includes tests)
cmake --preset dev
cmake --build --preset dev

# Run all tests via CTest
ctest --preset dev

# Run tests directly
./build/dev/OscilTests

# Run specific test
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.*"

# Run with XML output for CI
./build/dev/OscilTests --gtest_output=xml:test-results/results.xml
```

## Where to Put Tests

**Location**: `tests/test_[class_name_snake_case].cpp`

**Examples**:
- Testing `SignalProcessor` → `tests/test_signal_processor.cpp`
- Testing `InstanceRegistry` → `tests/test_instance_registry.cpp`
- Testing `ThemeManager` → `tests/test_theme_manager.cpp`

## How to Write a Test

**Step 1**: Create test file at `tests/test_my_class.cpp`

```cpp
/*
    Oscil - My Class Tests
*/

#include <gtest/gtest.h>
#include "core/MyClass.h"  // or dsp/ or ui/

using namespace oscil;

class MyClassTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test resources before each test
    }

    void TearDown() override
    {
        // Clean up after each test
    }

    // Test helpers
    MyClass myClass_;
};

TEST_F(MyClassTest, MethodDoesExpectedThing)
{
    // Arrange
    int input = 5;

    // Act
    int result = myClass_.someMethod(input);

    // Assert
    EXPECT_EQ(result, 10);
}
```

**Step 2**: Add to `CMakeLists.txt` under `add_executable(OscilTests ...)`

```cmake
tests/test_my_class.cpp
```

**Step 3**: If testing a new source file, also add the source to `OscilTests`:

```cmake
src/core/MyClass.cpp
```

## Test Template: DSP Processing

```cpp
class MyProcessorTest : public ::testing::Test
{
protected:
    MyProcessor processor;

    std::vector<float> generateSineWave(int numSamples, float frequency, float sampleRate)
    {
        std::vector<float> samples(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = std::sin(2.0f * M_PI * frequency * i / sampleRate);
        }
        return samples;
    }

    std::vector<float> generateDC(int numSamples, float level)
    {
        return std::vector<float>(numSamples, level);
    }
};

TEST_F(MyProcessorTest, ProcessesSignalCorrectly)
{
    const int numSamples = 1024;
    auto input = generateSineWave(numSamples, 440.0f, 44100.0f);
    std::vector<float> output(numSamples);

    processor.process(input.data(), output.data(), numSamples);

    // Use EXPECT_NEAR for floating point with tolerance
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output[i], expectedValue, 0.001f);
    }
}
```

## Test Template: State/Serialization

```cpp
class MyStateTest : public ::testing::Test
{
protected:
    OscilState state;
};

TEST_F(MyStateTest, RoundTripSerialization)
{
    // Create and populate state
    Oscillator osc;
    osc.setName("Test");
    osc.setOpacity(0.75f);
    state.addOscillator(osc);

    // Serialize
    juce::String xml = state.toXmlString();
    EXPECT_FALSE(xml.isEmpty());

    // Deserialize
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    // Verify
    auto oscillators = restored.getOscillators();
    ASSERT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Test");
    EXPECT_FLOAT_EQ(oscillators[0].getOpacity(), 0.75f);
}
```

## Test Template: Thread Safety

```cpp
TEST_F(CaptureBufferTest, ThreadSafetyConcurrentAccess)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

    std::thread writer([&]() {
        while (running)
        {
            buffer->write(testData, metadata);
            ++writeCount;
        }
    });

    std::thread reader([&]() {
        while (running)
        {
            buffer->read(output.data(), 64, 0);
            ++readCount;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    reader.join();

    // Verify no crashes, data integrity
    EXPECT_GT(writeCount.load(), 0);
    EXPECT_GT(readCount.load(), 0);
}
```

## Assertion Reference

| Assertion | Use When |
|-----------|----------|
| `EXPECT_EQ(a, b)` | Exact equality (integers, pointers) |
| `EXPECT_FLOAT_EQ(a, b)` | Float equality with ULP tolerance |
| `EXPECT_NEAR(a, b, tol)` | Float within specific tolerance |
| `EXPECT_TRUE(expr)` | Boolean true |
| `EXPECT_FALSE(expr)` | Boolean false |
| `ASSERT_*` variants | Stop test on failure (use for preconditions) |

## Decision Tree: What to Test

- Testing signal math? → Use `EXPECT_NEAR` with tolerance (typically 0.001f)
- Testing exact values? → Use `EXPECT_EQ` or `EXPECT_FLOAT_EQ`
- Testing preconditions? → Use `ASSERT_*` variants
- Testing thread safety? → Use atomics + short sleep + verify no crashes

## Test Data Patterns

| Signal | Generator | Use Case |
|--------|-----------|----------|
| Sine wave | `generateSineWave(samples, freq, sr)` | General waveform tests |
| DC level | `generateDC(samples, level)` | Level/gain tests |
| Zeros | `std::vector<float>(n, 0.0f)` | Silence handling |
| Identical L/R | Same vector for both | Mono/correlation tests |
| Inverted L/R | Negate one channel | Phase correlation tests |

## Running Specific Tests

```bash
# All tests in a fixture
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.*"

# Specific test
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.MonoSumming"

# Multiple patterns
./build/dev/OscilTests --gtest_filter="Signal*:Instance*"

# Exclude tests
./build/dev/OscilTests --gtest_filter="-*Slow*"
```

## CTest Presets

```bash
# Run via CTest with preset
ctest --preset dev

# Verbose output
ctest --preset dev -V

# Output on failure only
ctest --preset dev --output-on-failure
```

## Common Mistakes

❌ **Don't**: Use `EXPECT_EQ` for floating point comparison
✅ **Do**: Use `EXPECT_NEAR(a, b, tolerance)` or `EXPECT_FLOAT_EQ`

❌ **Don't**: Forget to add test file to `CMakeLists.txt`
✅ **Do**: Add to `add_executable(OscilTests ...)` section

❌ **Don't**: Forget to add source file being tested to OscilTests
✅ **Do**: Add the `.cpp` file if it's not already in the test executable

❌ **Don't**: Test with hard-coded magic numbers
✅ **Do**: Use named constants or calculated expected values

❌ **Don't**: Use `ASSERT_*` when test can continue
✅ **Do**: Use `EXPECT_*` for most assertions, `ASSERT_*` only for preconditions

❌ **Don't**: Write tests that depend on execution order
✅ **Do**: Each test must be independent (use SetUp/TearDown)

❌ **Don't**: Use long sleeps in thread tests
✅ **Do**: Use 50-100ms max with atomics for thread synchronization

❌ **Don't**: Allocate JUCE objects without proper initialization
✅ **Do**: Include JUCE headers and ensure event loop for UI tests

❌ **Don't**: Forget namespace
✅ **Do**: Use `using namespace oscil;` at top of test file

---

## E2E Test Harness

The E2E test harness is a standalone JUCE application that simulates a DAW environment for testing the plugin UI and behavior.

### Building the Test Harness

```bash
# Configure with test harness enabled
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness

# Run test harness (starts HTTP server on port 8765)
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"
```

### Test Harness HTTP API

The test harness exposes a REST API on `http://localhost:8765` for automation.

**Transport Control**:
```bash
# Start playback
curl -X POST http://localhost:8765/transport/play

# Stop playback
curl -X POST http://localhost:8765/transport/stop

# Set BPM
curl -X POST http://localhost:8765/transport/bpm -H "Content-Type: application/json" -d '{"bpm": 120}'

# Get transport state
curl http://localhost:8765/transport/state
```

**Track Audio Control**:
```bash
# Set track audio waveform (sine, square, sawtooth, triangle, noise, dc, silence)
curl -X POST http://localhost:8765/track/{id}/audio -H "Content-Type: application/json" \
  -d '{"waveform": "sine", "frequency": 440, "amplitude": 0.5}'

# Send audio burst
curl -X POST http://localhost:8765/track/{id}/burst -H "Content-Type: application/json" \
  -d '{"duration_ms": 100, "waveform": "sine"}'

# Get track info
curl http://localhost:8765/track/{id}/info
```

**UI Control**:
```bash
# Click element by test ID
curl -X POST http://localhost:8765/ui/click -H "Content-Type: application/json" \
  -d '{"elementId": "sidebar-timing-section"}'

# Set slider value
curl -X POST http://localhost:8765/ui/slider -H "Content-Type: application/json" \
  -d '{"elementId": "time-interval-slider", "value": 0.5}'

# Toggle button
curl -X POST http://localhost:8765/ui/toggle -H "Content-Type: application/json" \
  -d '{"elementId": "host-sync-toggle", "state": true}'

# Get element state
curl http://localhost:8765/ui/element/{elementId}
```

**Screenshot/Verification**:
```bash
# Take screenshot (saves to specified path)
curl -X POST http://localhost:8765/screenshot -H "Content-Type: application/json" \
  -d '{"path": "/tmp/screenshot.png"}'

# Verify waveform is rendering
curl http://localhost:8765/verify/waveform
```

**State Management**:
```bash
# Reset to default state
curl -X POST http://localhost:8765/state/reset

# Get oscillators state
curl http://localhost:8765/state/oscillators

# Get panes state
curl http://localhost:8765/state/panes

# Get sources state
curl http://localhost:8765/state/sources
```

**Health Check**:
```bash
curl http://localhost:8765/health
```

### Adding Test Element IDs

To make UI elements testable, implement the `TestableComponent` interface:

```cpp
#include "TestableComponent.h"

class MyComponent : public juce::Component, public TestableComponent
{
public:
    // Implement TestableComponent
    juce::String getTestId() const override { return "my-component-id"; }

    // Optional: provide element state for automation
    json getTestState() const override
    {
        return {
            {"visible", isVisible()},
            {"enabled", isEnabled()},
            {"customValue", myValue_}
        };
    }
};
```

### Writing E2E Test Scripts

E2E tests are typically written in Python or shell scripts that interact with the HTTP API:

```python
import requests
import time

BASE_URL = "http://localhost:8765"

def test_waveform_displays_on_play():
    # Reset to known state
    requests.post(f"{BASE_URL}/state/reset")

    # Configure track with sine wave
    requests.post(f"{BASE_URL}/track/0/audio", json={
        "waveform": "sine",
        "frequency": 440,
        "amplitude": 0.8
    })

    # Start playback
    requests.post(f"{BASE_URL}/transport/play")
    time.sleep(0.1)  # Wait for buffer to fill

    # Verify waveform is rendering
    response = requests.get(f"{BASE_URL}/verify/waveform")
    assert response.json()["success"] == True

    # Take screenshot for visual verification
    requests.post(f"{BASE_URL}/screenshot", json={
        "path": "screenshots/test_waveform_play.png"
    })

    # Stop playback
    requests.post(f"{BASE_URL}/transport/stop")

if __name__ == "__main__":
    test_waveform_displays_on_play()
    print("Test passed!")
```

### E2E Test Common Mistakes

❌ **Don't**: Run E2E tests without starting the test harness first
✅ **Do**: Start `OscilTestHarness` before running test scripts

❌ **Don't**: Use hard-coded pixel coordinates for UI interaction
✅ **Do**: Use test element IDs with the `TestableComponent` interface

❌ **Don't**: Assume immediate state changes after HTTP calls
✅ **Do**: Add short delays (50-100ms) after transport/audio changes

❌ **Don't**: Leave test harness running after tests complete
✅ **Do**: Clean up by stopping the test harness process
