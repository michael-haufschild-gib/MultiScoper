/*
    Oscil - Render Engine Implementation
*/

#include "rendering/RenderEngine.h"

#include "plugin/HostCompatibility.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/effects/PostProcessEffect.h"
#include "tools/PerformanceProfiler.h"

#include <cmath>
#include <iostream>

namespace oscil
{

using namespace juce::gl;

RenderEngine::RenderEngine()
    : bootstrapper_(std::make_unique<RenderBootstrapper>())
    , effectPipeline_(std::make_unique<EffectPipeline>())
    , waveformPass_(std::make_unique<WaveformPass>())
{
    // Pre-allocate batch storage to avoid runtime allocations
    batchEntries_.reserve(kMaxBatchSize);
    batchShaderOrder_.reserve(16);  // Number of shader types
}

RenderEngine::~RenderEngine()
{
    // Attempt cleanup if context is still valid
    if (initialized_ && context_ && context_->isActive())
    {
        DBG("[RenderEngine] WARNING: shutdown() not called, attempting cleanup");
        shutdown();
    }
    else if (initialized_)
    {
        // Context gone, can't clean up - log for debugging
        std::cerr << "[RenderEngine] LEAK: GPU resources leaked (context unavailable)" << std::endl;
    }
}

bool RenderEngine::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        shutdown();

    context_ = &context;

    // Get initial dimensions from target component
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
    {
        RE_LOG("RenderEngine: No target component");
        return false;
    }

    // Use physical (scaled) dimensions for internal state and FBOs
    double scale = context.getRenderingScale();
    currentWidth_ = static_cast<int>(targetComponent->getWidth() * scale);
    currentHeight_ = static_cast<int>(targetComponent->getHeight() * scale);

    if (currentWidth_ <= 0 || currentHeight_ <= 0)
    {
        RE_LOG("RenderEngine: Invalid dimensions " << currentWidth_ << "x" << currentHeight_);
        return false;
    }

    // Initialize subsystems
    if (!bootstrapper_->initialize(context))
    {
        RE_LOG("RenderEngine: Failed to initialize bootstrapper");
        return false;
    }

    if (!effectPipeline_->initialize(context, currentWidth_, currentHeight_))
    {
        RE_LOG("RenderEngine: Failed to initialize effect pipeline");
        bootstrapper_->shutdown(context);
        return false;
    }

    if (!waveformPass_->initialize(context, currentWidth_, currentHeight_))
    {
        RE_LOG("RenderEngine: Failed to initialize waveform pass");
        effectPipeline_->shutdown(context);
        bootstrapper_->shutdown(context);
        return false;
    }

    initialized_ = true;

    // Detect GPU capabilities
    gpuCapabilities_ = GPUCapabilities::detect(context);
    int maxGPUTier = gpuCapabilities_.getMaxTier();

    // Detect host compatibility and set initial rendering tier
    HostCapabilityLevel hostLevel = HostCompatibility::detectHost();
    RenderTier initialTier = RenderTier::Baseline;  // Start conservative
    
    // Determine max tier based on both GPU capabilities and host compatibility
    int maxHostTier = 4;  // Default to full capability
    switch (hostLevel)
    {
        case HostCapabilityLevel::Full:
            maxHostTier = 4;
            break;
        case HostCapabilityLevel::NoCompute:
            maxHostTier = 3;  // SSBO max
            break;
        case HostCapabilityLevel::NoMetal:
            maxHostTier = 3;  // No Metal, but OpenGL features OK
            break;
        case HostCapabilityLevel::SafeMode:
            maxHostTier = 1;  // Baseline only
            break;
    }

    // Select the minimum of GPU capability and host compatibility
    int effectiveMaxTier = std::min(maxGPUTier, maxHostTier);
    
    // Start at baseline but allow upgrade through user settings later
    // For now, auto-select the best safe tier (but not Compute - that's opt-in)
    if (effectiveMaxTier >= 3 && hostLevel != HostCapabilityLevel::SafeMode)
    {
        initialTier = RenderTier::SSBO;
    }
    else if (effectiveMaxTier >= 2 && hostLevel != HostCapabilityLevel::SafeMode)
    {
        initialTier = RenderTier::Instanced;
    }
    else
    {
        initialTier = RenderTier::Baseline;
    }
    
    errorStats_.currentTier.store(initialTier, std::memory_order_release);
    
