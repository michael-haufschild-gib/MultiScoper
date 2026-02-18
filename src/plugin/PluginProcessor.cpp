/*
    Oscil - Plugin Processor Implementation
    Main audio plugin processor
*/

#include "plugin/PluginProcessor.h"

#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "core/DebugLogger.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "tools/PerformanceProfiler.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace oscil
{

OscilPluginProcessor::OscilPluginProcessor(IInstanceRegistry& instanceRegistry, 
                                           IThemeService& themeService, 
                                           ShaderRegistry& shaderRegistry, 
                                           MemoryBudgetManager& memoryBudgetManager,
                                           AudioCapturePool& audioCapturePool,
                                           CaptureThread& captureThread)
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , instanceRegistry_(instanceRegistry)
    , themeService_(themeService)
    , shaderRegistry_(shaderRegistry)
    , memoryBudgetManager_(memoryBudgetManager)
    , capturePool_(audioCapturePool)    // Reference to centralized pool
    , captureThread_(captureThread)      // Reference to centralized thread
{
    // Create capture buffer (legacy path - always available as fallback)
    captureBuffer_ = std::make_shared<DecimatingCaptureBuffer>();
    analysisEngine_ = std::make_shared<AnalysisEngine>();

    // Generate unique track identifier (used for registry and source ID)
    trackIdentifier_ = juce::Uuid().toString();

    // Generate provisional instanceUUID (may be overwritten by setStateInformation)
    // This UUID persists across DAW sessions for reliable source reconnection
    instanceUUID_ = juce::Uuid().toString();

    // Initialize MemoryBudgetManager with config from state
    // Use default sample rate until prepareToPlay is called
    auto config = state_.getCaptureQualityConfig();
    setCaptureQualityConfig(config);
    memoryBudgetManager_.setGlobalConfig(config,
                                         static_cast<int>(currentSampleRate_));

    // Listen for state changes (quality settings)
    state_.getState().addListener(this);

    // Initialize cached state for real-time safe getStateInformation()
    updateCachedState();

    // SYNCHRONOUS slot allocation (defense-in-depth)
    // Constructor is always called on message thread, so this is safe.
    // Slot is allocated NOW so processBlock never sees INVALID_SLOT.
    // Using a temporary sourceId based on track identifier (updated in prepareToPlay when registered)
    SourceId tempSourceId{trackIdentifier_};  // Will be updated when registered with InstanceRegistry
    int decimationRatio = config.getDecimationRatio(static_cast<int>(currentSampleRate_));
    int slotIndex = capturePool_.allocateSlot(tempSourceId, currentSampleRate_, decimationRatio);
    captureSlotIndex_.store(slotIndex, std::memory_order_release);
    
    // Mark lock-free path as ready if slot allocation succeeded
    if (slotIndex != AudioCapturePool::INVALID_SLOT)
    {
        lockFreeReady_.store(true, std::memory_order_release);
        OSCIL_LOG_LIFECYCLE("PluginProcessor", "CONSTRUCTOR: Lock-free capture initialized, slotIndex="
                           + juce::String(slotIndex) + ", decimationRatio=" + juce::String(decimationRatio)
                           + ", trackId=" + trackIdentifier_);
    }
    else
    {
        OSCIL_LOG_ERROR("PluginProcessor::CONSTRUCTOR", "Slot allocation FAILED! Lock-free capture disabled.");
    }

    OSCIL_LOG_LIFECYCLE("PluginProcessor", "CONSTRUCTOR COMPLETE: trackId=" + trackIdentifier_);
    // Note: CaptureThread is already running (started by PluginFactory)
}

OscilPluginProcessor::~OscilPluginProcessor()
{
    // Mark lock-free path as not ready first (prevents audio thread from using it)
    lockFreeReady_.store(false, std::memory_order_release);

    // Release capture slot in pool (pool and thread are owned by PluginFactory)
    int slotIndex = captureSlotIndex_.load(std::memory_order_acquire);
    if (slotIndex != AudioCapturePool::INVALID_SLOT)
    {
        capturePool_.releaseSlot(slotIndex);
        captureSlotIndex_.store(AudioCapturePool::INVALID_SLOT, std::memory_order_release);
    }

    state_.getState().removeListener(this);

    // Unregister from MemoryBudgetManager
    memoryBudgetManager_.unregisterBuffer(trackIdentifier_);

    // Unregister from instance registry
    if (sourceId_.isValid())
    {
        instanceRegistry_.unregisterInstance(sourceId_);
    }
}

const juce::String OscilPluginProcessor::getName() const
{
    return "MultiScoper";
}

bool OscilPluginProcessor::acceptsMidi() const { return true; }
bool OscilPluginProcessor::producesMidi() const { return false; }
bool OscilPluginProcessor::isMidiEffect() const { return false; }

double OscilPluginProcessor::getTailLengthSeconds() const { return 0.0; }

int OscilPluginProcessor::getNumPrograms() { return 1; }
int OscilPluginProcessor::getCurrentProgram() { return 0; }
void OscilPluginProcessor::setCurrentProgram(int) {}
const juce::String OscilPluginProcessor::getProgramName(int) { return {}; }
void OscilPluginProcessor::changeProgramName(int, const juce::String&) {}

void OscilPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // H48 FIX: Reentrancy guard - some hosts call prepareToPlay reentrantly
    // Use exchange to atomically check and set the flag
    if (preparingToPlay_.exchange(true, std::memory_order_acq_rel))
    {
        // Already in prepareToPlay - skip reentrant call
        OSCIL_LOG_LIFECYCLE("PluginProcessor", "prepareToPlay SKIPPED (reentrant call): sampleRate="
                           + juce::String(sampleRate) + ", trackId=" + trackIdentifier_);
        return;
    }

    OSCIL_LOG_LIFECYCLE("PluginProcessor", "prepareToPlay CALLED: sampleRate=" + juce::String(sampleRate)
                       + ", samplesPerBlock=" + juce::String(samplesPerBlock)
                       + ", trackId=" + trackIdentifier_);

    currentSampleRate_.store(sampleRate, std::memory_order_release);
    currentBlockSize_.store(samplesPerBlock, std::memory_order_release);

    // Pre-calculate ticks to seconds conversion for real-time safe CPU timing
    ticksToSecondsScale_ = 1.0 / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());

    // TimingEngine setup is lock-free, safe to call from audio thread
    timingEngine_.setSampleRate(sampleRate);
    
    if (analysisEngine_)
    {
        analysisEngine_->prepare(sampleRate, samplesPerBlock);
        analysisEngine_->reset();
    }

    // Memory allocation and mutex-protected operations must happen on message thread
    // This is critical because prepareToPlay can be called from audio thread in some hosts
    // (e.g., Pro Tools, Reaper in certain configurations)
    // The DecimatingCaptureBuffer is already initialized with defaults in constructor,
    // so processBlock can safely write to it even before registration completes.
    //
    // IMPORTANT: Slot was already allocated synchronously in constructor.
    // Here we only UPDATE the config (sample rate, decimation ratio).
    //
    // Use WeakReference to handle case where processor is destroyed before callback runs.
    auto doRegistration = [weakThis = juce::WeakReference<OscilPluginProcessor>(this),
                           sampleRate,
                           captureConfig = getCaptureQualityConfig(), // Thread-safe read
                           trackId = trackIdentifier_,
                           channelCount = getTotalNumInputChannels()]() {
        auto* processor = weakThis.get();
        if (!processor) return;

        // Pre-initialize PerformanceProfiler on message thread before audio processing starts
        // This ensures the singleton is created here (with its allocations) rather than
        // lazily in processBlock which runs on audio thread.
        (void)PerformanceProfiler::getInstance();

        // Update MemoryBudgetManager with actual sample rate from host
        processor->memoryBudgetManager_.setGlobalConfig(captureConfig, static_cast<int>(sampleRate));

        // Configure capture buffer (allocates memory) - legacy path
        processor->captureBuffer_->configure(captureConfig, static_cast<int>(sampleRate));

        // Register buffer with MemoryBudgetManager for auto-quality adjustment
        processor->memoryBudgetManager_.registerBuffer(trackId, processor->captureBuffer_);

        // Update existing slot config (slot was allocated in constructor)
        int slotIndex = processor->captureSlotIndex_.load(std::memory_order_acquire);
        if (slotIndex != AudioCapturePool::INVALID_SLOT)
        {
            int decimationRatio = captureConfig.getDecimationRatio(static_cast<int>(sampleRate));
            processor->capturePool_.updateSlotConfig(slotIndex, sampleRate, decimationRatio);
        }

        // Register with instance registry (uses mutex, allocates)
        // Only register if not already registered (handles re-entrancy from host)
        if (!processor->sourceId_.isValid())
        {
            // Use the appropriate buffer based on capture mode
            std::shared_ptr<IAudioBuffer> bufferToRegister;
            bool useLockFree = processor->useLockFreeCapture_.load(std::memory_order_relaxed);

            OSCIL_LOG("REGISTRATION", "prepareToPlay: Registering NEW source, trackId=" + trackId
                     + ", useLockFreeCapture=" + juce::String(useLockFree ? "YES" : "NO")
                     + ", slotIndex=" + juce::String(slotIndex));

            if (useLockFree && slotIndex != AudioCapturePool::INVALID_SLOT)
            {
                bufferToRegister = processor->captureThread_.getProcessedBufferAsInterface(slotIndex);
                OSCIL_LOG("REGISTRATION", "prepareToPlay: Got lock-free buffer, valid="
                         + juce::String(bufferToRegister != nullptr ? "YES" : "NO"));
            }
            if (!bufferToRegister)
            {
                bufferToRegister = processor->captureBuffer_;
                OSCIL_LOG("REGISTRATION", "prepareToPlay: Using legacy DecimatingCaptureBuffer");
            }

            // Use track name from host if available, otherwise use a default
            juce::String sourceName = processor->trackName_.isNotEmpty()
                ? processor->trackName_
                : "Track " + juce::String(static_cast<int>(processor->instanceRegistry_.getSourceCount()) + 1);

            processor->sourceId_ = processor->instanceRegistry_.registerInstance(
                trackId,
                bufferToRegister,
                sourceName,
                channelCount,
                sampleRate,
                processor->analysisEngine_,
                processor->instanceUUID_);

            OSCIL_LOG("REGISTRATION", "prepareToPlay: REGISTERED with registry, sourceId="
                     + juce::String(processor->sourceId_.id)
                     + ", name=" + sourceName
                     + ", bufferValid=" + juce::String(bufferToRegister != nullptr ? "YES" : "NO"));
        }
        else
        {
            // Update existing registration with new sample rate
            juce::String sourceName = processor->trackName_.isNotEmpty()
                ? processor->trackName_
                : "Track";
            processor->instanceRegistry_.updateSource(processor->sourceId_, sourceName, channelCount, sampleRate);
            OSCIL_LOG("REGISTRATION", "prepareToPlay: UPDATED existing source, sourceId="
                     + juce::String(processor->sourceId_.id));
        }
    };

    // If already on message thread, execute directly; otherwise defer
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        doRegistration();
    }
    else
    {
        juce::MessageManager::callAsync(std::move(doRegistration));
    }

    // H48 FIX: Clear reentrancy guard at the end of synchronous work
    preparingToPlay_.store(false, std::memory_order_release);
}

