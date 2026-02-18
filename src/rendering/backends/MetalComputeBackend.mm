/*
    Oscil - Metal Compute Backend Implementation
    GPU compute shader support for Apple Silicon
*/

#include "rendering/backends/MetalComputeBackend.h"
#include "rendering/WaveformRenderState.h"
#include "plugin/HostCompatibility.h"

#if JUCE_MAC

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

namespace oscil
{

// Metal shader source for waveform vertex generation
static NSString* const kMetalShaderSource = @R"(
#include <metal_stdlib>
using namespace metal;

struct WaveformConfig {
    float4 bounds;      // x, y, width, height
    float4 color;       // RGBA
    float4 params;      // opacity, lineWidth, glowIntensity, verticalScale
    float4 viewport;    // viewportWidth, viewportHeight, reserved, reserved
};

kernel void processWaveform(
    device const float* samples [[buffer(0)]],
    device float4* vertices [[buffer(1)]],
    device const WaveformConfig* config [[buffer(2)]],
    constant uint& sampleCount [[buffer(3)]],
    constant float& lineWidth [[buffer(4)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= sampleCount)
        return;
    
    float amplitude = samples[gid];
    
    float boundsX = config->bounds.x;
    float boundsY = config->bounds.y;
    float boundsWidth = config->bounds.z;
    float boundsHeight = config->bounds.w;
    float verticalScale = config->params.w;
    float viewportWidth = config->viewport.x;
    float viewportHeight = config->viewport.y;
    
    // Calculate normalized position within waveform bounds
    float normalizedX = float(gid) / float(sampleCount - 1);
    
    // Convert to pixel coordinates
    float pixelX = boundsX + normalizedX * boundsWidth;
    float pixelY = boundsY + boundsHeight * 0.5 + amplitude * verticalScale * boundsHeight * 0.5;
    
    // Convert to normalized device coordinates (-1 to 1)
    float ndcX = (pixelX / viewportWidth) * 2.0 - 1.0;
    float ndcY = 1.0 - (pixelY / viewportHeight) * 2.0;  // Flip Y
    
    // For ribbon geometry, we create top and bottom vertices
    float halfWidth = lineWidth / viewportHeight;
    
    uint vertexIdx = gid * 2;
    
    // Top vertex
    vertices[vertexIdx] = float4(ndcX, ndcY + halfWidth, 1.0, 0.0);
    
    // Bottom vertex
    vertices[vertexIdx + 1] = float4(ndcX, ndcY - halfWidth, -1.0, 0.0);
}
)";

/**
 * Metal implementation using Objective-C++
 */
class MetalComputeBackend::Impl
{
public:
    Impl() = default;
    
    ~Impl()
    {
        shutdown();
    }
    
    bool initialize()
    {
        @autoreleasepool {
            // Get the default Metal device
            device_ = MTLCreateSystemDefaultDevice();
            if (!device_)
            {
                DBG("MetalComputeBackend: No Metal device available");
                return false;
            }
            
            DBG("MetalComputeBackend: Using device: " << 
                juce::String([device_.name UTF8String]));
            
            // Check for Apple Silicon (Unified Memory)
            if (![device_ hasUnifiedMemory])
            {
                DBG("MetalComputeBackend: Device does not have unified memory");
                // Still usable, but less optimal
            }
            
            // Create command queue
            commandQueue_ = [device_ newCommandQueue];
            if (!commandQueue_)
            {
                DBG("MetalComputeBackend: Failed to create command queue");
                return false;
            }
            
            // Compile the compute shader
            NSError* error = nil;
            id<MTLLibrary> library = [device_ newLibraryWithSource:kMetalShaderSource
                                                           options:nil
                                                             error:&error];
            if (!library)
            {
                DBG("MetalComputeBackend: Failed to compile shader: " <<
                    juce::String([[error localizedDescription] UTF8String]));
                return false;
            }
            
            // Get the compute function
            id<MTLFunction> computeFunction = [library newFunctionWithName:@"processWaveform"];
            if (!computeFunction)
            {
                DBG("MetalComputeBackend: Compute function not found");
                return false;
            }
            
            // Create compute pipeline state
            computePipeline_ = [device_ newComputePipelineStateWithFunction:computeFunction
                                                                      error:&error];
            if (!computePipeline_)
            {
                DBG("MetalComputeBackend: Failed to create pipeline: " <<
                    juce::String([[error localizedDescription] UTF8String]));
                return false;
            }
            
            // Pre-allocate buffers for typical usage
            // These will be resized as needed
            maxSamples_ = 4096;
            
            sampleBuffer_ = [device_ newBufferWithLength:maxSamples_ * sizeof(float)
                                                 options:MTLResourceStorageModeShared];
            
            // Vertex buffer: 2 vertices per sample (ribbon geometry), 4 floats per vertex
            vertexBuffer_ = [device_ newBufferWithLength:maxSamples_ * 2 * sizeof(float) * 4
                                                 options:MTLResourceStorageModeShared];
            
            configBuffer_ = [device_ newBufferWithLength:sizeof(float) * 16  // 4 vec4s
                                                 options:MTLResourceStorageModeShared];
            
            if (!sampleBuffer_ || !vertexBuffer_ || !configBuffer_)
            {
                DBG("MetalComputeBackend: Failed to allocate buffers");
                return false;
            }
            
            DBG("MetalComputeBackend: Initialized successfully");
            return true;
        }
    }
    
    void shutdown()
    {
        @autoreleasepool {
            sampleBuffer_ = nil;
            vertexBuffer_ = nil;
            configBuffer_ = nil;
            computePipeline_ = nil;
            commandQueue_ = nil;
            device_ = nil;
        }
    }
    
    bool processWaveform(const float* samples, 
                         size_t sampleCount,
                         float boundsX, float boundsY, 
                         float boundsWidth, float boundsHeight,
                         float viewportWidth, float viewportHeight,
                         float lineWidth, float verticalScale)
    {
        @autoreleasepool {
            if (!device_ || !computePipeline_ || !commandQueue_)
                return false;
            
            // Resize buffers if needed
            if (sampleCount > maxSamples_)
            {
                // Allocate new buffers before updating maxSamples_ to avoid
                // leaving state inconsistent if allocation fails
                id<MTLBuffer> newSampleBuffer = [device_ newBufferWithLength:sampleCount * sizeof(float)
                                                     options:MTLResourceStorageModeShared];
                id<MTLBuffer> newVertexBuffer = [device_ newBufferWithLength:sampleCount * 2 * sizeof(float) * 4
                                                     options:MTLResourceStorageModeShared];
                if (!newSampleBuffer || !newVertexBuffer)
                    return false;

                // Only update state after successful allocation
                sampleBuffer_ = newSampleBuffer;
                vertexBuffer_ = newVertexBuffer;
                maxSamples_ = sampleCount;
            }
            
            // Copy sample data to GPU buffer
            memcpy([sampleBuffer_ contents], samples, sampleCount * sizeof(float));
            
            // Set up config
            struct Config {
                float bounds[4];
                float color[4];
                float params[4];
                float viewport[4];
            };
            Config* config = static_cast<Config*>([configBuffer_ contents]);
            config->bounds[0] = boundsX;
            config->bounds[1] = boundsY;
            config->bounds[2] = boundsWidth;
            config->bounds[3] = boundsHeight;
            config->color[0] = config->color[1] = config->color[2] = config->color[3] = 1.0f;
            config->params[0] = 1.0f;  // opacity
            config->params[1] = lineWidth;
            config->params[2] = 0.5f;  // glowIntensity
            config->params[3] = verticalScale;
            config->viewport[0] = viewportWidth;
            config->viewport[1] = viewportHeight;
            config->viewport[2] = config->viewport[3] = 0.0f;
            
            // Create command buffer
            id<MTLCommandBuffer> commandBuffer = [commandQueue_ commandBuffer];
            if (!commandBuffer)
                return false;
            
            // Create compute command encoder
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            if (!encoder)
                return false;
            
            // Set pipeline and buffers
            [encoder setComputePipelineState:computePipeline_];
            [encoder setBuffer:sampleBuffer_ offset:0 atIndex:0];
            [encoder setBuffer:vertexBuffer_ offset:0 atIndex:1];
            [encoder setBuffer:configBuffer_ offset:0 atIndex:2];
            
            uint32_t sampleCountU32 = static_cast<uint32_t>(sampleCount);
            [encoder setBytes:&sampleCountU32 length:sizeof(uint32_t) atIndex:3];
            [encoder setBytes:&lineWidth length:sizeof(float) atIndex:4];
            
            // Calculate thread groups
            NSUInteger threadGroupSize = computePipeline_.maxTotalThreadsPerThreadgroup;
            if (threadGroupSize > sampleCount)
                threadGroupSize = sampleCount;
            
            MTLSize threadGroupSizeStruct = MTLSizeMake(threadGroupSize, 1, 1);
            MTLSize numThreadGroups = MTLSizeMake((sampleCount + threadGroupSize - 1) / threadGroupSize, 1, 1);
            
            [encoder dispatchThreadgroups:numThreadGroups threadsPerThreadgroup:threadGroupSizeStruct];
            [encoder endEncoding];
            
            // Execute and wait (synchronous for now, could be async)
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            
            lastVertexCount_ = sampleCount * 2;
            return true;
        }
    }
    
    const float* getVertexData(size_t& vertexCount) const
    {
        vertexCount = lastVertexCount_;
        return static_cast<const float*>([vertexBuffer_ contents]);
    }
    
private:
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> commandQueue_ = nil;
    id<MTLComputePipelineState> computePipeline_ = nil;
    
    id<MTLBuffer> sampleBuffer_ = nil;
    id<MTLBuffer> vertexBuffer_ = nil;
    id<MTLBuffer> configBuffer_ = nil;
    
    size_t maxSamples_ = 0;
    size_t lastVertexCount_ = 0;
};

// =============================================================================
// Public API implementation
// =============================================================================

MetalComputeBackend::MetalComputeBackend()
    : impl_(std::make_unique<Impl>())
{
}

MetalComputeBackend::~MetalComputeBackend()
{
    shutdown();
}

bool MetalComputeBackend::isAvailable()
{
    #if defined(__arm64__) || defined(__aarch64__)
    // Apple Silicon - Metal is always available
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        return device != nil;
    }
    #else
    // Intel Mac - Metal may be available but compute less useful
    return false;
    #endif
}

bool MetalComputeBackend::initialize()
{
    // Check host compatibility first
    HostCapabilityLevel hostLevel = HostCompatibility::detectHost();
    if (hostLevel == HostCapabilityLevel::SafeMode || 
        hostLevel == HostCapabilityLevel::NoMetal)
    {
        DBG("MetalComputeBackend: Disabled due to host compatibility");
        return false;
    }
    
    if (!isAvailable())
    {
        DBG("MetalComputeBackend: Metal not available on this system");
        return false;
    }
    
    initialized_ = impl_->initialize();
    return initialized_;
}

void MetalComputeBackend::shutdown()
{
    if (impl_)
        impl_->shutdown();
    initialized_ = false;
}

bool MetalComputeBackend::processWaveform(const float* samples,
                                          size_t sampleCount,
                                          int waveformId,
                                          const WaveformRenderState& config)
{
    (void)waveformId;  // Currently single waveform processing
    
    if (!initialized_ || !impl_)
        return false;
    
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // INFRASTRUCTURE NOTE: This is a placeholder implementation.
    // Full integration requires:
    // 1. Extracting bounds/viewport from WaveformRenderData (not available here)
    // 2. Connecting to the main render loop in RenderEngine
    // 3. Using the generated vertices in the OpenGL render path
    //
    // The Metal compute pipeline is functional for testing but not yet
    // integrated with the main rendering system. This is an opt-in Tier 4
    // feature that requires explicit user enablement.
    
    // Placeholder values - in production, these would come from render data
    // WaveformRenderState's visualConfig doesn't have lineWidth directly,
    // it's in WaveformRenderData. Using defaults for this placeholder.
    float boundsX = 0.0f, boundsY = 0.0f;
    float boundsWidth = 800.0f, boundsHeight = 400.0f;
    float viewportWidth = 800.0f, viewportHeight = 600.0f;
    float lineWidth = 2.0f;  // Default line width
    float verticalScale = 1.0f;
    
    (void)config;  // Will be used when full integration is complete
    
    bool result = impl_->processWaveform(samples, sampleCount,
                                         boundsX, boundsY, boundsWidth, boundsHeight,
                                         viewportWidth, viewportHeight,
                                         lineWidth, verticalScale);
    
    auto endTime = juce::Time::getHighResolutionTicks();
    double timeMs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
    
    stats_.lastComputeTimeMs = timeMs;
    stats_.totalComputeCalls++;
    stats_.avgComputeTimeMs = (stats_.avgComputeTimeMs * (stats_.totalComputeCalls - 1) + timeMs) 
                              / stats_.totalComputeCalls;
    
    return result;
}

const float* MetalComputeBackend::getVertexData(int waveformId, size_t& vertexCount) const
{
    (void)waveformId;
    
    if (!initialized_ || !impl_)
    {
        vertexCount = 0;
        return nullptr;
    }
    
    return impl_->getVertexData(vertexCount);
}

} // namespace oscil

#endif // JUCE_MAC