    RE_LOG("RenderEngine: Initialized " << currentWidth_ << "x" << currentHeight_);
    RE_LOG("  GPU: " << gpuCapabilities_.glRenderer.toStdString() 
           << " (GL " << gpuCapabilities_.glMajorVersion << "." << gpuCapabilities_.glMinorVersion << ")");
    RE_LOG("  GPU max tier: " << maxGPUTier << ", Host max tier: " << maxHostTier);
    RE_LOG("  Host: " << hostCapabilityLevelToString(hostLevel));
    RE_LOG("  Selected tier: " << static_cast<int>(initialTier));
    
    return true;
}

void RenderEngine::shutdown()
{
    if (!initialized_ || !context_)
        return;

    // Release all waveform states
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
        waveformStates_.clear();
    }

    // Shutdown subsystems
    waveformPass_->shutdown(*context_);
    effectPipeline_->shutdown(*context_);
    bootstrapper_->shutdown(*context_);

    context_ = nullptr;
    initialized_ = false;

    RE_LOG("RenderEngine: Shutdown complete");
}

void RenderEngine::resize(int width, int height)
{
    if (!initialized_ || width <= 0 || height <= 0)
        return;

    // C11 FIX: Store pending dimensions atomically instead of modifying FBOs directly.
    // The actual resize happens on the render thread at the start of the next frame
    // to avoid race conditions with renderWaveform() accessing FBOs.
    pendingWidth_.store(width, std::memory_order_relaxed);
    pendingHeight_.store(height, std::memory_order_relaxed);
    resizePending_.store(true, std::memory_order_release);

    RE_LOG("RenderEngine: Resize requested to " << width << "x" << height);
}

void RenderEngine::applyPendingResize()
{
    // Called at the start of each frame on the render thread
    if (!resizePending_.load(std::memory_order_acquire))
        return;

    int width = pendingWidth_.load(std::memory_order_relaxed);
    int height = pendingHeight_.load(std::memory_order_relaxed);
    resizePending_.store(false, std::memory_order_release);

    // Skip if dimensions haven't actually changed
    if (width == currentWidth_ && height == currentHeight_)
        return;

    if (!context_ || width <= 0 || height <= 0)
        return;

    currentWidth_ = width;
    currentHeight_ = height;

    // Resize subsystems (safe now - we're on the render thread)
    effectPipeline_->resize(*context_, width, height);

    // Resize history FBOs for waveforms with trails
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        for (auto& [key, state] : waveformStates_)
        {
            if (state.trailsEnabled)
            {
                state.resizeHistoryFBO(*context_, width, height);
            }
        }
    }

    RE_LOG("RenderEngine: Applied resize to " << width << "x" << height);
}

void RenderEngine::beginFrame(float deltaTime)
{
    if (!initialized_)
        return;

    // C11 FIX: Apply any pending resize BEFORE rendering starts
    // This ensures FBOs are properly sized before any draw calls
    applyPendingResize();

    // Start frame profiling
    frameStartTime_ = std::chrono::high_resolution_clock::now();
    PerformanceProfiler::getInstance().getGpuProfiler().beginFrame();

    // Track GL errors from previous frame and check for fallback
    errorStats_.beginFrame();
    if (errorStats_.shouldTriggerFallback() && !errorStats_.fallbackTriggered.load(std::memory_order_acquire))
    {
        triggerFallback();
    }

    // Initialize frame budget tracking
    frameBudgetGuard_.beginFrame(budgetConfig_);

    stats_.update(deltaTime);

    RE_LOG_THROTTLED(60, "beginFrame: dt=" << deltaTime << ", tier=" << static_cast<int>(getCurrentTier()));

    // Update camera animation
    if (auto* cam = waveformPass_->getCamera())
        cam->update(deltaTime);

    // Clear scene FBO
    if (auto* sceneFBO = effectPipeline_->getSceneFBO())
    {
        sceneFBO->bind();
        sceneFBO->clear(backgroundColour_, false);
        sceneFBO->unbind();
    }
}

