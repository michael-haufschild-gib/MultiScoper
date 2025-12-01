# API Reference for LLM Coding Agents

**Purpose**: Class and struct reference for the Oscil codebase. Use this when you need to understand how to call existing APIs or implement interfaces.

## Core Classes

### InstanceRegistry

Singleton managing all active Oscil plugin instances for multi-instance coordination.

**Lifecycle Policy**:
- Sources must be registered once when active and explicitly unregistered when no longer needed.
- This applies to both DAW-provided sources and HTTP-created test sources.
- Test sources created via `/source/add` persist until `/source/remove` is called or the plugin state is reset.
- Failure to unregister sources leads to resource leaks (SharedCaptureBuffer memory) and may hit `MAX_TRACKS`.

```cpp
class InstanceRegistry
{
public:
    static InstanceRegistry& getInstance();

    // Register a new plugin instance as a signal source
    SourceId registerInstance(
        const juce::String& trackIdentifier,
        std::shared_ptr<SharedCaptureBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0);

    // Unregister a plugin instance
    void unregisterInstance(const SourceId& sourceId);

    // Get all available sources
    std::vector<SourceInfo> getAllSources() const;

    // Get a specific source by ID
    std::optional<SourceInfo> getSource(const SourceId& sourceId) const;

    // Get the capture buffer for a source
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer(const SourceId& sourceId) const;

    // Update source metadata
    void updateSource(const SourceId& sourceId, const juce::String& name,
                      int channelCount, double sampleRate);

    // Get source count
    size_t getSourceCount() const;

    // Listener management
    void addListener(InstanceRegistryListener* listener);
    void removeListener(InstanceRegistryListener* listener);
};
```

### SharedCaptureBuffer

Lock-free ring buffer for real-time audio capture.

```cpp
class SharedCaptureBuffer
{
public:
    static constexpr size_t DEFAULT_BUFFER_SAMPLES = 65536;
    static constexpr size_t MAX_CHANNELS = 2;

    explicit SharedCaptureBuffer(size_t bufferSamples = DEFAULT_BUFFER_SAMPLES);

    // Write from audio thread (lock-free)
    void write(const juce::AudioBuffer<float>& buffer,
               const CaptureFrameMetadata& metadata);

    void write(const float* const* samples, int numSamples, int numChannels,
               const CaptureFrameMetadata& metadata);

    // Read from UI thread
    int read(float* output, int numSamples, int channel = 0) const;
    int read(juce::AudioBuffer<float>& output, int numSamples) const;

    // Metadata and levels
    CaptureFrameMetadata getLatestMetadata() const;
    size_t getAvailableSamples() const;
    float getPeakLevel(int channel, int numSamples = 1024) const;
    float getRMSLevel(int channel, int numSamples = 1024) const;

    // Buffer management
    void clear();
    size_t getCapacity() const;
};
```

### SignalProcessor

Stateless processor for per-oscillator audio transforms.

```cpp
class SignalProcessor
{
public:
    // Process audio according to mode
    void process(const float* leftChannel, const float* rightChannel,
                 int numSamples, ProcessingMode mode,
                 ProcessedSignal& output) const;

    void process(const juce::AudioBuffer<float>& input,
                 ProcessingMode mode, ProcessedSignal& output) const;

    // Static analysis functions
    static float calculateCorrelation(const float* left, const float* right,
                                      int numSamples);
    static float calculatePeak(const float* samples, int numSamples);
    static float calculateRMS(const float* samples, int numSamples);

    // Decimation for display
    static void decimate(const float* input, int inputLength,
                         float* output, int outputLength,
                         bool preservePeaks = true);
};
```

### Oscillator

Represents a visualization unit with source, processing, and appearance settings.

```cpp
class Oscillator
{
public:
    Oscillator();
    explicit Oscillator(const juce::ValueTree& state);

    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& state);

    // Getters
    OscillatorId getId() const;
    SourceId getSourceId() const;
    ProcessingMode getProcessingMode() const;
    juce::Colour getColour() const;
    float getOpacity() const;
    PaneId getPaneId() const;
    int getOrderIndex() const;
    bool isVisible() const;
    juce::String getName() const;

    // Setters
    void setSourceId(const SourceId& sourceId);
    void setProcessingMode(ProcessingMode mode);
    void setColour(juce::Colour colour);
    void setOpacity(float opacity);
    void setPaneId(const PaneId& paneId);
    void setOrderIndex(int index);
    void setVisible(bool visible);
    void setName(const juce::String& name);

    // Helpers
    juce::Colour getEffectiveColour() const;
    bool isSingleTrace() const;
};
```