void OscilPluginProcessor::releaseResources()
{
    // H49 FIX: Properly release all resources allocated during prepareToPlay
    // This is called when audio processing is about to stop
    // Note: We don't unregister from registry here - that happens in destructor
    // Only unregistering causes issues when host calls prepareToPlay again

    OSCIL_LOG_LIFECYCLE("PluginProcessor", "releaseResources CALLED: trackId=" + trackIdentifier_);

    // Mark lock-free path as temporarily not ready during resource release
    // This prevents the audio thread from accessing buffers while we're releasing
    bool wasLockFreeReady = lockFreeReady_.exchange(false, std::memory_order_acq_rel);

    // Reset analysis engine state (but keep the object for reuse)
    if (analysisEngine_)
    {
        analysisEngine_->reset();
    }

    // Clear capture buffer data (but keep the object for reuse)
    // The buffer will be reconfigured in the next prepareToPlay call
    if (captureBuffer_)
    {
        captureBuffer_->clear();
    }

    // Reset the raw buffer in the capture pool (if using lock-free path)
    int slotIndex = captureSlotIndex_.load(std::memory_order_acquire);
    if (slotIndex != AudioCapturePool::INVALID_SLOT)
    {
        RawCaptureBuffer* rawBuffer = capturePool_.getRawBuffer(slotIndex);
        if (rawBuffer != nullptr)
        {
            rawBuffer->reset();
        }
    }

    // Note: TimingEngine doesn't have a reset method - it uses atomics internally
    // and will be reconfigured on next prepareToPlay

    // Restore lock-free ready state if it was previously ready
    // The next prepareToPlay will properly reinitialize everything
    if (wasLockFreeReady)
    {
        lockFreeReady_.store(true, std::memory_order_release);
    }

    OSCIL_LOG_LIFECYCLE("PluginProcessor", "releaseResources COMPLETE: trackId=" + trackIdentifier_);
}

bool OscilPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // CRITICAL: Reject disabled buses - this prevents hosts from passing 0-channel buffers
    // Without this check, some hosts (FL Studio, Ableton) may disable audio routing entirely
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled() || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    // Support mono and stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output for effect plugins
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void OscilPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // Safety check
    if (numSamples == 0 || numChannels == 0)
        return;

    // Measure start time for CPU usage calculation
    auto startTime = juce::Time::getHighResolutionTicks();

    // Get host timing information
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto positionInfo = hostPlayHead->getPosition())
        {
            timingEngine_.updateHostInfo(*positionInfo);
        }
    }

    // ========== AUDIO CAPTURE ==========
    // Two paths: Lock-free (new) or Legacy (old)
    // 
    // DEFENSE-IN-DEPTH: Check atomic lockFreeReady_ flag (set only after slot fully configured)
    // This ensures we never access an invalid slot, even if processBlock is called
    // before prepareToPlay completes (possible in Pro Tools, Reaper)
    
    bool usedLockFreePath = false;
    bool useLockFree = lockFreeReady_.load(std::memory_order_acquire);

    if (useLockFree && useLockFreeCapture_.load(std::memory_order_relaxed))
    {
        // ===== LOCK-FREE PATH (~1μs per source) =====
        // Ultra-minimal: just memcpy to SPSC ring buffer
        // No decimation, no filtering - CaptureThread handles that
        int slotIndex = captureSlotIndex_.load(std::memory_order_relaxed);  // Already validated by lockFreeReady_
        RawCaptureBuffer* rawBuffer = capturePool_.getRawBuffer(slotIndex);
        
        if (rawBuffer != nullptr)
        {
            // Check for multi-producer (DAWs with multiple audio threads)
            // If detected, fall back to legacy path which uses SpinLock
            if (rawBuffer->wasMultiProducerDetected())
            {
                // Multi-threaded audio detected - disable lock-free for this instance
                // This is rare but must be handled for DAWs like Pro Tools HDX
                lockFreeReady_.store(false, std::memory_order_release);
                // Fall through to legacy path
            }
            else
            {
                const float* left = numChannels > 0 ? buffer.getReadPointer(0) : nullptr;
                const float* right = numChannels > 1 ? buffer.getReadPointer(1) : left;
                double sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
                rawBuffer->write(left, right, numSamples, sampleRate);
                usedLockFreePath = true;

                // Debug: Log lock-free write periodically (avoid logging in audio thread, use counter)
                // CRITICAL FIX (C5): Use atomic to prevent data race when multiple instances
                // access this counter from their audio threads
                static std::atomic<int> lockFreeWriteCounter{0};
                if (lockFreeWriteCounter.fetch_add(1, std::memory_order_relaxed) >= 10000) // Every ~10000 blocks
                {
                    lockFreeWriteCounter.store(0, std::memory_order_relaxed);
                    // Can't use DBG in audio thread safely, but this counter proves the path is taken
                }
            }
        }
        // If rawBuffer is null, fall through to legacy path
    }
    
    // Debug: Log first path usage (one-time, from message thread callback)
    static std::atomic<int> firstPathUsed{0}; // 0=none, 1=lock-free, 2=legacy
    if (usedLockFreePath && firstPathUsed.load(std::memory_order_relaxed) == 0)
    {
        firstPathUsed.store(1, std::memory_order_relaxed);
        // Schedule logging on message thread (safe)
        juce::MessageManager::callAsync([]() {
            DBG("[PluginProcessor::processBlock] First path used: LOCK-FREE");
        });
    }

    if (!usedLockFreePath)
    {
        // Debug: Log legacy path usage (one-time, from message thread callback)
        if (firstPathUsed.load(std::memory_order_relaxed) == 0)
        {
            firstPathUsed.store(2, std::memory_order_relaxed);
            juce::MessageManager::callAsync([]() {
                DBG("[PluginProcessor::processBlock] First path used: LEGACY");
            });
        }

        // ===== LEGACY PATH =====
        // Always available as fallback. Uses DecimatingCaptureBuffer which handles
        // all filtering and decimation internally.
        CaptureFrameMetadata metadata;
        metadata.sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
        metadata.numChannels = numChannels;
        metadata.numSamples = numSamples;
        metadata.isPlaying = timingEngine_.getHostInfo().isPlaying;
        metadata.bpm = timingEngine_.getHostInfo().bpm;
        metadata.timestamp = timingEngine_.getHostInfo().timeInSamples;

        // Capture audio for visualization BEFORE any modifications (read-only operation)
        // DecimatingCaptureBuffer handles downsampling internally
        captureBuffer_->write(buffer, metadata);
    }

    // Run analysis (Real-time stats) - always on audio thread
    if (analysisEngine_)
        analysisEngine_->process(buffer, currentSampleRate_.load(std::memory_order_relaxed));

    // Process triggers (MIDI and Audio)
    // Note: TimingEngine uses internal atomic flags to store trigger state
    // processMidi checks for Note On events if in MIDI mode
    // processBlock checks for audio threshold if in Audio mode
    (void) timingEngine_.processMidi(midiMessages);
    (void) timingEngine_.processBlock(buffer);

    // Audio passes through unchanged - this is a visualization plugin

    // Calculate CPU usage using pre-calculated conversion factor (real-time safe)
    // Guard against division by zero: some hosts may call processBlock before prepareToPlay
    double sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
    if (sampleRate <= 0.0)
        return;

    auto endTime = juce::Time::getHighResolutionTicks();
    double elapsedSeconds = static_cast<double>(endTime - startTime) * ticksToSecondsScale_;
    double availableSeconds = static_cast<double>(numSamples) / sampleRate;
    float usage = static_cast<float>(elapsedSeconds / availableSeconds * 100.0);

    // Smooth CPU usage with exponential moving average
    float currentUsage = cpuUsage_.load(std::memory_order_relaxed);
    cpuUsage_.store(currentUsage * 0.9f + usage * 0.1f, std::memory_order_relaxed);

    // Record audio thread execution time for profiling
    double executionMs = elapsedSeconds * 1000.0;
    PerformanceProfiler::getInstance().getThreadProfiler().recordAudioThreadExecution(executionMs);
}

