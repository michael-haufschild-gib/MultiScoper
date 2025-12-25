# Oscil E2E Testing Guide

This document describes the end-to-end (E2E) testing system for the Oscil plugin, including setup instructions, test execution, and guidelines for writing new tests.

## Overview

The E2E testing system consists of:

1. **OscilTestHarness** - A standalone application that hosts the plugin with an HTTP API for remote control
2. **Python Test Framework** - pytest-based test suite with:
   - `OscilClient` - HTTP client for interacting with the harness
   - `LogAnalyzer` - Debug log parser for execution path verification
   - `OscilAssertions` - Custom assertions for plugin behavior
3. **Debug Instrumentation** - Runtime logging integrated into the plugin code

## Architecture

```
┌─────────────────────┐     HTTP      ┌─────────────────────┐
│   pytest Test       │───────────────│  OscilTestHarness   │
│                     │    :8765      │                     │
│  - OscilClient      │               │  - HTTP Server      │
│  - LogAnalyzer      │               │  - Plugin Editor    │
│  - Assertions       │               │  - TestDAW          │
└─────────────────────┘               └─────────────────────┘
         │                                      │
         │                                      │
         ▼                                      ▼
┌─────────────────────┐               ┌─────────────────────┐
│   debug.log         │◄──────────────│  Instrumented Code  │
│   (JSON entries)    │   writes      │  (#region agent log)│
└─────────────────────┘               └─────────────────────┘
```

## Setup

### Prerequisites

1. **Build the Test Harness**:
   ```bash
   cmake -B build -DOSCIL_BUILD_TEST_HARNESS=ON
   cmake --build build --target OscilTestHarness
   ```

2. **Install Python dependencies**:
   ```bash
   cd tests/e2e
   pip install pytest requests
   ```

### Running Tests

#### Start Test Harness Manually

```bash
./build/Test/OscilTestHarness_artefacts/Debug/OscilTestHarness.app/Contents/MacOS/OscilTestHarness
```

#### Run All Tests

```bash
cd tests/e2e
pytest -v
```

#### Run Specific Test File

```bash
pytest test_oscillators.py -v
```

#### Run Tests with Markers

```bash
# Run only GPU-related tests
pytest -m gpu -v

# Skip slow tests
pytest -m "not slow" -v

# Run visual regression tests
pytest -m visual -v
```

#### Run with Coverage Report

```bash
pytest --cov=. --cov-report=html
```

## Writing Tests

### Basic Test Structure

```python
import pytest
import time
from .oscil_client import OscilClient
from .log_analyzer import LogAnalyzer
from .assertions import OscilAssertions


class TestFeature:
    """Tests for a specific feature."""
    
    def test_basic_functionality(self, clean_state: OscilClient, analyzer: LogAnalyzer):
        """Description of what this test verifies."""
        # Arrange
        clean_state.clear_logs()
        analyzer.clear()
        
        # Act
        clean_state.add_oscillator("Test Osc")
        time.sleep(0.5)  # Wait for async operations
        
        # Assert via API
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
        
        # Assert via execution path
        analyzer.reload()
        assertions = OscilAssertions(analyzer)
        assertions.assert_panels_refreshed()
```

### Available Fixtures

| Fixture | Scope | Description |
|---------|-------|-------------|
| `harness_binary` | session | Path to test harness executable |
| `test_harness` | session | Running test harness process |
| `client` | function | Connected OscilClient instance |
| `clean_state` | function | Client with reset plugin state |
| `analyzer` | function | LogAnalyzer with cleared log |
| `assertions` | function | OscilAssertions instance |
| `setup_oscillator` | function | Client with one oscillator |
| `setup_two_oscillators` | function | Client with two oscillators |
| `gpu_enabled` | function | Client with GPU rendering on |
| `cpu_mode` | function | Client with CPU rendering on |

### OscilClient API

#### State Management

```python
client.reset()                              # Reset to default state
client.save_state("/path/to/file.xml")      # Save state
client.load_state("/path/to/file.xml")      # Load state
```

#### Oscillator Operations

```python
client.add_oscillator(name, source_id, pane_id, color, visible)
client.update_oscillator(id, name, visible, color, processing_mode)
client.get_oscillators()                    # Returns List[OscillatorInfo]
client.reorder_oscillators(order)           # Reorder by ID list
```

#### Pane Operations

```python
client.add_pane(name)
client.get_panes()                          # Returns List[PaneInfo]
client.swap_panes(pane_id_1, pane_id_2)
```

#### UI Interactions

```python
client.click(element_id)
client.double_click(element_id)
client.right_click(element_id)
client.toggle(element_id, state)
client.select(element_id, value)
client.slider(element_id, value)
client.type_text(element_id, text)
client.drag(from_element, to_element)
client.key_press(key, modifiers)
```

#### Rendering Control

```python
client.set_gpu_enabled(True)                # Enable GPU rendering
client.set_gpu_enabled(False)               # Enable CPU rendering
```

#### Logging

```python
client.clear_logs()                         # Clear debug log
client.get_logs(lines=100)                  # Get last N log entries
client.wait_for_log(pattern, timeout)       # Wait for log entry
```

### LogAnalyzer API

```python
analyzer = LogAnalyzer()
analyzer.reload()                           # Reload from file
analyzer.clear()                            # Clear log file

# Filtering
entries = analyzer.filter(
    hypothesis_id="H4",
    location="refreshPanels",
    message="Refreshing"
)

# Finding
entry = analyzer.find_first(location="setGpuRenderingEnabled")
entry = analyzer.find_last(message="Clearing")
count = analyzer.count_matches(location="paint")

# Execution paths
path = analyzer.extract_path("H4")
assert path.contains_sequence(["refreshPanels", "updateWaveform"])
```

### Custom Assertions

```python
assertions = OscilAssertions(analyzer)

# Rendering assertions
assertions.assert_gpu_enabled()
assertions.assert_gpu_disabled()
assertions.assert_waveform_cleared()
assertions.assert_waveform_rendered(waveform_id)
assertions.assert_background_color(red, green, blue, alpha)
assertions.assert_opaque_state("PaneComponent", expected_opaque)

# Panel assertions
assertions.assert_panels_refreshed(at_least=1)
assertions.assert_column_layout(expected_columns)

# Waveform assertions
assertions.assert_waveform_updated(waveform_id)
assertions.assert_unique_waveform_count(expected)

# Composite assertions
assertions.assert_gpu_toggle_clean()
assertions.assert_theme_applied()
assertions.assert_oscillator_added()
```

## Debug Instrumentation

Debug logs are written to `.cursor/debug.log` as JSON entries:

```json
{
  "hypothesisId": "H4",
  "location": "OscillatorPanelController.cpp:refreshPanels",
  "message": "Refreshing panels",
  "data": {
    "paneCountBefore": 1,
    "columnCount": 2
  },
  "timestamp": 1703419200000
}
```

### Adding Instrumentation

To add new instrumentation to the C++ code:

```cpp
// #region agent log
{ 
    std::ofstream f("/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log", std::ios::app);
    f << "{\"hypothesisId\":\"H_MY_TEST\","
      << "\"location\":\"MyFile.cpp:myFunction\","
      << "\"message\":\"Description\","
      << "\"data\":{\"key\":" << value << "},"
      << "\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
    f.close();
}
// #endregion
```

## Test Categories

### Test Markers

| Marker | Description |
|--------|-------------|
| `@pytest.mark.gpu` | Requires GPU rendering |
| `@pytest.mark.slow` | Takes more than 5 seconds |
| `@pytest.mark.visual` | Visual regression test |

### Test Files

| File | Coverage |
|------|----------|
| `test_oscillators.py` | Oscillator CRUD, visibility, colors |
| `test_timing.py` | Timing modes, BPM, intervals |
| `test_display_settings.py` | Gain, grid, layout, GPU toggle |
| `test_rendering.py` | GPU/CPU rendering, ghost images |
| `test_panes.py` | Pane management, reordering |
| `test_themes.py` | Theme selection, colors |
| `test_visual_presets.py` | Preset browser, effects |
| `test_dialogs.py` | Dialog interactions |
| `test_state_persistence.py` | Save/load state |
| `test_status_bar.py` | Metrics display |

## Troubleshooting

### Test Harness Won't Start

1. Check if already running: `curl http://127.0.0.1:8765/ping`
2. Verify build completed successfully
3. Check for port conflicts

### Tests Timing Out

1. Increase `timeout` in client initialization
2. Add longer `time.sleep()` after async operations
3. Use `wait_for_element()` instead of fixed sleeps

### Element Not Found

1. Verify element test ID in source code
2. Check if element is visible (`wait_for_visible()`)
3. Use `client.get_elements()` to list available elements

### Log Assertions Failing

1. Call `analyzer.reload()` after actions
2. Check hypothesis ID matches instrumentation
3. Use `analyzer.print_entries()` for debugging

## CI Integration

```yaml
# .github/workflows/e2e-tests.yml
name: E2E Tests

on: [push, pull_request]

jobs:
  e2e:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build Test Harness
        run: |
          cmake -B build -DOSCIL_BUILD_TEST_HARNESS=ON
          cmake --build build --target OscilTestHarness
      
      - name: Install Python deps
        run: pip install pytest requests
      
      - name: Start Test Harness
        run: |
          ./build/Test/OscilTestHarness.app/Contents/MacOS/OscilTestHarness &
          sleep 5
      
      - name: Run E2E Tests
        run: |
          cd tests/e2e
          pytest -v --tb=short
```

## Best Practices

1. **Always use fixtures** - Don't create clients manually
2. **Clear logs before actions** - Use `client.clear_logs(); analyzer.clear()`
3. **Wait for async operations** - Add `time.sleep()` after UI actions
4. **Reload analyzer** - Call `analyzer.reload()` before assertions
5. **Skip gracefully** - Use `pytest.skip()` when UI elements aren't available
6. **Test execution paths** - Verify behavior through logs, not just state
7. **Keep tests independent** - Each test should work in isolation
8. **Use descriptive names** - Test names should explain what's being verified


