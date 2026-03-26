# API Guide

**Purpose**: How to use the Test Harness HTTP API for E2E testing and how to create new API handlers.

**Base URL**: `http://localhost:8765`

---

## Test Harness Architecture

The E2E test harness hosts the plugin and exposes HTTP endpoints for automation.

```
OscilTestHarness (Standalone App)
    │
    ├── PluginTestServer (HTTP Server on :8765)
    │       │
    │       ├── TestServerHandlerBase (Abstract base)
    │       │       ├── OscillatorHandler
    │       │       ├── SourceHandler
    │       │       ├── LayoutHandler
    │       │       ├── WaveformHandler
    │       │       ├── StateHandler
    │       │       ├── ScreenshotHandler
    │       │       └── TestRunnerHandler
    │       │
    │       └── TestElementRegistry (UI element lookup by testId)
    │
    └── OscilPluginProcessor + OscilPluginEditor
```

---

## Using the HTTP API

### 1. Start the Test Harness
```bash
# Build
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness

# Run
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &
```

### 2. Open the Editor (CRITICAL)
```bash
curl -X POST http://localhost:8765/track/0/showEditor
sleep 1  # Wait for UI to initialize
```

### 3. Interact with UI Elements
```bash
# Click a button by testId
curl -X POST http://localhost:8765/ui/click \
  -H "Content-Type: application/json" \
  -d '{"elementId": "sidebar_oscillators_add"}'

# Get element info
curl http://localhost:8765/ui/element/sidebar_oscillators_add
```

---

## API Endpoint Patterns

### UI Interaction Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/ui/element/{testId}` | Get element bounds and state |
| POST | `/ui/click` | Click element by testId |
| POST | `/ui/setValue` | Set value on slider/input |
| GET | `/ui/screenshot` | Capture current UI state |

**Click Request**:
```json
{
  "elementId": "button_testId"
}
```

**Element Response**:
```json
{
  "id": "button_testId",
  "type": "OscilButton",
  "bounds": { "x": 10, "y": 20, "width": 100, "height": 30 },
  "visible": true,
  "enabled": true
}
```

### Oscillator Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/oscillators` | List all oscillators |
| POST | `/oscillator/add` | Add new oscillator |
| DELETE | `/oscillator/{id}` | Delete oscillator |
| PUT | `/oscillator/{id}` | Update oscillator |

**Add Oscillator**:
```json
{
  "name": "Bass",
  "sourceId": "source_123",
  "processingMode": "Mono",
  "colour": "#FF5500"
}
```

### Source Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/sources` | List available sources |
| POST | `/source/add` | Create test source |
| DELETE | `/source/{id}` | Remove test source |

### State Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/state` | Get full plugin state |
| POST | `/state/reset` | Reset to defaults |
| GET | `/state/timing` | Get timing config |
| PUT | `/state/timing` | Update timing |

---

## Creating a New HTTP Handler

### Step 1: Create Header
**Location**: `include/tools/test_server/MyHandler.h`

```cpp
#pragma once

#include "tools/test_server/TestServerHandlerBase.h"

namespace oscil
{

class MyHandler : public TestServerHandlerBase
{
public:
    explicit MyHandler(PluginTestServer& server);

    void registerRoutes(httplib::Server& httpServer) override;

private:
    void handleGetThing(const httplib::Request& req, httplib::Response& res);
    void handlePostThing(const httplib::Request& req, httplib::Response& res);
};

} // namespace oscil
```

### Step 2: Create Implementation
**Location**: `src/tools/test_server/MyHandler.cpp`