bool OscilPluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* OscilPluginProcessor::createEditor()
{
    // Use unique_ptr for exception safety - if constructor throws, memory is cleaned up
    auto editor = std::make_unique<OscilPluginEditor>(*this);
    // JUCE API requires raw pointer (framework takes ownership and deletes the editor)
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return editor.release();
}

void OscilPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL FIX (C6): Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We use try_lock to avoid priority inversion.
    // The SpinLock's ScopedTryLockType attempts exactly once without spinning.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Audio thread path: return cached UTF-8 bytes using non-blocking try_lock
        // If lock is held by message thread, we return empty data - host will retry
        const juce::SpinLock::ScopedTryLockType sl(cachedStateLock_);
        if (sl.isLocked() && !cachedStateUtf8_.empty())
        {
            // NOTE: destData.replaceAll() should not allocate if destData was pre-sized
            // by the host. If it does allocate, this is unavoidable but acceptable
            // since it's a single allocation per save operation (not per audio block).
            destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
        }
        // If lock failed or cache empty, return empty - host will retry
        return;
    }

    // Message thread path: perform full serialization (allocates memory)
    updateCachedState();

    // Safe to use blocking lock on message thread
    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
}

void OscilPluginProcessor::updateCachedState()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Sync timing engine state to OscilState before saving
    auto timingState = timingEngine_.toValueTree();
    auto& stateTree = state_.getState();

    // Persist instanceUUID for cross-session source reconnection
    stateTree.setProperty(StateIds::InstanceUUID, instanceUUID_, nullptr);

    // Remove existing Timing node if present
    auto existingTiming = stateTree.getChildWithName(StateIds::Timing);
    if (existingTiming.isValid())
        stateTree.removeChild(existingTiming, nullptr);

    // Add current timing state
    stateTree.appendChild(timingState, nullptr);

    auto xmlString = state_.toXmlString();

    // Convert to UTF-8 bytes now (on message thread) to avoid any
    // potential allocation when audio thread reads the cached state.
    const char* utf8Ptr = xmlString.toRawUTF8();
    size_t utf8Size = xmlString.getNumBytesAsUTF8();

    // Update cache with lock
    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    cachedStateUtf8_.assign(utf8Ptr, utf8Ptr + utf8Size);
}

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // CRITICAL FIX (C4): Check thread FIRST, before ANY allocations.
    // Some DAW hosts (Pro Tools, Reaper) may call setStateInformation from
    // the audio thread during session load or preset recall. We must defer ALL
    // operations to the message thread because:
    // 1. juce::String creation allocates memory (not real-time safe)
    // 2. fromXmlString() allocates memory (not real-time safe)
    // 3. state_ may be accessed by UI thread concurrently (data race)
    // 4. ValueTree operations are not thread-safe

    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // AUDIO THREAD PATH: Defer everything to message thread using pre-allocated buffer
        // NO ALLOCATIONS - styleguide constitutional rule #1
        //
        // Guard conditions:
        // 1. Data must fit in pre-allocated buffer
        // 2. No pending state already waiting (atomic check)
        // If either fails, we drop the state - host will typically retry
        if (sizeInBytes > 0 &&
            static_cast<size_t>(sizeInBytes) <= MAX_STATE_SIZE &&
            pendingStateSize_.load(std::memory_order_acquire) == 0)
        {
            // Copy to pre-allocated buffer (memcpy is real-time safe)
            std::memcpy(pendingStateBuffer_.data(), data, static_cast<size_t>(sizeInBytes));
            pendingStateSize_.store(sizeInBytes, std::memory_order_release);

            // Schedule processing on message thread
            juce::MessageManager::callAsync(
                [weakThis = juce::WeakReference<OscilPluginProcessor>(this)]() {
                    if (auto* processor = weakThis.get())
                    {
                        // Atomically consume the pending state
                        int size = processor->pendingStateSize_.exchange(0, std::memory_order_acq_rel);
                        if (size > 0)
                        {
                            processor->setStateInformation(processor->pendingStateBuffer_.data(), size);
                        }
                    }
                });
        }
        // If buffer too large or pending state exists, drop this call - host will retry
        return;
    }

    // MESSAGE THREAD PATH: Safe to allocate and do full processing
    auto xmlString = juce::String::createStringFromData(data, sizeInBytes);
    auto sampleRate = static_cast<int>(currentSampleRate_);

    // Extract and restore instanceUUID synchronously
    // This ensures prepareToPlay's registration uses the correct UUID.
    {
        auto uuidStart = xmlString.indexOf("instanceUUID=\"");
        if (uuidStart >= 0)
        {
            uuidStart += 14; // Length of 'instanceUUID="'
            auto uuidEnd = xmlString.indexOf(uuidStart, "\"");
            if (uuidEnd > uuidStart)
            {
                auto restoredUUID = xmlString.substring(uuidStart, uuidEnd);
                if (restoredUUID.isNotEmpty())
                {
                    instanceUUID_ = restoredUUID;
                    OSCIL_LOG("PluginProcessor", "setStateInformation: Restored instanceUUID: " + restoredUUID);
                }
            }
        }
    }

    // Parse and apply state (safely on message thread)
    if (!state_.fromXmlString(xmlString))
    {
        juce::Logger::writeToLog("PluginProcessor::setStateInformation - Failed to parse state XML");
        OSCIL_LOG_ERROR("PluginProcessor", "Failed to parse state XML - state restoration aborted");
        return;
    }

    // Restore persisted instanceUUID if available (for cross-session source reconnection)
    auto restoredUUID = state_.getState().getProperty(StateIds::InstanceUUID, "").toString();
    if (restoredUUID.isNotEmpty())
    {
        instanceUUID_ = restoredUUID;
        OSCIL_LOG("PluginProcessor", "Restored instanceUUID: " + restoredUUID);
    }

    // Apply restored timing state (TimingEngine uses atomics internally)
    auto timingTree = state_.getState().getChildWithName(StateIds::Timing);
    if (timingTree.isValid())
        timingEngine_.fromValueTree(timingTree);

    // Sync restored capture quality config to MemoryBudgetManager
    auto captureConfig = state_.getCaptureQualityConfig();
    setCaptureQualityConfig(captureConfig);
    memoryBudgetManager_.setGlobalConfig(captureConfig, sampleRate);

    // Update capture buffer config (allocates memory)
    captureBuffer_->configure(captureConfig, sampleRate);

    // Update cached state for real-time safe getStateInformation()
    updateCachedState();
}

void OscilPluginProcessor::updateTrackProperties(const TrackProperties& properties)
{
    // Called by host when track properties change (including track name)
    // This should be called on the message thread by most hosts
    // properties.name is std::optional<juce::String>
    if (properties.name.has_value() && properties.name->isNotEmpty())
    {
        trackName_ = *properties.name;

        // If we're already registered, update the source name in the registry
        if (sourceId_.isValid())
        {
            instanceRegistry_.updateSource(sourceId_, trackName_,
                static_cast<int>(getMainBusNumInputChannels()),
                currentSampleRate_.load(std::memory_order_relaxed));
        }
    }
}

std::shared_ptr<SharedCaptureBuffer> OscilPluginProcessor::getCaptureBuffer() const
{
    // Debug: Log which path is being used (one-time per processor)
    static std::atomic<bool> loggedPath{false};

    bool lockFreeReady = lockFreeReady_.load(std::memory_order_acquire);
    bool useLockFree = useLockFreeCapture_.load(std::memory_order_relaxed);

    // Use lock-free path if enabled and ready
    if (lockFreeReady && useLockFree)
    {
        int slotIndex = captureSlotIndex_.load(std::memory_order_relaxed);
        auto buffer = captureThread_.getProcessedBuffer(slotIndex);
        if (buffer)
        {
            if (!loggedPath.exchange(true))
            {
                OSCIL_LOG_BUFFER("PluginProcessor::getCaptureBuffer", juce::String(sourceId_.id),
                                true, true, "LOCK-FREE path, slotIndex=" + juce::String(slotIndex)
                                + ", availableSamples=" + juce::String(static_cast<int>(buffer->getAvailableSamples())));
            }
            return buffer;
        }
        else if (!loggedPath.exchange(true))
        {
            OSCIL_LOG_ERROR("PluginProcessor::getCaptureBuffer",
                           "Lock-free buffer is NULL for slotIndex=" + juce::String(slotIndex));
        }
    }
    else if (!loggedPath.exchange(true))
    {
        OSCIL_LOG_BUFFER("PluginProcessor::getCaptureBuffer", juce::String(sourceId_.id),
                        true, true, "LEGACY path, lockFreeReady=" + juce::String(lockFreeReady ? "YES" : "NO")
                        + ", useLockFree=" + juce::String(useLockFree ? "YES" : "NO"));
    }
    // Fallback to legacy buffer
    return captureBuffer_->getInternalBuffer();
}

