# Oscil Code Review Fix Plan

**Generated:** 2025-12-28
**Total Issues:** ~285
**Review Scope:** Full codebase review by 12 specialized agents

---

## Executive Summary

This plan addresses all issues found during the comprehensive code review, organized into 4 phases by severity. Each phase must be completed and verified before proceeding to the next.

| Phase | Severity | Issues | Focus |
|-------|----------|--------|-------|
| 1 | CRITICAL | 25 | Compilation blockers, crashes, data corruption |
| 2 | HIGH | 58 | Thread safety, resource leaks, silent failures |
| 3 | MEDIUM | 82 | Edge cases, suboptimal patterns, consistency |
| 4 | LOW | 42 | Code quality, documentation, optimization |

---

# Phase 1: CRITICAL Issues (25 issues)

**Goal:** Make the codebase compile and run without crashes.
**Verification:** `cmake --build --preset dev` succeeds, unit tests pass, plugin loads in DAW without crash.

---

## 1.1 Compilation Blockers (3 issues)

### C1: Duplicate Function Definitions (ODR Violation)
**Files:**
- `src/rendering/VisualConfiguration.cpp`
- `src/rendering/serialization/VisualConfigurationSerialization.cpp`

**Problem:** Both files define `toValueTree()` and `fromValueTree()` methods for `VisualConfiguration`. This violates One Definition Rule and prevents linking.

**Fix:**
```cpp
// Option A: Remove from VisualConfiguration.cpp, keep in serialization file
// Option B: Make one set of methods call the other
// Option C: Use different namespaces/static functions
```

**Action:**
1. Read both files to understand which implementation is correct
2. Remove duplicate definitions from one file
3. Ensure all callers still compile

---

### C2: DropdownPopup Uses Deleted SpringAnimation Class
**File:** `src/ui/components/dropdown/DropdownPopup.cpp`

**Problem:** References `SpringAnimation` class that has been deleted from codebase.

**Fix:**
```cpp
// Option A: Replace with OscilAnimationService
// Option B: Use simple linear animation
// Option C: Remove animation entirely
```

**Action:**
1. Read DropdownPopup.cpp to understand animation usage
2. Replace SpringAnimation with OscilAnimationService or remove
3. Update header if needed

---

### C3: DropdownPopup Missing Timer Inheritance
**File:** `src/ui/components/dropdown/DropdownPopup.cpp`

**Problem:** Calls `startTimer()`/`stopTimer()` but class doesn't inherit `juce::Timer`.

**Fix:**
```cpp
// In header:
class DropdownPopup : public juce::Component, private juce::Timer
{
    void timerCallback() override;
    // ...
};
```

**Action:**
1. Add Timer inheritance to class declaration
2. Implement timerCallback() method
3. Or remove timer usage if replaced by animation service

---

## 1.2 Audio Thread Safety Violations (5 issues)

### C4: Memory Allocation Before Thread Check
**File:** `src/plugin/PluginProcessor.cpp`
**Lines:** 492-493

**Problem:** `setStateInformation()` creates `juce::String` (allocation) before checking if on message thread.

**Fix:**
```cpp
void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Thread check FIRST, before any allocations
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Store raw data and defer
        pendingStateData_.store(/* raw copy */);
        triggerAsyncUpdate();
        return;
    }

    // Now safe to allocate
    auto xmlString = juce::String::fromUTF8(...);
    // ...
}
```

---

### C5: Static Variables Shared Between Instances
**File:** `src/plugin/PluginProcessor.cpp`
**Lines:** 333-353

**Problem:** Static variables accessed from multiple plugin instances without synchronization.

**Fix:**
```cpp
// Replace static with instance member or use std::atomic
// If truly global, use proper singleton with mutex
```

**Action:**
1. Identify which statics are problematic
2. Convert to instance members or atomic
3. Add thread-safe accessor if needed

---

### C6: SpinLock in getStateInformation
**File:** `src/plugin/PluginProcessor.cpp`
**Lines:** 208-212

**Problem:** SpinLock can cause priority inversion when audio thread calls this.

**Fix:**
```cpp
// Option A: Use lock-free snapshot
// Option B: Use try_lock with fallback
// Option C: Pre-serialize state on message thread
```

---

### C7: SharedCaptureBuffer::clear() No Synchronization
**File:** `src/core/SharedCaptureBuffer.cpp`
**Lines:** 278-283

**Problem:** `clear()` can race with concurrent `write()`/`read()` operations.

**Fix:**
```cpp
void SharedCaptureBuffer::clear()
{
    // Use atomic fence or SeqLock to synchronize
    writeIndex_.store(0, std::memory_order_release);
    readIndex_.store(0, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
```

---

### C8: jassert for Null Pointer Checks
**File:** `src/core/SharedCaptureBuffer.cpp`
**Lines:** 64-66

**Problem:** `jassert` disabled in release, allows null dereference.

**Fix:**
```cpp
// Add runtime guard
if (samples == nullptr || numSamples < 0 || numChannels < 0)
    return;  // Defensive check for release builds
jassert(samples != nullptr);  // Keep for debug
```

---

## 1.3 OpenGL Thread Safety & Resource Leaks (7 issues)

### C9: SpinLock During GPU Operations
**File:** `src/rendering/RenderEngine.cpp`
**Lines:** 251-318

**Problem:** SpinLock held during GPU draw calls can block 100ms+ on GPU stall.

**Fix:**
```cpp
// Option A: Copy data under lock, render outside lock
// Option B: Use double-buffering with swap
// Option C: Use lock-free queue for render commands
```

---

### C10: BatchEntry Stores Dangling Raw Pointers
**File:** `src/rendering/RenderEngine.cpp`
**Lines:** 407-432

**Problem:** Raw pointers to oscillators become dangling when oscillator deleted.

**Fix:**
```cpp
// Use weak references or IDs instead of raw pointers
struct BatchEntry {
    juce::WeakReference<Oscillator> oscillator;  // or use UUID
    // ...
};
```

---

### C11: FBO Resize Without Synchronization
**File:** `src/rendering/RenderEngine.cpp`
**Lines:** 560+

**Problem:** `resize()` modifies FBOs while render thread may be using them.

**Fix:**
```cpp
// Use pending resize flag, apply resize at frame start
void RenderEngine::resize(int w, int h)
{
    pendingWidth_.store(w);
    pendingHeight_.store(h);
    resizePending_.store(true);
}

void RenderEngine::beginFrame()
{
    if (resizePending_.exchange(false))
        applyPendingResize();
}
```

---

### C12: clearAllWaveforms From Wrong Thread
**File:** `src/rendering/WaveformGLRenderer.cpp`
**Lines:** 827-838

**Problem:** UI thread clears render data that render thread is using.

**Fix:**
```cpp
// Use atomic flag or defer to render thread
void WaveformGLRenderer::clearAllWaveforms()
{
    clearRequested_.store(true, std::memory_order_release);
    // Actual clear happens in render thread
}
```

---

### C13: GPU Resource Leak in Reinitialization
**File:** `src/rendering/WaveformGLRenderer.cpp`
**Lines:** 676-680

**Problem:** Context deleted but GL objects not released first.

**Fix:**
```cpp
void WaveformGLRenderer::attemptReinitialization()
{
    // Release GL resources BEFORE context destruction
    if (context_ && context_->isActive())
    {
        releaseAllGLResources();
    }
    // Then reinitialize
}
```

---

### C14: GradientFillShader VAO/VBO Leak + Double-Free
**File:** `src/rendering/shaders/GradientFillShader.cpp`
**Lines:** 83-88

**Problem:**
1. Early return on `!compiled` leaves VAO/VBO leaked if compile failed partway
2. Handles not reset to 0 after deletion

**Fix:**
```cpp
void GradientFillShader::release(juce::OpenGLContext& context)
{
    if (gl_->vbo != 0) {
        context.extensions.glDeleteBuffers(1, &gl_->vbo);
        gl_->vbo = 0;
    }
    if (gl_->vao != 0) {
        context.extensions.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
    }
    gl_->program.reset();
    gl_->compiled = false;
}
```

---

### C15: NeonGlowShader Same VAO/VBO Issue
**File:** `src/rendering/shaders/NeonGlowShader.cpp`
**Lines:** 86-93

**Fix:** Same pattern as C14.

---

## 1.4 Multi-Instance Race Conditions (3 issues)

### C16: UUID Collision Overwrites Without Check
**File:** `src/core/InstanceRegistry.cpp`
**Line:** 95