```cpp
#include "tools/test_server/MyHandler.h"
#include "tools/test_server/PluginTestServer.h"
#include <nlohmann/json.hpp>

namespace oscil
{

MyHandler::MyHandler(PluginTestServer& server)
    : TestServerHandlerBase(server)
{
}

void MyHandler::registerRoutes(httplib::Server& httpServer)
{
    httpServer.Get("/my/thing", [this](const auto& req, auto& res) {
        handleGetThing(req, res);
    });

    httpServer.Post("/my/thing", [this](const auto& req, auto& res) {
        handlePostThing(req, res);
    });
}

void MyHandler::handleGetThing(const httplib::Request& req, httplib::Response& res)
{
    // Access plugin via server reference
    auto* processor = server_.getProcessor();
    if (!processor)
    {
        sendError(res, 500, "No processor available");
        return;
    }

    nlohmann::json response;
    response["status"] = "ok";
    response["data"] = "...";

    sendJson(res, response);
}

void MyHandler::handlePostThing(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto json = nlohmann::json::parse(req.body);
        // Process request...

        sendJson(res, {{"success", true}});
    }
    catch (const std::exception& e)
    {
        sendError(res, 400, e.what());
    }
}

} // namespace oscil
```

### Step 3: Register Handler
In `PluginTestServer.cpp`:

```cpp
#include "tools/test_server/MyHandler.h"

void PluginTestServer::initializeHandlers()
{
    // ... existing handlers ...
    handlers_.push_back(std::make_unique<MyHandler>(*this));
}
```

### Step 4: Update Sources.cmake
```cmake
# In cmake/Sources.cmake, add:
${CMAKE_SOURCE_DIR}/src/tools/test_server/MyHandler.cpp
```

---

## TestServerHandlerBase Helpers

```cpp
class TestServerHandlerBase
{
protected:
    // Send JSON response
    void sendJson(httplib::Response& res, const nlohmann::json& json);

    // Send error response
    void sendError(httplib::Response& res, int code, const std::string& message);

    // Access server
    PluginTestServer& server_;

    // Get processor/editor (may be null!)
    OscilPluginProcessor* getProcessor();
    OscilPluginEditor* getEditor();

    // Run on message thread (required for UI operations)
    void runOnMessageThread(std::function<void()> fn);
};
```

---

## Test Element Registry

UI components register themselves for E2E testing via `TestIdSupport`:

```cpp
// In component constructor
class MyComponent : public juce::Component, public TestIdSupport
{
public:
    MyComponent()
    {
        OSCIL_REGISTER_TEST_ID("my_component_testId");
    }
};
```

**Lookup from handler**:
```cpp
auto* element = TestElementRegistry::getInstance().findElement("my_component_testId");
if (element)
{
    auto bounds = element->getBoundsInWindow();
    // ...
}
```

---

## Python Test Example

```python
import requests
import time

BASE = "http://localhost:8765"

def test_add_oscillator():
    # 1. Open editor
    requests.post(f"{BASE}/track/0/showEditor")
    time.sleep(1)

    # 2. Get initial state
    state = requests.get(f"{BASE}/oscillators").json()
    initial_count = len(state["oscillators"])

    # 3. Add oscillator via UI click
    requests.post(f"{BASE}/ui/click", json={"elementId": "sidebar_oscillators_add"})
    time.sleep(0.5)

    # 4. Verify
    state = requests.get(f"{BASE}/oscillators").json()
    assert len(state["oscillators"]) == initial_count + 1
```

---

## Common Mistakes

**Don't**: Access UI without opening editor first
```python
# Bad: Editor not open
requests.get(f"{BASE}/ui/element/my_button")  # Returns 404
```
**Do**: Always call `/track/0/showEditor` first

---

**Don't**: Modify UI state from non-message thread
```cpp
// Bad: Direct UI access from HTTP thread
editor_->doSomething();
```
**Do**: Use `runOnMessageThread()`
```cpp
runOnMessageThread([this]() {
    if (auto* editor = getEditor())
        editor->doSomething();
});
```

---

**Don't**: Forget to wait after UI actions
```python
# Bad: No wait
requests.post(f"{BASE}/ui/click", json={"elementId": "button"})
state = requests.get(f"{BASE}/state").json()  # May not reflect click yet
```
**Do**: Add small delays for UI to update
```python
requests.post(f"{BASE}/ui/click", json={"elementId": "button"})
time.sleep(0.3)  # Wait for UI update
state = requests.get(f"{BASE}/state").json()
```
