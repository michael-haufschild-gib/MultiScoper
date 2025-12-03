# Testing Guide for LLM Coding Agents

**Purpose**: Teach agents HOW to write and run tests for this JUCE audio plugin.

**Tech Stack**: GoogleTest (unit tests), Test Harness HTTP API (E2E tests)

---

## Testing Architecture

```
tests/                      # Unit tests (GoogleTest)
├── test_oscillator.cpp     # Model tests
├── test_signal_processor.cpp
├── test_timing_engine.cpp
├── test_oscil_button.cpp   # UI component tests
└── ...

test_harness/               # E2E test infrastructure
├── src/
│   ├── TestTrack.cpp
│   ├── TestHttpServer.cpp
│   └── TestUIController.cpp
└── include/
```

---

## Unit Testing (GoogleTest)

### How to Write a Unit Test

**Location**: `tests/test_{name}.cpp`

**Template**:
```cpp
/*
    Oscil - {ClassName} Tests
*/

#include <gtest/gtest.h>
#include "core/MyClass.h"  // Include what you're testing

using namespace oscil;

class MyClassTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Called before each test
    }

    void TearDown() override
    {
        // Called after each test
    }

    // Test fixtures
    MyClass instance_;
};

// Test naming: {Behavior}_{Condition}
TEST_F(MyClassTest, DefaultConstruction)
{
    MyClass obj;
    EXPECT_TRUE(obj.getId().isValid());
    EXPECT_EQ(obj.getValue(), 0);
}

TEST_F(MyClassTest, SetterUpdatesValue)
{
    instance_.setValue(42);
    EXPECT_EQ(instance_.getValue(), 42);
}

TEST_F(MyClassTest, InvalidInput_ReturnsError)
{
    EXPECT_FALSE(instance_.setInvalidValue(-1));
}
```

**Key Points**:
- Use `TEST_F` with fixture class for shared setup
- Use `TEST` for standalone tests
- Name tests descriptively: `{What}_{Condition}` or `{Action}_{ExpectedResult}`

### UI Component Test Pattern

UI components require JUCE initialization:

```cpp
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/OscilButton.h"
#include "TestElementRegistry.h"

using namespace oscil;

class OscilButtonTest : public ::testing::Test
{
protected:
    // No SetUp needed - global JuceTestEnvironment handles initialization
};

TEST_F(OscilButtonTest, Construction)
{
    OscilButton button("Click Me", "test_button");

    EXPECT_EQ(button.getText(), "Click Me");
    EXPECT_TRUE(button.isEnabled());
}

TEST_F(OscilButtonTest, TestIdRegistration)
{
    OscilButton button("Test", "my_test_id");

    auto* found = oscil::test::TestElementRegistry::getInstance()
                      .findElement("my_test_id");
    EXPECT_EQ(found, &button);
}

TEST_F(OscilButtonTest, ClickCallback)
{
    OscilButton button;
    bool clicked = false;

    button.onClick = [&clicked]() { clicked = true; };
    button.triggerClick();

    EXPECT_TRUE(clicked);
}
```

### Mock/Dummy Pattern

For testing with dependencies:

```cpp
// Dummy implementation of interface
class DummyRegistry : public IInstanceRegistry
{
public:
    SourceId registerInstance(const juce::String&,
                              std::shared_ptr<SharedCaptureBuffer>,
                              const juce::String&, int, double) override
    {
        return SourceId::generate();
    }

    void unregisterInstance(const SourceId&) override {}

    std::vector<SourceInfo> getAllSources() const override
    {
        return testSources_;  // Return test data
    }

    // ... implement other methods ...

    // Test control
    std::vector<SourceInfo> testSources_;
};

TEST_F(MyComponentTest, UsesRegistry)
{
    DummyRegistry registry;
    registry.testSources_ = { /* test data */ };

    MyComponent component(registry);
    // Test component behavior with mock data
}
```

---

## Running Tests

### All Unit Tests
```bash
ctest --preset dev
```

### Specific Test
```bash
# Run tests matching pattern
ctest --preset dev -R "Oscillator"

# Run single test binary with filter
./build/dev/OscilTests --gtest_filter="OscillatorTest.*"
```

### Verbose Output
```bash
ctest --preset dev --output-on-failure
```

### List Available Tests
```bash
./build/dev/OscilTests --gtest_list_tests
```

---

## Test Naming Conventions

| Pattern | Example | When to Use |
|---------|---------|-------------|
| `test_{class}.cpp` | `test_oscillator.cpp` | One class per file |
| `test_{feature}_{aspect}.cpp` | `test_timing_config.cpp` | Feature-specific tests |

### Test Case Naming

```cpp
// Good: Descriptive behavior
TEST_F(OscillatorTest, IdGeneration)
TEST_F(OscillatorTest, DefaultValues)
TEST_F(OscillatorTest, SettersAndGetters)
TEST_F(OscillatorTest, ProcessingModeConversion)

// Bad: Vague names
TEST_F(OscillatorTest, Test1)
TEST_F(OscillatorTest, Works)
```