**Problem:** Duplicate UUID silently replaces existing source.

**Fix:**
```cpp
bool InstanceRegistry::registerSource(Source* source)
{
    auto uuid = source->getUUID();
    std::lock_guard lock(mutex_);

    if (sources_.find(uuid) != sources_.end())
    {
        juce::Logger::writeToLog("InstanceRegistry: UUID collision for " + uuid);
        return false;  // Reject duplicate
    }

    sources_[uuid] = source;
    return true;
}
```

---

### C17: Source Serialization Without Lock
**File:** `src/core/Source.cpp`
**Lines:** 70, 97-111

**Problem:** `toValueTree()`/`fromValueTree()` access `backupInstanceIds_` without mutex.

**Fix:**
```cpp
juce::ValueTree Source::toValueTree() const
{
    std::lock_guard lock(mutex_);
    // ... serialize backupInstanceIds_
}
```

---

### C18: Raw Pointer to Source Can Dangle
**File:** `src/core/Oscillator.cpp`

**Problem:** Oscillator holds raw `Source*` that can dangle when source unregisters.

**Fix:**
```cpp
// Use weak reference or UUID lookup
class Oscillator {
    juce::WeakReference<Source> source_;
    // or
    juce::Uuid sourceUuid_;

    Source* getSource() {
        return source_.get();  // Returns nullptr if deleted
    }
};
```

---

## 1.5 State Corruption (2 issues)

### C19: OscilState Not Thread-Safe
**File:** `src/core/OscilState.cpp`

**Problem:** `getState()` returns non-const reference allowing unsynchronized modification.

**Fix:**
```cpp
// Option A: Return copy instead of reference
OscilState::State OscilState::getStateCopy() const;

// Option B: Add mutex and return locked accessor
// Option C: Document thread ownership requirements
```

---

### C20: State Restoration Failure Silent in Release
**File:** `src/plugin/PluginProcessor.cpp`
**Line:** 525

**Problem:** Only DBG logging for state parse failure.

**Fix:**
```cpp
if (!processor->state_.fromXmlString(*xmlString))
{
    juce::Logger::writeToLog("PluginProcessor: Failed to restore state");
    stateLoadFailed_.store(true);
    return;
}
```

---

## 1.6 Shader & Effect Failures (3 issues)

### C21: InstancedShader UBO Never Bound
**File:** `src/rendering/shaders/InstancedShader.cpp`
**Lines:** 121-131

**Problem:** UBO created but never bound to uniform block - shader non-functional.

**Fix:**
```cpp
// Option A: Complete UBO binding with direct GL calls
// Option B: Remove InstancedShader, use alternative approach
// Option C: Mark as experimental/disabled
```

---

### C22: Out-of-Bounds Array Access in Shader
**File:** `resources/shaders/waveform_instanced.vert`
**Lines:** 44-47

**Problem:** `gl_InstanceID >= 64` reads garbage from UBO.

**Fix:**
```glsl
if (gl_InstanceID >= MAX_INSTANCES) {
    gl_Position = vec4(0.0);
    return;
}
```

---

### C23: catch(...) Swallows All Exceptions
**File:** `src/rendering/WaveformGLRenderer.cpp`
**Lines:** 419-426

**Problem:** Memory allocation failure completely hidden.

**Fix:**
```cpp
try {
    captureBuffer_.resize(requiredSize);
} catch (const std::exception& e) {
    juce::Logger::writeToLog("Frame capture failed: " + juce::String(e.what()));
    return;
} catch (...) {
    juce::Logger::writeToLog("Frame capture failed: unknown exception");
    return;
}
```

---

## 1.7 Resource Lifecycle (2 issues)

### C24: MemoryBudgetManager USE-AFTER-FREE
**File:** `src/core/MemoryBudgetManager.cpp`
**Lines:** 418, 435

**Problem:** `[this]` in callAsync without safety.

**Fix:**
```cpp
juce::MessageManager::callAsync([weak = juce::WeakReference<MemoryBudgetManager>(this)]() {
    if (auto* self = weak.get())
    {
        auto snapshot = self->getMemorySnapshot();
        self->listeners_.call([&](Listener& l) { l.memoryUsageChanged(snapshot); });
    }
});
```

---

### C25: OscilAlertModal Dead Code
**File:** `src/ui/components/OscilAlertModal.cpp`

