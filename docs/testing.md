# Testing Guide for LLM Coding Agents

**Purpose**: Instructions for writing and running tests in Oscil.
**Test Stack**: GoogleTest (C++ unit tests) | pytest (Python E2E tests)

---

## Test Locations

| Test Type     | Location                    | Naming Pattern          | Framework  |
| ------------- | --------------------------- | ----------------------- | ---------- |
| Unit tests    | `tests/`                    | `test_[snake_case].cpp` | GoogleTest |
| E2E tests     | `tests/e2e/`                | `test_[snake_case].py`  | pytest     |
| Test fixtures | `tests/OscilTestFixtures.h` | N/A                     | GoogleTest |

---

## Running Tests

| Command                                           | Purpose            |
| ------------------------------------------------- | ------------------ |
| `ctest --preset dev`                              | Run all unit tests |
| `ctest --preset dev -R "Pattern"`                 | Run matching tests |
| `ctest --preset dev --output-on-failure`          | Verbose on failure |
| `./build/dev/OscilTests --gtest_filter="Class.*"` | GoogleTest filter  |
| `./build/dev/OscilTests --gtest_list_tests`       | List all tests     |

---

## Unit Test Template

### Basic Model Test

```cpp
/*
    Oscil - [ClassName] Tests
*/

#include <gtest/gtest.h>
#include "core/[ClassName].h"

using namespace oscil;

class [ClassName]Test : public ::testing::Test
{
protected:
    void SetUp() override { /* Called before each test */ }
    void TearDown() override { /* Called after each test */ }
};

TEST_F([ClassName]Test, DescriptiveBehaviorName)
{
    [ClassName] obj;
    obj.setValue(42);
    EXPECT_EQ(obj.getValue(), 42);
}

TEST_F([ClassName]Test, InvalidInput_ReturnsError)
{
    [ClassName] obj;
    EXPECT_FALSE(obj.setInvalidValue(-1));
}
```

### UI Component Test

UI components require `ThemeManager`:

```cpp
#include <gtest/gtest.h>
#include "ui/components/Oscil[Name].h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class Oscil[Name]Test : public ::testing::Test
{
protected:
    std::unique_ptr<ThemeManager> themeManager_;

    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }
    void TearDown() override { themeManager_.reset(); }
    ThemeManager& getThemeManager() { return *themeManager_; }
};

TEST_F(Oscil[Name]Test, ConstructionAndBasicBehavior)
{
    Oscil[Name] component(getThemeManager());
    component.setEnabled(false);
    EXPECT_FALSE(component.isEnabled());
}
```

---

## Adding New Tests

1. Create test file: `tests/test_[feature].cpp`
2. Build: `cmake --build --preset dev`
3. Run: `ctest --preset dev -R "[Feature]"`

Tests are auto-discovered via glob in CMakeLists.txt — no manual registration needed.

---

## Test Naming

```cpp
// Good: Descriptive behavior
TEST_F(OscillatorTest, ProcessingModeConversion)
TEST_F(OscillatorTest, OpacityClamping_AtMinimum)
TEST_F(OscillatorTest, InvalidInput_ReturnsError)

// Bad: Vague names
TEST_F(OscillatorTest, Test1)
TEST_F(OscillatorTest, Works)
```

---

## Common Mistakes

**Don't**: Test implementation details

```cpp
EXPECT_EQ(component.internalBuffer_.size(), 1024);  // BAD: private state
```

**Do**: Test public behavior

```cpp
EXPECT_EQ(component.getBufferCapacity(), 1024);  // GOOD
```

**Don't**: Forget ThemeManager for UI tests — won't compile.

**Don't**: Forget to remove listeners in TearDown — causes dangling pointers.

**Don't**: Write flaky timing-dependent tests — test state, not timing.

**Don't**: Use raw `TEST()` for tests needing setup — use `TEST_F()` with fixture.

---

## E2E Testing

See `docs/e2e-testing.md` for full E2E guide. Quick reference:

```bash
# Build and start harness
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &

# Run tests
cd tests/e2e && python -m pytest -v
```

---

## On-Demand References

| Domain                                                | Serena Memory               |
| ----------------------------------------------------- | --------------------------- |
| Listener mock, serialization round-trip, E2E patterns | `testing-detailed-patterns` |
| MockInstanceRegistry, full fixture usage              | `testing-detailed-patterns` |
