/*
    Oscil - GPU Capabilities Detection
    Detects OpenGL/Metal features available on the current GPU.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>

namespace oscil
{

/**
 * GPU capabilities detected at runtime.
 * Used to determine which rendering tier is available.
 */
struct GPUCapabilities
{
    // OpenGL version info
    int glMajorVersion = 0;
    int glMinorVersion = 0;
    juce::String glVersionString;
    juce::String glRenderer;
    juce::String glVendor;

    // Core feature availability
    bool hasInstancing = false;        // GL 3.3+ - glDrawArraysInstanced
    bool hasUBO = false;               // GL 3.1+ - Uniform Buffer Objects
    bool hasSSBO = false;              // GL 4.3+ or ARB_shader_storage_buffer_object
    bool hasCompute = false;           // GL 4.3+ - Compute shaders
    bool hasPersistentMap = false;     // GL 4.4+ or ARB_buffer_storage
    bool hasMultiDrawIndirect = false; // GL 4.3+ - Multi-draw indirect

    // Platform-specific
    bool hasMetal = false;             // Apple Silicon with Metal support
    bool isAppleSilicon = false;       // Running on Apple Silicon

    // Limits
    int maxTextureSize = 0;
    int maxUniformBlockSize = 0;
    int maxSSBOSize = 0;
    int maxComputeWorkGroupSize = 0;

    /**
     * Detect GPU capabilities from the current OpenGL context.
     * MUST be called from the OpenGL thread with an active context.
     * 
     * @param context The active OpenGL context
     * @return Populated GPUCapabilities struct
     */
    static GPUCapabilities detect(juce::OpenGLContext& context);

    /**
     * Check if a specific OpenGL extension is supported.
     * 
     * @param extensionName The extension name (e.g., "GL_ARB_buffer_storage")
     * @return true if the extension is available
     */
    static bool hasExtension(const juce::String& extensionName);

    /**
     * Get a human-readable summary of capabilities.
     */
    [[nodiscard]] juce::String getSummary() const;

    /**
     * Determine the maximum safe rendering tier based on capabilities.
     * Does NOT consider host compatibility - use in conjunction with HostCompatibility.
     */
    [[nodiscard]] int getMaxTier() const;

private:
    // Thread-safe extension caching with double-checked locking
    static inline juce::String cachedExtensions_;
    static inline std::atomic<bool> extensionsCached_{false};
    static inline juce::SpinLock extensionsCacheLock_;
};

// =============================================================================
// Implementation
// =============================================================================