**Problem:** Calls non-existent method, not in Sources.cmake.

**Fix:**
```cpp
// Option A: Add to Sources.cmake and implement setContentOwned
// Option B: Remove file entirely if obsolete
```

---

## Phase 1 Verification Checklist

- [ ] All source files compile without errors
- [ ] `cmake --build --preset dev` succeeds
- [ ] `ctest --preset dev` passes all tests
- [ ] Plugin loads in standalone without crash
- [ ] Plugin loads in DAW (Bitwig/Logic) without crash
- [ ] Create/delete 10 oscillators rapidly - no crash
- [ ] Resize window while audio playing - no crash
- [ ] Save/load preset - works correctly

---

# Phase 2: HIGH Priority Issues (58 issues)

**Goal:** Eliminate thread safety bugs, resource leaks, and silent failures.
**Verification:** Stress tests pass, no memory leaks in profiler, all error conditions logged.

---

## 2.1 Thread Safety (15 issues)

| ID | File | Line | Issue | Fix |
|----|------|------|-------|-----|
| H1 | PluginProcessor.cpp | 120-125 | `bufferSize_`/`sampleRate_` no sync | Use atomics |
| H2 | InstanceRegistry.cpp | - | Async notification race | Add sequence number |
| H3 | SharedCaptureBuffer.cpp | - | SPSC overtake, wrong memory order | Fix acquire/release |
| H4 | RenderEngine.cpp | - | Batch pointer invalidation | Use weak refs |
| H5 | DecimatingCaptureBuffer.cpp | - | jassert thread checks | Add runtime checks |
| H6 | PluginProcessor.cpp | - | prepareToPlay without cleanup | Add cleanup call |
| H7 | RenderEngine.cpp | - | Context access from multiple threads | Add context ownership |
| H8 | Source.cpp | - | Listener notification during mutation | Copy listeners first |
| H9 | Oscillator.cpp | - | State access during render | Use SeqLock |
| H10 | WaveformRenderState.cpp | - | Dirty flag race | Use atomic |
| H11 | EffectPipeline.cpp | - | Effect enable/disable race | Use pending changes |
| H12 | FramebufferPool.cpp | - | Pool access from threads | Add mutex |
| H13 | ShaderRegistry.cpp | - | Shader lookup during compile | Use rwlock |
| H14 | TimingEngine.cpp | - | BPM update race | Use atomic |
| H15 | VisualConfiguration.cpp | - | Config update race | Use copy-on-write |

---

## 2.2 OpenGL Resource Management (12 issues)

| ID | File | Issue | Fix |
|----|------|-------|-----|
| H16 | TrailsEffect.cpp:126-127 | Same texture on multiple units | Use separate texture |
| H17 | ScanlineEffect.cpp:112-117 | Missing uniform validation | Add location checks |
| H18 | RadialBlurEffect.cpp:87-91 | Missing uniform validation | Add location checks |
| H19 | Framebuffer.cpp | DBG-only error logging | Use Logger |
| H20 | RenderEngine.cpp:1002-1012 | Silent black screen | Log and fallback |
| H21 | GradientFillShader.cpp | No renderSoftware() | Implement fallback |
| H22 | NeonGlowShader.cpp | No renderSoftware() | Implement fallback |
| H23 | BloomEffect.cpp | Mip chain bounds | Add bounds check |
| H24 | All effects | Blend state not restored | Save/restore blend |
| H25 | DistortionEffect.cpp | Uniform not validated | Add validation |
| H26 | ChromaticAberrationEffect.cpp | Same issue | Add validation |
| H27 | VignetteEffect.cpp | Same issue | Add validation |

---

## 2.3 UI Component Safety (10 issues)

| ID | File | Issue | Fix |
|----|------|-------|-----|
| H28 | OscilCheckbox.cpp | Animation lambda captures this | Use SafePointer |
| H29 | OscilToggle.cpp | Same | Use SafePointer |
| H30 | OscilSlider.cpp | Same | Use SafePointer |
| H31 | OscilAccordion.cpp | Same | Use SafePointer |
| H32 | OscilRadioButton.cpp | Same | Use SafePointer |
| H33 | PresetCard.cpp | Missing listener cleanup | Add removeListener |
| H34 | OscilColorPicker.cpp | Timer not guarded | Use SafePointer |
| H35 | OscilTabs.cpp | Animation safety | Use SafePointer |
| H36 | OscilTextField.cpp | Focus callback safety | Use SafePointer |
| H37 | OscilModal.cpp | Content ownership unclear | Clarify ownership |