### OscilState

Central state manager with ValueTree serialization.

```cpp
class OscilState
{
public:
    OscilState();
    explicit OscilState(const juce::String& xmlString);

    // Serialization
    juce::String toXmlString() const;
    bool fromXmlString(const juce::String& xmlString);

    // Oscillator management
    std::vector<Oscillator> getOscillators() const;
    void addOscillator(const Oscillator& oscillator);
    void removeOscillator(const OscillatorId& oscillatorId);
    void updateOscillator(const Oscillator& oscillator);
    std::optional<Oscillator> getOscillator(const OscillatorId& oscillatorId) const;

    // Layout
    PaneLayoutManager& getLayoutManager();
    ColumnLayout getColumnLayout() const;
    void setColumnLayout(ColumnLayout layout);

    // Theme
    juce::String getThemeName() const;
    void setThemeName(const juce::String& themeName);

    // Schema version
    int getSchemaVersion() const;
};
```

### ThemeManager

Manages color themes with system and custom theme support.

```cpp
class ThemeManager
{
public:
    static ThemeManager& getInstance();

    // Current theme
    const ColorTheme& getCurrentTheme() const;
    bool setCurrentTheme(const juce::String& themeName);

    // Theme queries
    std::vector<juce::String> getAvailableThemes() const;
    const ColorTheme* getTheme(const juce::String& themeName) const;
    bool isSystemTheme(const juce::String& name) const;

    // Theme CRUD
    bool createTheme(const juce::String& name,
                     const juce::String& sourceTheme = "");
    bool updateTheme(const juce::String& name, const ColorTheme& theme);
    bool deleteTheme(const juce::String& name);
    bool cloneTheme(const juce::String& sourceName,
                    const juce::String& newName);

    // Import/export
    bool importTheme(const juce::String& json);
    juce::String exportTheme(const juce::String& name) const;

    // Listeners
    void addListener(ThemeManagerListener* listener);
    void removeListener(ThemeManagerListener* listener);
};
```

## Rendering Classes

### WaveformShader

Abstract base class for GPU-accelerated waveform visualization shaders.

```cpp
class WaveformShader
{
public:
    virtual ~WaveformShader() = default;

    // Shader identification
    [[nodiscard]] virtual juce::String getId() const = 0;
    [[nodiscard]] virtual juce::String getDisplayName() const = 0;
    [[nodiscard]] virtual juce::String getDescription() const = 0;
    [[nodiscard]] ShaderInfo getInfo() const;

#if OSCIL_ENABLE_OPENGL
    virtual bool compile(juce::OpenGLContext& context) = 0;
    virtual void release() = 0;
    [[nodiscard]] virtual bool isCompiled() const = 0;
    virtual void render(juce::OpenGLContext& context,
                        const std::vector<float>& channel1,
                        const std::vector<float>* channel2,
                        const ShaderRenderParams& params) = 0;
#endif

    virtual void renderSoftware(juce::Graphics& g,
                                const std::vector<float>& channel1,
                                const std::vector<float>* channel2,
                                const ShaderRenderParams& params);
};
```

### ShaderRegistry

Singleton managing shader registration and retrieval.

```cpp
class ShaderRegistry
{
public:
    [[nodiscard]] static ShaderRegistry& getInstance();

    // Shader management
    void registerShader(std::unique_ptr<WaveformShader> shader);
    [[nodiscard]] WaveformShader* getShader(const juce::String& shaderId);
    [[nodiscard]] std::vector<ShaderInfo> getAvailableShaders() const;
    [[nodiscard]] juce::String getDefaultShaderId() const;
    [[nodiscard]] bool hasShader(const juce::String& shaderId) const;

#if OSCIL_ENABLE_OPENGL
    void compileAll(juce::OpenGLContext& context);
    void releaseAll();
#endif
};
```

### WaveformGLRenderer

OpenGL renderer implementing `juce::OpenGLRenderer` for GPU-accelerated waveform display.

```cpp
class WaveformGLRenderer : public juce::OpenGLRenderer
{
public:
    // juce::OpenGLRenderer interface
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Waveform management
    void setContext(juce::OpenGLContext* context);
    void registerWaveform(int id);
    void unregisterWaveform(int id);
    void updateWaveform(const WaveformRenderData& data);
    void setBackgroundColour(juce::Colour colour);
    bool isReady() const;
    int getWaveformCount() const;
};
```

## Enums

### ProcessingMode

```cpp
enum class ProcessingMode
{
    FullStereo,  // L/R visualization (two traces)
    Mono,        // (L+R)/2 summed mono
    Mid,         // M=(L+R)/2
    Side,        // S=(L-R)/2
    Left,        // Left channel only
    Right        // Right channel only
};
```

### TimingMode

```cpp
enum class TimingMode
{
    TIME,     // Millisecond-based timing
    MELODIC   // Musical note-based timing (requires BPM)
};
```

### TriggerMode

```cpp
enum class TriggerMode
{
    FREE_RUNNING,  // Continuous display without sync
    HOST_SYNC,     // Sync to host transport (play/stop)
    TRIGGERED      // Trigger on signal threshold
};
```

### TriggerEdge

```cpp
enum class TriggerEdge
{
    Rising,
    Falling,
    Both
};
```

### NoteInterval

```cpp
enum class NoteInterval
{
    // Standard notes
    THIRTY_SECOND,   // 1/32
    SIXTEENTH,       // 1/16
    TWELFTH,         // 1/12 - Triplet eighth
    EIGHTH,          // 1/8
    QUARTER,         // 1/4
    HALF,            // 1/2
    WHOLE,           // 1/1 (1 bar in 4/4)

    // Multi-bar
    TWO_BARS, THREE_BARS, FOUR_BARS, EIGHT_BARS,

    // Dotted (1.5x duration)
    DOTTED_EIGHTH, DOTTED_QUARTER, DOTTED_HALF,

    // Triplets (2/3x duration)
    TRIPLET_EIGHTH, TRIPLET_QUARTER, TRIPLET_HALF
};
```

### ColumnLayout

```cpp
enum class ColumnLayout
{
    Single = 1,
    Double = 2,
    Triple = 3
};
```

## Data Structures

### SourceInfo

```cpp
struct SourceInfo
{
    SourceId sourceId;
    juce::String name;
    int channelCount = 2;
    double sampleRate = 44100.0;
    std::weak_ptr<SharedCaptureBuffer> buffer;
    std::atomic<bool> active;
};
```

### CaptureFrameMetadata

```cpp
struct CaptureFrameMetadata
{
    double sampleRate = 44100.0;
    int numChannels = 2;
    int64_t timestamp = 0;
    int numSamples = 0;
    bool isPlaying = false;
    double bpm = 120.0;
};
```

### ProcessedSignal

```cpp
struct ProcessedSignal
{
    std::vector<float> channel1;
    std::vector<float> channel2;
    int numSamples = 0;
    bool isStereo = false;

    void resize(int samples, bool stereo);
    void clear();
};
```

### ShaderInfo

```cpp
struct ShaderInfo
{
    juce::String id;           // Unique identifier (e.g., "basic")
    juce::String displayName;  // Human-readable name
    juce::String description;  // Brief description for tooltips
};
```

### ShaderRenderParams

```cpp
struct ShaderRenderParams
{
    juce::Colour colour;
    float opacity = 1.0f;
    float lineWidth = 1.5f;
    juce::Rectangle<float> bounds;
    bool isStereo = false;
};
```

### WaveformRenderData

```cpp
struct WaveformRenderData
{
    int id = 0;                          // Unique identifier
    juce::Rectangle<float> bounds;       // Screen position and size
    std::vector<float> channel1;         // First channel samples
    std::vector<float> channel2;         // Second channel (stereo only)
    juce::Colour colour{ 0xFF00FF00 };   // Waveform color
    float opacity = 1.0f;                // Opacity (0-1)
    float lineWidth = 1.5f;              // Line thickness
    bool isStereo = false;               // Whether to render channel2
    juce::String shaderId = "basic"; // Shader to use
    bool visible = true;                 // Whether to render this waveform
};
```

### ColorTheme

```cpp
struct ColorTheme
{
    juce::String name;
    bool isSystemTheme = false;

    // Background
    juce::Colour backgroundPrimary;
    juce::Colour backgroundSecondary;
    juce::Colour backgroundPane;

    // Grid
    juce::Colour gridMajor;
    juce::Colour gridMinor;
    juce::Colour gridZeroLine;

    // Text
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour textHighlight;

    // Controls
    juce::Colour controlBackground;
    juce::Colour controlBorder;
    juce::Colour controlHighlight;
    juce::Colour controlActive;

    // Status
    juce::Colour statusActive;
    juce::Colour statusWarning;
    juce::Colour statusError;

    // Waveform colors (64 total)
    std::vector<juce::Colour> waveformColors;

    juce::Colour getWaveformColor(int index) const;
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& state);
};
```

### TimingConfig

```cpp
struct TimingConfig
{
    // Mode selection
    TimingMode timingMode = TimingMode::TIME;
    TriggerMode triggerMode = TriggerMode::FREE_RUNNING;

    // TIME mode settings
    float timeIntervalMs = 50.0f;  // 1.0-60000.0 ms

    // MELODIC mode settings
    NoteInterval noteInterval = NoteInterval::QUARTER;

    // Host sync settings
    bool hostSyncEnabled = false;
    float hostBPM = 120.0f;        // 20.0-300.0 BPM
    bool syncToPlayhead = false;

    // Trigger settings
    float triggerThreshold = -20.0f;  // dBFS
    TriggerEdge triggerEdge = TriggerEdge::Rising;

    // Computed (read-only after calculateActualInterval)
    float actualIntervalMs = 50.0f;

    // Methods
    void calculateActualInterval();
    int getIntervalInSamples(float sampleRate) const;
    void setHostBPM(float bpm);
    void setTimingMode(TimingMode mode);
    void setTimeInterval(float ms);
    void setNoteInterval(NoteInterval interval);
    bool isHostSyncAvailable() const;
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
};
```

## Listener Interfaces

### InstanceRegistryListener

```cpp
class InstanceRegistryListener
{
public:
    virtual ~InstanceRegistryListener() = default;
    virtual void sourceAdded(const SourceId& sourceId) = 0;
    virtual void sourceRemoved(const SourceId& sourceId) = 0;
    virtual void sourceUpdated(const SourceId& sourceId) = 0;
};
```

### ThemeManagerListener

```cpp
class ThemeManagerListener
{
public:
    virtual ~ThemeManagerListener() = default;
    virtual void themeChanged(const ColorTheme& newTheme) = 0;
};
```

## Common API Usage Patterns

### Getting a Capture Buffer from a Source

```cpp
auto& registry = InstanceRegistry::getInstance();
auto buffer = registry.getCaptureBuffer(sourceId);
if (buffer)
{
    // Read audio data
    std::vector<float> output(1024);
    int samplesRead = buffer->read(output.data(), 1024, 0);
}
```

### Processing Audio for Display

```cpp
SignalProcessor processor;
ProcessedSignal output;

// Get raw audio from buffer
buffer->read(rawAudio.data(), numSamples, 0);

// Process according to mode
processor.process(leftChannel, rightChannel, numSamples, mode, output);

// Decimate for display width
SignalProcessor::decimate(output.channel1.data(), output.numSamples,
                          displayBuffer.data(), displayWidth, true);
```

### Listening for Source Changes

```cpp
class MyComponent : public juce::Component, public InstanceRegistryListener
{
public:
    MyComponent()
    {
        InstanceRegistry::getInstance().addListener(this);
    }

    ~MyComponent() override
    {
        InstanceRegistry::getInstance().removeListener(this);
    }

    void sourceAdded(const SourceId& id) override { /* refresh UI */ }
    void sourceRemoved(const SourceId& id) override { /* refresh UI */ }
    void sourceUpdated(const SourceId& id) override { /* refresh UI */ }
};
```

## Common Mistakes

❌ **Don't**: Call `write()` from UI thread
✅ **Do**: Only call `SharedCaptureBuffer::write()` from audio thread

❌ **Don't**: Hold shared_ptr to buffer longer than needed
✅ **Do**: Get buffer reference when needed, release promptly

❌ **Don't**: Forget to remove listeners in destructor
✅ **Do**: Always pair `addListener()` with `removeListener()`

❌ **Don't**: Modify ColorTheme from system themes
✅ **Do**: Clone system theme before modifying: `cloneTheme("Dark", "MyTheme")`

❌ **Don't**: Use ProcessedSignal without calling `resize()` first
✅ **Do**: Call `output.resize(numSamples, isStereo)` before processing

---

## Test Harness HTTP API

The test harness provides a REST API on port 8765 for automated E2E testing.

### Health Check

```
GET /health
Response: { success: true, data: { status: "ok", running: true, tracks: 2 } }
```

### Transport Control

```
POST /transport/play           - Start playback
POST /transport/stop           - Stop playback
POST /transport/setBpm         - Body: { bpm: 120.0 }
POST /transport/setPosition    - Body: { samples: 0 }
GET  /transport/state          - Get transport state
```

### Track Audio Control

```
POST /track/{id}/audio  - Body: { waveform: "sine", frequency: 440, amplitude: 0.8 }
POST /track/{id}/burst  - Body: { samples: 4410, waveform: "sine" }
GET  /track/{id}/info   - Get track info
```

### UI Mouse Interactions

```
POST /ui/click           - Body: { elementId: "button-1" }
POST /ui/doubleClick     - Body: { elementId: "slider-1" }
POST /ui/rightClick      - Body: { elementId: "item-1" }
POST /ui/hover           - Body: { elementId: "button-1", durationMs: 500 }
POST /ui/drag            - Body: { from: "item-1", to: "pane-2", shift: false, alt: false }
POST /ui/dragOffset      - Body: { elementId: "slider-1", deltaX: 50, deltaY: 0, alt: true }
POST /ui/scroll          - Body: { elementId: "slider-1", deltaY: 0.1, shift: false }
POST /ui/select          - Body: { elementId: "dropdown-1", itemId: 2 } or { itemText: "Option" }
POST /ui/toggle          - Body: { elementId: "checkbox-1", value: true }
POST /ui/slider          - Body: { elementId: "slider-1", value: 0.5 }
POST /ui/slider/increment- Body: { elementId: "slider-1" }
POST /ui/slider/decrement- Body: { elementId: "slider-1" }
POST /ui/slider/reset    - Body: { elementId: "slider-1" } (double-click to reset)
```

### UI Keyboard Interactions

```
POST /ui/keyPress  - Body: { key: "escape", elementId: "", shift: false, alt: false, ctrl: false }
                   - Keys: escape, enter, space, tab, up, down, left, right, home, end, delete, f1-f12
POST /ui/typeText  - Body: { elementId: "textfield-1", text: "hello" }
POST /ui/clearText - Body: { elementId: "textfield-1" }
```

### UI Focus Management

```
POST /ui/focus         - Body: { elementId: "button-1" }
GET  /ui/focused       - Get currently focused element
POST /ui/focusNext     - Move focus to next element
POST /ui/focusPrevious - Move focus to previous element
```

### UI Wait Conditions

```
POST /ui/waitForElement - Body: { elementId: "modal-1", timeoutMs: 5000 }
POST /ui/waitForVisible - Body: { elementId: "panel-1", timeoutMs: 5000 }
POST /ui/waitForEnabled - Body: { elementId: "button-1", timeoutMs: 5000 }
```

### UI State Queries

```
GET /ui/state           - Get full UI state with all elements
GET /ui/elements        - List all registered test elements
GET /ui/element/{id}    - Get specific element info
```

### Screenshot & Visual Verification

```
POST /screenshot         - Body: { path: "/tmp/shot.png", element: "window" or testId }
POST /screenshot/compare - Body: { elementId: "panel-1", baseline: "/path/baseline.png", tolerance: 5 }
POST /baseline/save      - Body: { elementId: "panel-1", directory: "/path/baselines", name: "panel" }
```

### Visual Verification

```
POST /verify/waveform - Body: { elementId: "waveform-1", minAmplitude: 0.05 }
POST /verify/color    - Body: { elementId: "panel-1", color: "#FF0000", tolerance: 10, mode: "background"|"contains" }
POST /verify/bounds   - Body: { elementId: "panel-1", width: 400, height: 300, tolerance: 5 }
POST /verify/visible  - Body: { elementId: "panel-1" }
GET  /analyze/waveform?elementId=waveform-1&backgroundColor=#000000
```

### State Management

```
POST /state/reset      - Reset plugin state
POST /state/save       - Body: { path: "/tmp/state.xml" }
POST /state/load       - Body: { path: "/tmp/state.xml" }
GET  /state/oscillators- List all oscillators
GET  /state/panes      - List all panes
GET  /state/sources    - List all available sources
```

### Performance Metrics

```
POST /metrics/start       - Body: { intervalMs: 100 } - Start collection
POST /metrics/stop        - Stop collection
GET  /metrics/current     - Get current FPS, CPU%, Memory
GET  /metrics/stats       - Get statistics (avgFps, minFps, maxFps, avgCpu, maxCpu, avgMemoryMB)
GET  /metrics/stats?periodMs=5000 - Stats for last N milliseconds
POST /metrics/reset       - Clear all collected data
POST /metrics/recordFrame - Record a frame render time
```

### Test Element IDs

UI components are registered with testIds following these patterns:

```
sidebar-source-list              - Source list component
sidebar-source-{sourceId}        - Individual source item
sidebar-add-oscillator           - Add oscillator button
pane-{paneId}                    - Pane component
waveform-{oscillatorId}          - Waveform display
oscillator-panel-{oscillatorId}  - Oscillator control panel
button-{name}                    - Buttons
slider-{name}                    - Sliders
dropdown-{name}                  - Dropdowns
toggle-{name}                    - Toggle switches
tabs-{name}                      - Tab bars
```