void RenderEngine::renderWaveform(const WaveformRenderData& data)
{
    if (!initialized_ || !data.visible || data.channel1.size() < 2)
        return;

    // C9 FIX: Copy config data under lock, then render outside lock
    // This prevents GPU stalls from blocking other threads
    // NOTE: WaveformRenderState can't be copied (contains unique_ptr), so we copy just config
    VisualConfiguration configCopy;
    int waveformId = data.id;
    bool stateFound = false;

    {
        // Short lock scope - only for map access and copy
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);

        auto it = waveformStates_.find(data.id);
        if (it == waveformStates_.end())
        {
            // Register inline to avoid recursive lock
            // Use emplace to avoid copy/move issues with std::atomic members
            auto [newIt, inserted] = waveformStates_.emplace(data.id, WaveformRenderState{});
            if (inserted)
            {
                newIt->second.waveformId = data.id;
                RE_LOG("RenderEngine: Registered waveform " << data.id);
            }
            it = newIt;
        }

        if (it != waveformStates_.end())
        {
            // Copy just the config (copyable)
            configCopy = it->second.visualConfig;
            stateFound = true;
        }
    }
    // Lock released here - GPU operations happen outside the lock

    if (!stateFound)
        return;

    const auto& config = configCopy;

    // Step 0: Prepare (Timing, Camera, Viewport)
    // This sets up the camera and viewport for the waveform geometry.
    // Need to access state for updateTiming
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        auto it = waveformStates_.find(waveformId);
        if (it != waveformStates_.end())
        {
            waveformPass_->prepareRender(data, it->second, stats_.getDeltaTime());
        }
    }

    // Step 1: Render Grid Layer (Background) -> Scene FBO
    // We render the grid directly to the scene FBO so it is NOT affected by waveform post-processing.
    if (data.gridConfig.enabled)
    {
        auto* sceneFBO = effectPipeline_->getSceneFBO();
        if (sceneFBO && sceneFBO->isValid())
        {
            sceneFBO->bind();
            // Note: renderGrid handles its own viewport setting internally
            waveformPass_->renderGrid(data);
            sceneFBO->unbind();
        }
    }

    // Step 2: Render Waveform Geometry -> Scratch FBO
    auto* pool = effectPipeline_->getFramebufferPool();
    auto* waveformFBO = pool ? pool->getWaveformFBO() : nullptr;
    if (waveformFBO)
    {
        waveformFBO->bind();
        waveformFBO->clear(juce::Colours::transparentBlack, true);

        // Render Geometry (uses viewport set by prepareRender)
        waveformPass_->renderWaveformGeometry(data, config, stats_.getTime());

        waveformFBO->unbind();

        // Step 3: Apply Post-Processing Effects -> Processed FBO
        Framebuffer* processedFBO = waveformFBO;
        if (config.hasPostProcessing())
        {
            // Need state reference for historyFBO access
            juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
            auto it = waveformStates_.find(waveformId);
            if (it != waveformStates_.end())
            {
                processedFBO = effectPipeline_->applyPostProcessing(processedFBO, it->second, *context_,
                                                                    stats_.getDeltaTime(),
                                                                    bootstrapper_->getCompositeShader(),
                                                                    bootstrapper_->getCompositeTextureLoc());
                if (!processedFBO)
                {
                    processedFBO = waveformFBO;
                }
            }
        }

        // Step 4: Composite IMMEDIATELY (Blend Waveform over Grid/Scene)
        // This is critical because waveformFBO is reused for the next waveform
        executeComposite(processedFBO, config);
    }
}

void RenderEngine::endFrame()
{
    if (!initialized_)
        return;

    // Apply global effects to scene FBO - CONDITIONALLY based on budget
    if (isGlobalPostProcessingEnabled() && !frameBudgetGuard_.isEmergency())
    {
        if (auto* sceneFBO = effectPipeline_->getSceneFBO())
            effectPipeline_->applyGlobalEffects(sceneFBO, *context_);
    }
    else if (isGlobalPostProcessingEnabled() && frameBudgetGuard_.isEmergency())
    {
        frameBudgetGuard_.recordSkippedEffect();
        RE_LOG_THROTTLED(60, "Budget emergency - skipping global effects");
    }

    // CRITICAL: Always blit to screen - must produce output
    blitToScreen();

    // Finalize budget tracking
    frameBudgetGuard_.endFrame();

    // Log budget overruns (throttled to avoid log spam)
    auto budgetStats = frameBudgetGuard_.getFrameStats();
    if (budgetStats.budgetExceeded)
    {
        RE_LOG_THROTTLED(60, "Frame budget exceeded: " << budgetStats.elapsedMs 
                         << "ms, skipped " << budgetStats.skippedWaveforms << " waveforms");
    }

    // Record budget stats to profiler
    auto& gpuProfiler = PerformanceProfiler::getInstance().getGpuProfiler();
    gpuProfiler.recordBudgetStats(
        budgetStats.elapsedMs,
        budgetConfig_.targetFrameMs,
        budgetStats.skippedWaveforms,
        budgetStats.skippedEffects
    );

    // End frame profiling
    gpuProfiler.endFrame();
}

// ============================================================================
// Batched Rendering Implementation
// ============================================================================

void RenderEngine::beginBatch()
{
    if (!initialized_)
        return;

    // Clear previous batch data (no deallocation due to reserve)
    batchEntries_.clear();
    for (auto& [shaderType, group] : batchGroups_)
    {
        group.clear();
    }
    batchShaderOrder_.clear();

    // Reset vertex buffer pool for new batch
    if (effectPipeline_)
    {
        if (auto* pool = effectPipeline_->getFramebufferPool())
        {
            if (auto* vbPool = pool->getVertexBufferPool())
            {
                vbPool->reset();
            }
        }
    }

    batchActive_ = true;
}