inline GPUCapabilities GPUCapabilities::detect(juce::OpenGLContext& context)
{
    using namespace juce::gl;
    
    // Context parameter reserved for future use (e.g., context-specific queries)
    (void)context;
    
    GPUCapabilities caps;

    // Get version info
    glGetIntegerv(GL_MAJOR_VERSION, &caps.glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &caps.glMinorVersion);

    const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

    caps.glVersionString = versionStr ? versionStr : "Unknown";
    caps.glRenderer = rendererStr ? rendererStr : "Unknown";
    caps.glVendor = vendorStr ? vendorStr : "Unknown";

    // Calculate effective version as float for comparison
    float version = static_cast<float>(caps.glMajorVersion) + 
                    static_cast<float>(caps.glMinorVersion) * 0.1f;

    // Detect core features based on version
    caps.hasInstancing = version >= 3.3f;
    caps.hasUBO = version >= 3.1f;
    caps.hasSSBO = version >= 4.3f || hasExtension("GL_ARB_shader_storage_buffer_object");
    caps.hasCompute = version >= 4.3f;
    caps.hasPersistentMap = version >= 4.4f || hasExtension("GL_ARB_buffer_storage");
    caps.hasMultiDrawIndirect = version >= 4.3f || hasExtension("GL_ARB_multi_draw_indirect");

    // Get limits
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);
    
    if (caps.hasUBO)
    {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &caps.maxUniformBlockSize);
    }

    if (caps.hasSSBO)
    {
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &caps.maxSSBOSize);
    }

    if (caps.hasCompute)
    {
        // GL_MAX_COMPUTE_WORK_GROUP_SIZE requires an index (X=0, Y=1, Z=2)
        // We query the X dimension as a representative value
        // Note: glGetIntegeri_v is the correct function but we use a simpler approach
        GLint workGroupSizes[3] = {0, 0, 0};
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSizes[0]);
        caps.maxComputeWorkGroupSize = workGroupSizes[0];
    }

    // Detect Apple Silicon
    #if JUCE_MAC
    {
        // Check if running on Apple Silicon
        #if defined(__arm64__) || defined(__aarch64__)
        caps.isAppleSilicon = true;
        caps.hasMetal = true;  // Apple Silicon always has Metal
        #else
        caps.isAppleSilicon = false;
        // Intel Mac - check for Metal support (macOS 10.14+)
        caps.hasMetal = false;  // Conservative: don't assume Metal on Intel
        #endif
    }
    #endif

    // Log detection results
    DBG("GPUCapabilities detected:");
    DBG("  OpenGL: " << caps.glVersionString);
    DBG("  Renderer: " << caps.glRenderer);
    DBG("  Vendor: " << caps.glVendor);
    DBG("  Instancing: " << (caps.hasInstancing ? "Yes" : "No"));
    DBG("  UBO: " << (caps.hasUBO ? "Yes" : "No"));
    DBG("  SSBO: " << (caps.hasSSBO ? "Yes" : "No"));
    DBG("  Compute: " << (caps.hasCompute ? "Yes" : "No"));
    DBG("  Persistent Map: " << (caps.hasPersistentMap ? "Yes" : "No"));
    DBG("  Metal: " << (caps.hasMetal ? "Yes" : "No"));
    DBG("  Apple Silicon: " << (caps.isAppleSilicon ? "Yes" : "No"));

    return caps;
}

inline bool GPUCapabilities::hasExtension(const juce::String& extensionName)
{
    using namespace juce::gl;

    // Double-checked locking for thread-safe initialization
    // First check without lock (fast path when already cached)
    if (!extensionsCached_.load(std::memory_order_acquire))
    {
        // Lock and check again before initializing
        juce::SpinLock::ScopedLockType lock(extensionsCacheLock_);

        if (!extensionsCached_.load(std::memory_order_relaxed))
        {
            const char* extStr = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
            if (extStr)
            {
                cachedExtensions_ = extStr;
            }
            else
            {
                // GL 3.0+ uses glGetStringi
                GLint numExtensions = 0;
                glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

                juce::StringArray extensions;
                for (GLint i = 0; i < numExtensions; ++i)
                {
                    const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
                    if (ext)
                    {
                        extensions.add(ext);
                    }
                }
                cachedExtensions_ = extensions.joinIntoString(" ");
            }
            // Use release to ensure cachedExtensions_ write is visible before flag
            extensionsCached_.store(true, std::memory_order_release);
        }
    }

    return cachedExtensions_.contains(extensionName);
}

inline juce::String GPUCapabilities::getSummary() const
{
    juce::String summary;
    summary << "OpenGL " << glMajorVersion << "." << glMinorVersion;
    summary << " (" << glRenderer << ")";
    summary << "\nFeatures: ";
    
    juce::StringArray features;
    if (hasInstancing) features.add("Instancing");
    if (hasUBO) features.add("UBO");
    if (hasSSBO) features.add("SSBO");
    if (hasCompute) features.add("Compute");
    if (hasPersistentMap) features.add("PersistentMap");
    if (hasMetal) features.add("Metal");
    
    summary << (features.isEmpty() ? "None" : features.joinIntoString(", "));
    summary << "\nMax Tier: " << getMaxTier();
    
    return summary;
}

inline int GPUCapabilities::getMaxTier() const
{
    // Tier 4: Compute (GL 4.3+ or Metal)
    if (hasCompute || hasMetal)
        return 4;
    
    // Tier 3: SSBO (GL 4.3+)
    if (hasSSBO)
        return 3;
    
    // Tier 2: Instancing (GL 3.3+)
    if (hasInstancing && hasUBO)
        return 2;
    
    // Tier 1: Baseline (GL 3.2+)
    return 1;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL

