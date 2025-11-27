# API Reference for LLM Coding Agents

**Purpose**: Class and struct reference for the Oscil codebase. Use this when you need to understand how to call existing APIs or implement interfaces.

## Core Classes

### InstanceRegistry

Singleton managing all active Oscil plugin instances for multi-instance coordination.

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

    // Migration
    int getSchemaVersion() const;
    bool needsMigration() const;
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