void RenderEngine::addToBatch(const WaveformRenderData& data)
{
    if (!initialized_ || !batchActive_)
        return;

    // Skip invisible or empty waveforms
    if (!data.visible || data.channel1.size() < 2)
        return;

    // C10 FIX: Copy only the copyable parts of state under lock
    VisualConfiguration configCopy;
    float lastFrameTime = 0.0f;
    float accumulatedTime = 0.0f;
    bool stateFound = false;

    {
        // Short lock scope - only for map access and copy
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);

        auto it = waveformStates_.find(data.id);
        if (it == waveformStates_.end())
        {
            // Register new waveform state
            // Use emplace to avoid copy/move issues with std::atomic members
            auto [newIt, inserted] = waveformStates_.emplace(data.id, WaveformRenderState{});
            if (inserted)
            {
                newIt->second.waveformId = data.id;
                RE_LOG("RenderEngine: Registered waveform " << data.id << " (batch)");
            }
            it = newIt;
        }

        if (it != waveformStates_.end())
        {
            // Copy the copyable parts of state
            configCopy = it->second.visualConfig;
            lastFrameTime = it->second.lastFrameTime;
            accumulatedTime = it->second.accumulatedTime;
            stateFound = true;
        }
    }
    // Lock released here

    if (!stateFound)
        return;

    // Create batch entry with copied config
    BatchEntry entry;
    entry.waveformId = data.id;
    entry.data = &data;
    entry.visualConfig = std::move(configCopy);
    entry.lastFrameTime = lastFrameTime;
    entry.accumulatedTime = accumulatedTime;
    entry.shaderType = entry.visualConfig.shaderType;
    entry.hasPostProcessing = entry.visualConfig.hasPostProcessing();
    entry.originalIndex = batchEntries_.size();  // Track insertion order

    batchEntries_.push_back(std::move(entry));
}

void RenderEngine::flushBatch()
{
    if (!initialized_ || !batchActive_ || batchEntries_.empty())
    {
        batchActive_ = false;
        return;
    }

    // Guard against GL errors during batch rendering
    GLErrorGuard guard(errorStats_, "flushBatch");

    // Step 1: Sort entries by shader type for efficient batching
    sortBatchByShaderType();

    // Step 2: Reset shader cache for this batch
    waveformPass_->resetShaderCache();

    // Step 3: Render all grids in a single sceneFBO bind
    renderBatchedGrids();

    // Step 4: Bind vertex buffer pool for batched geometry
    VertexBufferPool* vbPool = nullptr;
    if (effectPipeline_)
    {
        if (auto* pool = effectPipeline_->getFramebufferPool())
        {
            vbPool = pool->getVertexBufferPool();
            if (vbPool && vbPool->isInitialized())
            {
                vbPool->bind();  // Single bind for entire batch
            }
        }
    }

    // Step 5: Set pool on WaveformPass for shader use
    waveformPass_->setVertexBufferPool(vbPool);

    // Step 6: Render waveforms grouped by shader type
    // Use instanced rendering if available (Tier 2+) and group is large enough
    RenderTier currentTier = getCurrentTier();
    
    for (ShaderType shaderType : batchShaderOrder_)
    {
        // BUDGET CHECK: Skip remaining shader groups in emergency
        if (frameBudgetGuard_.isEmergency())
        {
            frameBudgetGuard_.recordSkippedShaderGroup();
            RE_LOG_THROTTLED(60, "Budget emergency - skipping shader group");
            continue;
        }

        auto groupIt = batchGroups_.find(shaderType);
        if (groupIt != batchGroups_.end() && !groupIt->second.entries.empty())
        {
            // Use instanced rendering for Tier 2+ if group has multiple entries
            // and none have post-processing (PP requires individual FBO clears)
            bool canUseInstanced = (currentTier >= RenderTier::Instanced) &&
                                   (groupIt->second.entries.size() >= 2) &&
                                   gpuCapabilities_.hasInstancing;
            
            // Check if any entries have post-processing (disqualifies instancing)
            if (canUseInstanced)
            {
                for (const auto& entry : groupIt->second.entries)
                {
                    if (entry.hasPostProcessing)
                    {
                        canUseInstanced = false;
                        break;
                    }
                }
            }
            
            if (canUseInstanced)
            {
                renderBatchGroupInstanced(groupIt->second);
            }
            else
            {
                renderBatchGroup(groupIt->second);
            }
        }
    }

    // Step 7: Cleanup - unbind pool and clear from WaveformPass
    waveformPass_->setVertexBufferPool(nullptr);
    if (vbPool && vbPool->isInitialized())
    {
        vbPool->unbind();
    }

    batchActive_ = false;
}

void RenderEngine::sortBatchByShaderType()
{
    // Clear groups
    for (auto& [shaderType, group] : batchGroups_)
    {
        group.clear();
    }
    batchShaderOrder_.clear();

    // Group entries by shader type
    for (const auto& entry : batchEntries_)
    {
        auto& group = batchGroups_[entry.shaderType];
        if (group.entries.empty())
        {
            group.shaderType = entry.shaderType;
            batchShaderOrder_.push_back(entry.shaderType);
        }
        group.entries.push_back(entry);
    }

    // Sort shader order for deterministic rendering (optional, but helps debugging)
    std::sort(batchShaderOrder_.begin(), batchShaderOrder_.end(),
              [](ShaderType a, ShaderType b) {
                  return static_cast<int>(a) < static_cast<int>(b);
              });
}

void RenderEngine::renderBatchedGrids()
{
    auto* sceneFBO = effectPipeline_->getSceneFBO();
    if (!sceneFBO || !sceneFBO->isValid())
        return;

    // Single FBO bind for all grids
    sceneFBO->bind();

    for (auto& entry : batchEntries_)
    {
        if (entry.data->gridConfig.enabled)
        {
            // Prepare camera/viewport for this waveform's grid
            // C10 FIX: Look up state for prepareRender (needs to call updateTiming)
            {
                juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
                auto it = waveformStates_.find(entry.waveformId);
                if (it != waveformStates_.end())
                {
                    waveformPass_->prepareRender(*entry.data, it->second, stats_.getDeltaTime());
                }
            }
            // Note: renderGrid doesn't need state, just data - lock released
            waveformPass_->renderGrid(*entry.data);
        }
    }

    sceneFBO->unbind();
}

void RenderEngine::renderBatchGroup(BatchGroup& group)
{
    if (group.entries.empty())
        return;

    auto* pool = effectPipeline_->getFramebufferPool();
    auto* waveformFBO = pool ? pool->getWaveformFBO() : nullptr;
    if (!waveformFBO)
        return;

    // Separate entries into those with and without post-processing
    // Entries without PP and with same blend mode can be batched together
    // Reuse member vectors to avoid per-batch allocation
    noPPEntries_.clear();
    withPPEntries_.clear();

    for (const auto& entry : group.entries)
    {
        if (entry.hasPostProcessing)
        {
            withPPEntries_.push_back(&entry);
        }
        else
        {
            noPPEntries_.push_back(&entry);
        }
    }

    // Batch render waveforms WITHOUT post-processing
    // These can all render into the same FBO clear, then composite once per blend mode
    if (!noPPEntries_.empty())
    {
        // Group by blend mode for batch compositing
        // Clear and reuse member map to avoid per-batch allocation
        for (auto& [_, vec] : blendGroups_)
            vec.clear();

        for (const auto* entry : noPPEntries_)
        {
            // C10 FIX: Use copied visualConfig
            BlendMode mode = entry->visualConfig.compositeBlendMode;
            blendGroups_[mode].push_back(entry);
        }

        for (auto& [blendMode, entries] : blendGroups_)
        {
            // Sort by original index to preserve correct layering order within group
            std::sort(entries.begin(), entries.end(), [](const BatchEntry* a, const BatchEntry* b) {
                return a->originalIndex < b->originalIndex;
            });

            // Single FBO bind for all waveforms in this blend mode group
            waveformFBO->bind();
            waveformFBO->clear(juce::Colours::transparentBlack, true);

            // Render all waveform geometries into the same FBO
            for (const auto* entry : entries)
            {
                // C10 FIX: Use copied visualConfig, look up state for prepareRender
                if (entry->data)
                {
                    // Look up state for prepareRender (needs to update timing)
                    {
                        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
                        auto it = waveformStates_.find(entry->waveformId);
                        if (it != waveformStates_.end())
                        {
                            waveformPass_->prepareRender(*entry->data, it->second, stats_.getDeltaTime());
                        }
                    }
                    // GPU operation outside lock
                    waveformPass_->renderWaveformGeometry(*entry->data, entry->visualConfig, stats_.getTime());
                }
            }

            waveformFBO->unbind();

            // Single composite for the entire group (uses first entry's config for blend mode)
            if (!entries.empty())
            {
                executeComposite(waveformFBO, entries[0]->visualConfig);
            }
        }
    }

    // Sort by original index to preserve correct layering order for PP waveforms
    std::sort(withPPEntries_.begin(), withPPEntries_.end(), [](const BatchEntry* a, const BatchEntry* b) {
        return a->originalIndex < b->originalIndex;
    });

    // Render waveforms WITH post-processing individually (they need separate FBO clears)
    for (const auto* entry : withPPEntries_)
    {
        // BUDGET CHECK: Skip expensive waveforms (with PP) when critical
        if (frameBudgetGuard_.isCritical())
        {
            frameBudgetGuard_.recordSkippedWaveform();
            continue;
        }

        renderSingleWaveform(*entry);
    }
}

void RenderEngine::renderBatchGroupInstanced(BatchGroup& group)
{
    // Tier 2: Instanced rendering path
    // INFRASTRUCTURE NOTE: Full instanced rendering requires UBO binding functions
    // (glBindBufferBase/glBindBufferRange) which are not exposed by JUCE's 
    // OpenGLExtensionFunctions. The shader and UBO infrastructure is in place,
    // but until direct GL function loading is added, this falls back to regular
    // batched rendering which still provides good performance.
    //
    // Current behavior: Same as renderBatchGroup but with logging for debugging.
    // Future: Single instanced draw call with gl_InstanceID indexing.
    
    if (group.entries.empty())
        return;

    GLErrorGuard guard(errorStats_, "renderBatchGroupInstanced");

    auto* pool = effectPipeline_->getFramebufferPool();
    auto* waveformFBO = pool ? pool->getWaveformFBO() : nullptr;
    if (!waveformFBO)
    {
        renderBatchGroup(group);  // Fallback
        return;
    }

    // For now, instanced rendering is a placeholder that falls back to regular batching
    // Full implementation requires:
    // 1. Uploading all waveform vertex data to a single VBO
    // 2. Setting up the instanced shader with UBO data
    // 3. Using glDrawArraysInstanced
    //
    // This is complex because each waveform has different sample counts and vertex data.
    // A proper implementation would need to:
    // - Use a maximum sample count per waveform
    // - Pad shorter waveforms with zeros
    // - Store all vertices in a mega-buffer
    
    RE_LOG_THROTTLED(300, "RenderEngine: Instanced path (Tier 2) - " << group.entries.size() << " waveforms");
    
    // TODO: Implement full instanced rendering
    // For now, use the existing batched rendering which is still efficient
    // The infrastructure (shaders, UBO, capability detection) is in place
    
    // Fallback to non-instanced batched rendering
    // This still benefits from single FBO bind per blend mode group

    // Group by blend mode for efficient compositing
    // Clear and reuse member map to avoid per-batch allocation
    for (auto& [_, vec] : blendGroups_)
        vec.clear();

    for (const auto& entry : group.entries)
    {
        // C10 FIX: Use copied visualConfig
        BlendMode mode = entry.visualConfig.compositeBlendMode;
        blendGroups_[mode].push_back(&entry);
    }

    for (auto& [blendMode, entries] : blendGroups_)
    {
        // Sort by original index to preserve correct layering order within group
        std::sort(entries.begin(), entries.end(), [](const BatchEntry* a, const BatchEntry* b) {
            return a->originalIndex < b->originalIndex;
        });

        // Single FBO bind for all waveforms in this blend mode group
        waveformFBO->bind();
        waveformFBO->clear(juce::Colours::transparentBlack, true);

        // Render all waveform geometries into the same FBO
        for (const auto* entry : entries)
        {
            // C10 FIX: Use copied visualConfig, look up state for prepareRender
            if (entry->data)
            {
                // Look up state for prepareRender (needs to update timing)
                {
                    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
                    auto it = waveformStates_.find(entry->waveformId);
                    if (it != waveformStates_.end())
                    {
                        waveformPass_->prepareRender(*entry->data, it->second, stats_.getDeltaTime());
                    }
                }
                // GPU operation outside lock
                waveformPass_->renderWaveformGeometry(*entry->data, entry->visualConfig, stats_.getTime());
            }
        }

        waveformFBO->unbind();

        // Single composite for the entire group
        if (!entries.empty())
        {
            executeComposite(waveformFBO, entries[0]->visualConfig);
        }
    }
}

void RenderEngine::renderSingleWaveform(const BatchEntry& entry)
{
    // C10 FIX: Use copied visualConfig, look up state for operations that need it
    if (!entry.data)
        return;

    const WaveformRenderData& data = *entry.data;
    const auto& config = entry.visualConfig;

    // Prepare (Timing, Camera, Viewport) - needs state reference for updateTiming
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        auto it = waveformStates_.find(entry.waveformId);
        if (it != waveformStates_.end())
        {
            waveformPass_->prepareRender(data, it->second, stats_.getDeltaTime());
        }
    }

    // Render Waveform Geometry -> Scratch FBO (GPU operations outside lock)
    auto* pool = effectPipeline_->getFramebufferPool();
    auto* waveformFBO = pool ? pool->getWaveformFBO() : nullptr;
    if (waveformFBO)
    {
        waveformFBO->bind();
        waveformFBO->clear(juce::Colours::transparentBlack, true);

        // Render Geometry
        waveformPass_->renderWaveformGeometry(data, config, stats_.getTime());

        waveformFBO->unbind();

        // Apply Post-Processing Effects - needs state reference for historyFBO
        Framebuffer* processedFBO = waveformFBO;
        if (config.hasPostProcessing())
        {
            juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
            auto it = waveformStates_.find(entry.waveformId);
            if (it != waveformStates_.end())
            {
                processedFBO = effectPipeline_->applyPostProcessing(
                    processedFBO, it->second, *context_,
                    stats_.getDeltaTime(),
                    bootstrapper_->getCompositeShader(),
                    bootstrapper_->getCompositeTextureLoc());
                if (!processedFBO)
                {
                    processedFBO = waveformFBO;
                }
            }
        }

        // Composite to scene
        executeComposite(processedFBO, config);
    }
}

void RenderEngine::executeComposite(Framebuffer* source, const VisualConfiguration& config)
{
    if (!source || !source->isValid())
        return;

    auto* sceneFBO = effectPipeline_->getSceneFBO();
    if (!sceneFBO)
        return;

    sceneFBO->bind();

    // Restore full viewport for compositing to the scene FBO
    // This is necessary because renderWaveformGeometry sets the viewport to the pane size
    if (currentWidth_ > 0 && currentHeight_ > 0)
    {
        glViewport(0, 0, currentWidth_, currentHeight_);
    }

    glEnable(GL_BLEND);

    auto* shader = bootstrapper_->getCompositeShader();
    if (shader)
    {
        shader->use();
        context_->extensions.glUniform1i(bootstrapper_->getCompositeTextureLoc(), 0);

        // Set blend mode based on config
        switch (config.compositeBlendMode)
        {
            case BlendMode::Alpha:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Additive:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::Multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            case BlendMode::Screen:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
        }

        source->bindTexture(0);
        effectPipeline_->getFramebufferPool()->renderFullscreenQuad();
    }

    glDisable(GL_BLEND);
    sceneFBO->unbind();
}

void RenderEngine::registerWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    if (waveformStates_.find(waveformId) == waveformStates_.end())
    {
        // Use emplace to avoid copy/move issues with std::atomic members
        auto [it, inserted] = waveformStates_.emplace(waveformId, WaveformRenderState{});
        if (inserted)
        {
            it->second.waveformId = waveformId;
            RE_LOG("RenderEngine: Registered waveform " << waveformId);
        }
    }
}

void RenderEngine::unregisterWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        if (context_)
            it->second.release(*context_);
        waveformStates_.erase(it);
        RE_LOG("RenderEngine: Unregistered waveform " << waveformId);
    }
}

std::optional<VisualConfiguration> RenderEngine::getWaveformConfig(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
        return it->second.visualConfig;
    return std::nullopt;
}

bool RenderEngine::hasWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    return waveformStates_.find(waveformId) != waveformStates_.end();
}

void RenderEngine::setWaveformConfig(int waveformId, const VisualConfiguration& config)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        it->second.visualConfig = config;

        // Update trails state based on config
        if (context_)
        {
            if (config.trails.enabled && !it->second.trailsEnabled)
            {
                it->second.enableTrails(*context_, currentWidth_, currentHeight_);
            }
            else if (!config.trails.enabled && it->second.trailsEnabled)
            {
                it->second.disableTrails(*context_);
            }
        }
    }
}

void RenderEngine::clearAllWaveforms()
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    if (context_)
    {
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
    }
    waveformStates_.clear();
}

void RenderEngine::syncWaveforms(const std::unordered_set<int>& activeIds)
{
    if (!context_)
        return;

    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);

    std::vector<int> toRemove;
    for (const auto& pair : waveformStates_)
    {
        if (activeIds.find(pair.first) == activeIds.end())
        {
            toRemove.push_back(pair.first);
        }
    }

    for (int id : toRemove)
    {
        // Inline unregisterWaveform to avoid recursive lock
        auto it = waveformStates_.find(id);
        if (it != waveformStates_.end())
        {
            it->second.release(*context_);
            waveformStates_.erase(it);
            RE_LOG("RenderEngine: Synced out stale waveform " << id);
        }
    }
}