---

## Adding a New Test File

### Step 1: Create Test File
```cpp
// tests/test_myfeature.cpp

#include <gtest/gtest.h>
#include "core/MyFeature.h"

using namespace oscil;

class MyFeatureTest : public ::testing::Test { };

TEST_F(MyFeatureTest, BasicBehavior)
{
    // ...
}
```

### Step 2: Update CMakeLists.txt
Tests are auto-discovered via glob in CMakeLists.txt, but verify:
```cmake
# In CMakeLists.txt, tests section
file(GLOB TEST_SOURCES "tests/test_*.cpp")
```

### Step 3: Build and Run
```bash
cmake --build --preset dev
ctest --preset dev -R "MyFeature"
```

---

## E2E Testing with Test Harness

### Build Test Harness
```bash
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
```

### Start Harness
```bash
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &
```

### Python E2E Test Pattern
```python
import requests
import time
import pytest

BASE_URL = "http://localhost:8765"

@pytest.fixture(scope="module")
def editor():
    """Ensure editor is open before tests."""
    requests.post(f"{BASE_URL}/track/0/showEditor")
    time.sleep(1)
    yield
    # Cleanup if needed

def test_oscillator_creation(editor):
    # Get initial count
    initial = requests.get(f"{BASE_URL}/oscillators").json()
    initial_count = len(initial["oscillators"])

    # Add oscillator
    requests.post(f"{BASE_URL}/oscillator/add", json={
        "name": "Test Osc",
        "processingMode": "Mono"
    })

    # Verify
    after = requests.get(f"{BASE_URL}/oscillators").json()
    assert len(after["oscillators"]) == initial_count + 1

def test_ui_click_button(editor):
    # Click add button
    response = requests.post(f"{BASE_URL}/ui/click", json={
        "elementId": "sidebar_oscillators_add"
    })
    assert response.status_code == 200
    time.sleep(0.5)
```

---

## Testing Patterns

### 1. Model Tests
Test domain logic in isolation:
```cpp
TEST_F(OscillatorTest, ProcessingModeAffectsOutput)
{
    Oscillator osc;
    osc.setProcessingMode(ProcessingMode::Mono);
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mono);
}
```

### 2. State Serialization Tests
Ensure state survives save/load:
```cpp
TEST_F(OscilStateTest, SerializationRoundTrip)
{
    OscilState state;
    state.addOscillator(createTestOscillator());

    // Serialize
    auto data = state.serialize();

    // Deserialize
    OscilState restored;
    restored.deserialize(data);

    EXPECT_EQ(restored.getOscillatorCount(), 1);
}
```

### 3. Listener Tests
Verify callbacks are triggered:
```cpp
class MockListener : public MyComponent::Listener
{
public:
    int callCount = 0;
    void onValueChanged(float value) override { callCount++; lastValue = value; }
    float lastValue = 0;
};

TEST_F(MyComponentTest, NotifiesListeners)
{
    MyComponent component;
    MockListener listener;
    component.addListener(&listener);

    component.setValue(42.0f);

    EXPECT_EQ(listener.callCount, 1);
    EXPECT_EQ(listener.lastValue, 42.0f);

    component.removeListener(&listener);
}
```

### 4. Boundary Tests
Test edge cases:
```cpp
TEST_F(OscillatorTest, OpacityClamping)
{
    Oscillator osc;

    osc.setOpacity(-0.5f);
    EXPECT_GE(osc.getOpacity(), 0.0f);

    osc.setOpacity(1.5f);
    EXPECT_LE(osc.getOpacity(), 1.0f);
}
```

---

## Common Mistakes

**Don't**: Test implementation details
```cpp
// Bad: Testing private state
EXPECT_EQ(component.internalBuffer_.size(), 1024);
```
**Do**: Test public behavior
```cpp
// Good: Testing observable behavior
EXPECT_EQ(component.getBufferCapacity(), 1024);
```

---

**Don't**: Forget to clean up listeners
```cpp
// Bad: Listener outlives component
component.addListener(&listener);
// listener goes out of scope while still registered
```
**Do**: Always remove listeners
```cpp
component.addListener(&listener);
// ... test ...
component.removeListener(&listener);
```

---

**Don't**: Write flaky timing-dependent tests
```cpp
// Bad: Arbitrary sleep
component.startAnimation();
std::this_thread::sleep_for(100ms);
EXPECT_TRUE(component.isAnimating());  // May fail!
```
**Do**: Test state directly or use proper synchronization

---

## Quick Reference

| Command | Purpose |
|---------|---------|
| `ctest --preset dev` | Run all tests |
| `ctest --preset dev -R "Pattern"` | Run matching tests |
| `ctest --preset dev -V` | Verbose output |
| `./build/dev/OscilTests --gtest_filter="*"` | Run with GoogleTest filter |
| `./build/dev/OscilTests --gtest_list_tests` | List all tests |