---

## 2.4 Error Handling (8 issues)

| ID | File | Issue | Fix |
|----|------|-------|-----|
| H38 | FramebufferPool.cpp:24-28 | jassertfalse only | Add Logger |
| H39 | VisualPresetManager.cpp:69-74 | DBG only | Use Logger |
| H40 | OscilState.cpp:70-99 | No error details | Return error string |
| H41 | GlobalPreferences.cpp | Save failure not propagated | Return bool |
| H42 | FramebufferPool.cpp:260-276 | Continue after FBO fail | Set error flag |
| H43 | PostProcessEffect.cpp | Shader compile DBG only | Use Logger |
| H44 | WaveformShader.cpp | Same | Use Logger |
| H45 | GridRenderer.cpp | Same | Use Logger |

---

## 2.5 Plugin Lifecycle (5 issues)

| ID | File | Issue | Fix |
|----|------|-------|-----|
| H46 | PluginFactory.cpp | Static destruction order | Use weak refs |
| H47 | PluginEditor.cpp | Animation service leak | Clear on destroy |
| H48 | PluginProcessor.cpp | prepareToPlay reentrancy | Add guard |
| H49 | PluginProcessor.cpp | releaseResources incomplete | Release all |
| H50 | PluginEditor.cpp | Component attachment order | Fix order |

---

## 2.6 Additional HIGH Issues (8 issues)

| ID | File | Issue | Fix |
|----|------|-------|-----|
| H51 | PluginEditor_Adapters.h:287 | Hardcoded debug path | Remove |
| H52 | waveform_ssbo.vert:17 | instanceIndex never set | Implement or remove |
| H53 | Multiple shaders | Intel GPU compatibility | Test and fix |
| H54 | waveform_instanced.vert | UBO alignment | Verify std140 |
| H55 | BasicShader.cpp | Projection matrix mismatch | Align formulas |
| H56 | ThemeManager.cpp | File write DBG only | Use Logger |
| H57 | RenderBootstrapper.cpp | Shader error DBG only | Use Logger |
| H58 | TextureManager.cpp | Texture load DBG only | Use Logger |

---

## Phase 2 Verification Checklist

- [ ] Run with ThreadSanitizer - no data races reported
- [ ] Run with AddressSanitizer - no memory errors
- [ ] Stress test: 100 oscillators, resize rapidly, no crash
- [ ] GPU profiler shows no resource leaks
- [ ] All errors appear in Logger output
- [ ] Intel GPU test pass (if available)
- [ ] Software rendering fallback works

---

# Phase 3: MEDIUM Priority Issues (82 issues)

**Goal:** Fix edge cases, improve consistency, handle error conditions gracefully.
**Verification:** Edge case tests pass, UI behaves correctly in all scenarios.

---

## 3.1 State Management (20 issues)

- Serialization edge cases (empty strings, max values)
- Validation gaps (out-of-range values accepted)
- Preset save/load edge cases
- Undo/redo state consistency
- Multi-instance state isolation

## 3.2 Effects Pipeline (15 issues)

- Effect parameter validation
- Effect ordering edge cases
- Blend mode restoration
- Texture unit cleanup
- HDR value clamping

## 3.3 UI Components (15 issues)

- Layout edge cases (zero size, extreme aspect ratios)
- Focus handling consistency
- Keyboard navigation gaps
- Tooltip edge cases
- Accessibility improvements

## 3.4 Animation System (8 issues)

- Animation timing precision
- Easing function edge cases
- Interrupt handling
- Queue management
- Memory during long animations

## 3.5 Test Harness (15 issues)

- HTTP error responses
- Request validation
- Timeout handling
- Connection cleanup
- Response formatting

## 3.6 Shaders (9 issues)

- Unused attribute declarations
- Precision qualifiers
- HDR overflow handling
- sRGB conversion accuracy
- Platform-specific behavior

---

# Phase 4: LOW Priority Issues (42 issues)

**Goal:** Code quality, documentation, performance optimization.
**Verification:** Code review approval, documentation complete.

---

## 4.1 Code Quality (20 issues)