void RenderEngine::setQualityLevel(QualityLevel level)
{
    qualityLevel_ = level;
    effectPipeline_->setQualityLevel(level);
    RE_LOG("RenderEngine: Quality level set to " << static_cast<int>(level));
}

PostProcessEffect* RenderEngine::getEffect(const juce::String& effectId)
{
    return effectPipeline_->getEffect(effectId);
}

void RenderEngine::setGlobalPostProcessingEnabled(bool enabled)
{
    effectPipeline_->setGlobalPostProcessingEnabled(enabled);
}

bool RenderEngine::isGlobalPostProcessingEnabled() const
{
    return effectPipeline_->isGlobalPostProcessingEnabled();
}

void RenderEngine::blitToScreen()
{
    // Check if context is still active/valid
    if (!context_ || !context_->isActive())
        return;

    // Guard against GL errors during final blit - this is critical
    GLErrorGuard guard(errorStats_, "blitToScreen", true /* critical */);

    // Bind default framebuffer
    context_->extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent)
        return;

    float desktopScale = static_cast<float>(context_->getRenderingScale());
    auto width = static_cast<GLsizei>(static_cast<float>(targetComponent->getWidth()) * desktopScale);
    auto height = static_cast<GLsizei>(static_cast<float>(targetComponent->getHeight()) * desktopScale);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    auto* shader = bootstrapper_->getBlitShader();
    auto* sceneFBO = effectPipeline_->getSceneFBO();

    if (shader && sceneFBO && sceneFBO->isValid())
    {
        shader->use();
        sceneFBO->bindTexture(0);
        context_->extensions.glUniform1i(bootstrapper_->getBlitTextureLoc(), 0);

        effectPipeline_->getFramebufferPool()->renderFullscreenQuad();
    }
    else
    {
        // H20 FIX: Log render failure and provide visual fallback
        // Instead of silent black screen, clear to a dark red color to indicate error
        static bool errorLogged = false;
        if (!errorLogged)
        {
            juce::String errorMsg = "RenderEngine::blitToScreen() failed: ";
            if (!shader)
                errorMsg += "blit shader is null; ";
            if (!sceneFBO)
                errorMsg += "scene FBO is null; ";
            else if (!sceneFBO->isValid())
                errorMsg += "scene FBO is invalid; ";

            juce::Logger::writeToLog(errorMsg);
            RE_LOG(errorMsg.toStdString());
            errorLogged = true;  // Only log once to avoid spam
        }

        // Clear to dark red as visual error indicator (not black)
        // This makes it obvious something is wrong rather than silent failure
        glClearColor(0.15f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Restore GL state to prevent leakage into subsequent operations
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// Frame Budget Management
// ============================================================================

void RenderEngine::setFrameBudgetMs(double targetMs)
{
    budgetConfig_.targetFrameMs = targetMs;
}

FrameBudgetGuard::FrameStats RenderEngine::getLastFrameBudgetStats() const
{
    return frameBudgetGuard_.getFrameStats();
}

// ============================================================================
// GL Error Tracking & Fallback System
// ============================================================================

void RenderEngine::triggerFallback()
{
    RenderTier currentTier = errorStats_.currentTier.load(std::memory_order_acquire);
    
    // Already at baseline, can't fall back further
    if (currentTier == RenderTier::Baseline)
    {
        RE_LOG("RenderEngine: Already at baseline tier, cannot fall back further");
        return;
    }
    
    // Decrease tier by one level
    RenderTier newTier = static_cast<RenderTier>(static_cast<int>(currentTier) - 1);
    errorStats_.currentTier.store(newTier, std::memory_order_release);
    errorStats_.fallbackTriggered.store(true, std::memory_order_release);
    
    RE_LOG("RenderEngine: Triggered fallback from tier " << static_cast<int>(currentTier) 
           << " to tier " << static_cast<int>(newTier));
    
    // Log last error that caused fallback
    GLenum lastErr = errorStats_.lastError.load(std::memory_order_relaxed);
    if (lastErr != GL_NO_ERROR)
    {
        juce::SpinLock::ScopedLockType lock(errorStats_.lastOperationLock);
        RE_LOG("RenderEngine: Fallback caused by " << glErrorToString(lastErr) 
               << " in " << errorStats_.lastOperation);
    }
}

void RenderEngine::recordGLError(const char* operation, GLenum error)
{
    errorStats_.recordError(error, operation);
    
    // Check if we should trigger fallback
    if (errorStats_.shouldTriggerFallback())
    {
        triggerFallback();
    }
}

} // namespace oscil