std::shared_ptr<DecimatingCaptureBuffer> OscilPluginProcessor::getDecimatingCaptureBuffer() const
{
    return captureBuffer_;
}

SourceId OscilPluginProcessor::getSourceId() const
{
    return sourceId_;
}

OscilState& OscilPluginProcessor::getState()
{
    return state_;
}

int OscilPluginProcessor::getCaptureRate() const
{
    if (captureBuffer_)
        return captureBuffer_->getCaptureRate();
    // Fall back to standard capture rate if buffer not yet configured
    return CaptureRate::STANDARD;
}

TimingEngine& OscilPluginProcessor::getTimingEngine()
{
    return timingEngine_;
}

IInstanceRegistry& OscilPluginProcessor::getInstanceRegistry()
{
    return instanceRegistry_;
}

IThemeService& OscilPluginProcessor::getThemeService()
{
    return themeService_;
}

ShaderRegistry& OscilPluginProcessor::getShaderRegistry()
{
    return shaderRegistry_;
}

std::shared_ptr<IAudioBuffer> OscilPluginProcessor::getBuffer(const SourceId& sourceId)
{
    // If the requested source is this processor's own source, return local buffer
    if (sourceId == sourceId_)
    {
        // Use lock-free path if enabled and ready
        if (lockFreeReady_.load(std::memory_order_acquire) && useLockFreeCapture_.load(std::memory_order_relaxed))
        {
            int slotIndex = captureSlotIndex_.load(std::memory_order_relaxed);
            auto buffer = captureThread_.getProcessedBufferAsInterface(slotIndex);
            if (buffer)
                return buffer;
        }
        // Fallback to legacy buffer
        return captureBuffer_;
    }

    // Otherwise, look up in registry
    return instanceRegistry_.getCaptureBuffer(sourceId);
}

void OscilPluginProcessor::setUseLockFreeCapture(bool enable)
{
    useLockFreeCapture_.store(enable, std::memory_order_relaxed);
}

void OscilPluginProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    // Check if CaptureQuality settings changed
    if (tree.hasType(StateIds::CaptureQuality) ||
        (tree.getParent().isValid() && tree.getParent().hasType(StateIds::CaptureQuality)))
    {
        // CRITICAL: Always defer to message thread to guarantee real-time safety
        // ValueTree listeners should be called on message thread, but we verify defensively.
        // The state read MUST happen on message thread to avoid race conditions.
        // Use WeakReference to handle case where processor is destroyed before callback runs.
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscilPluginProcessor>(this)]() {
            if (auto* processor = weakThis.get())
            {
                // Read state only on message thread (thread-safe)
                auto captureConfig = processor->state_.getCaptureQualityConfig();
                processor->setCaptureQualityConfig(captureConfig);

                auto sampleRate = static_cast<int>(processor->currentSampleRate_.load(std::memory_order_relaxed));
                processor->captureBuffer_->configure(captureConfig, sampleRate);
            }
        });
    }
}

CaptureQualityConfig OscilPluginProcessor::getCaptureQualityConfig() const
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    return cachedCaptureConfig_;
}

void OscilPluginProcessor::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    cachedCaptureConfig_ = config;
}

} // namespace oscil

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return oscil::PluginFactory::getInstance().createPluginProcessor().release();
}