- Remove dead code
- Consistent naming conventions
- Reduce code duplication
- Improve const-correctness
- Add missing noexcept

## 4.2 Documentation (12 issues)

- Add missing header comments
- Document thread ownership
- Document error handling
- Update architecture docs
- Add example usage

## 4.3 Performance (10 issues)

- Reduce unnecessary allocations
- Cache frequently computed values
- Optimize hot paths
- Reduce lock contention
- Improve cache locality

---

# Appendix A: File Reference

## Most Affected Files (by issue count)

| File | Critical | High | Medium | Total |
|------|----------|------|--------|-------|
| PluginProcessor.cpp | 4 | 5 | 3 | 12 |
| RenderEngine.cpp | 4 | 4 | 2 | 10 |
| WaveformGLRenderer.cpp | 3 | 3 | 2 | 8 |
| SharedCaptureBuffer.cpp | 2 | 3 | 2 | 7 |
| InstanceRegistry.cpp | 2 | 2 | 2 | 6 |
| Framebuffer.cpp | 1 | 3 | 2 | 6 |
| FramebufferPool.cpp | 1 | 3 | 1 | 5 |

---

# Appendix B: Testing Strategy

## Unit Tests to Add

```cpp
// Thread safety tests
TEST(ThreadSafety, ConcurrentStateAccess)
TEST(ThreadSafety, ConcurrentSourceRegistration)
TEST(ThreadSafety, ConcurrentRenderResize)

// Resource lifecycle tests
TEST(Resources, ShaderLeakOnFailure)
TEST(Resources, FBOLeakOnResize)
TEST(Resources, ListenerCleanup)

// Error handling tests
TEST(ErrorHandling, CorruptStateRecovery)
TEST(ErrorHandling, GPUResourceExhaustion)
TEST(ErrorHandling, InvalidPresetData)
```

## Stress Tests

```bash
# Run with sanitizers
cmake --preset dev -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --preset dev -DCMAKE_CXX_FLAGS="-fsanitize=address"

# Profile for leaks
leaks --atExit -- ./build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4
```

---

# Appendix C: Rollback Plan

If a phase introduces regressions:

1. Revert to last stable commit
2. Identify specific change causing regression
3. Apply fix in isolation
4. Re-run verification
5. Continue phase

---

# Appendix D: Progress Tracking

## Phase 1 Progress
- [ ] C1: Duplicate function definitions
- [ ] C2: DropdownPopup SpringAnimation
- [ ] C3: DropdownPopup Timer inheritance
- [ ] C4: Memory allocation before thread check
- [ ] C5: Static variables shared
- [ ] C6: SpinLock in getStateInformation
- [ ] C7: SharedCaptureBuffer::clear() sync
- [ ] C8: jassert null pointer checks
- [ ] C9: SpinLock during GPU ops
- [ ] C10: BatchEntry dangling pointers
- [ ] C11: FBO resize sync
- [ ] C12: clearAllWaveforms thread
- [ ] C13: GPU resource leak reinit
- [ ] C14: GradientFillShader VAO/VBO
- [ ] C15: NeonGlowShader VAO/VBO
- [ ] C16: UUID collision
- [ ] C17: Source serialization lock
- [ ] C18: Raw pointer to Source
- [ ] C19: OscilState thread safety
- [ ] C20: State restoration logging
- [ ] C21: InstancedShader UBO
- [ ] C22: Shader array bounds
- [ ] C23: catch(...) exception
- [ ] C24: MemoryBudgetManager UAF
- [ ] C25: OscilAlertModal dead code

## Phase 2 Progress
- [ ] H1-H15: Thread Safety (0/15)
- [ ] H16-H27: OpenGL Resources (0/12)
- [ ] H28-H37: UI Components (0/10)
- [ ] H38-H45: Error Handling (0/8)
- [ ] H46-H50: Plugin Lifecycle (0/5)
- [ ] H51-H58: Additional (0/8)

## Phase 3 Progress
- [ ] State Management (0/20)
- [ ] Effects Pipeline (0/15)
- [ ] UI Components (0/15)
- [ ] Animation System (0/8)
- [ ] Test Harness (0/15)
- [ ] Shaders (0/9)

## Phase 4 Progress
- [ ] Code Quality (0/20)
- [ ] Documentation (0/12)
- [ ] Performance (0/10)
