/*
    Oscil - Host Compatibility Detection
    Detects the host application and determines GPU feature availability.
    
    This is critical for plugin stability across different DAWs and hosts.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <map>
#include <string>

namespace oscil
{

/**
 * Capability level determines which rendering features are safe to use.
 * Higher levels enable more advanced GPU features.
 */
enum class HostCapabilityLevel
{
    Full,           // All features available (Tier 4: Compute shaders)
    NoCompute,      // Disable compute shaders (Tier 3 max: SSBO)
    NoMetal,        // Metal conflicts, use OpenGL only (Tier 3 max)
    SafeMode        // Tier 1 only (basic batched rendering)
};

/**
 * Converts HostCapabilityLevel to string for logging.
 */
inline const char* hostCapabilityLevelToString(HostCapabilityLevel level)
{
    switch (level)
    {
        case HostCapabilityLevel::Full:      return "Full";
        case HostCapabilityLevel::NoCompute: return "NoCompute";
        case HostCapabilityLevel::NoMetal:   return "NoMetal";
        case HostCapabilityLevel::SafeMode:  return "SafeMode";
        default:                             return "Unknown";
    }
}

/**
 * Host compatibility detection and management.
 * 
 * Maintains a database of known problematic hosts and their required
 * capability restrictions. Also supports runtime detection based on
 * host name patterns.
 * 
 * Thread Safety: All methods are thread-safe (read-only after initialization).
 */
class HostCompatibility
{
public:
    /**
     * Detect the current host and return its capability level.
     * Call this once at plugin load time.
     * 
     * @param processor Optional pointer to AudioProcessor for host info
     * @return The capability level for the current host
     */
    static HostCapabilityLevel detectHost(juce::AudioProcessor* processor = nullptr);

    /**
     * Get the name of the detected host.
     */
    static juce::String getHostName();

    /**
     * Check if a specific host name matches any problematic patterns.
     * Useful for unit testing.
     * 
     * @param hostName The host name to check
     * @return The capability level for that host
     */
    static HostCapabilityLevel getCapabilityForHost(const juce::String& hostName);

    /**
     * Force a specific capability level (for testing/debugging).
     * Pass nullptr to clear the override.
     */
    static void setOverride(HostCapabilityLevel* level);

    /**
     * Check if we're running in standalone mode.
     * Standalone typically has full capability.
     */
    static bool isStandalone();

    /**
     * Get human-readable description of why a host has restricted capabilities.
     */
    static juce::String getRestrictionReason(HostCapabilityLevel level);

private:
    // Cached host detection results
    static inline std::atomic<bool> detectionComplete_{false};
    static inline HostCapabilityLevel cachedLevel_{HostCapabilityLevel::Full};
    static inline juce::String cachedHostName_;
    static inline juce::SpinLock cacheLock_;
    
    // Override for testing
    static inline HostCapabilityLevel* overrideLevel_{nullptr};

    /**
     * Known problematic hosts and their capability restrictions.
     * 
     * Format: { "host_name_pattern", CapabilityLevel }
     * 
     * Patterns are case-insensitive substring matches.
     * More specific patterns should be listed first.
     */
    static inline const std::vector<std::pair<juce::String, HostCapabilityLevel>> knownHosts_ = {
        // Adobe products - aggressive graphics handling, force safe mode
        {"Adobe Audition", HostCapabilityLevel::SafeMode},
        {"Adobe Premiere", HostCapabilityLevel::SafeMode},
        {"Adobe", HostCapabilityLevel::SafeMode},
        
        // Audacity - basic plugin hosting, limited OpenGL support
        {"Audacity", HostCapabilityLevel::SafeMode},
        
        // OBS Studio - has its own rendering pipeline, Metal conflicts
        {"OBS", HostCapabilityLevel::NoMetal},
        
        // Sound Forge - older graphics handling
        {"Sound Forge", HostCapabilityLevel::NoCompute},
        {"SOUND FORGE", HostCapabilityLevel::NoCompute},
        
        // Older GarageBand versions - limited GPU support
        {"GarageBand", HostCapabilityLevel::NoCompute},
        
        // Some older versions of these may have issues
        // but modern versions are generally fine, so we don't restrict them:
        // - Logic Pro
        // - Ableton Live  
        // - Pro Tools
        // - Cubase
        // - FL Studio
        // - Reaper
        // - Bitwig
        // - Studio One
    };

    /**
     * Perform actual host detection (called once, results cached).
     */
    static void performDetection(juce::AudioProcessor* processor);
};

// =============================================================================
// Implementation (inline for header-only convenience)
// =============================================================================

inline HostCapabilityLevel HostCompatibility::detectHost(juce::AudioProcessor* processor)
{
    // Check for override first
    if (overrideLevel_ != nullptr)
    {
        return *overrideLevel_;
    }

    // Return cached result if already detected
    if (detectionComplete_.load(std::memory_order_acquire))
    {
        return cachedLevel_;
    }

    // Perform detection with lock
    juce::SpinLock::ScopedLockType lock(cacheLock_);
    
    // Double-check after acquiring lock
    if (!detectionComplete_.load(std::memory_order_relaxed))
    {
        performDetection(processor);
        detectionComplete_.store(true, std::memory_order_release);
    }

    return cachedLevel_;
}

inline juce::String HostCompatibility::getHostName()
{
    // Ensure detection has run
    detectHost(nullptr);
    
    juce::SpinLock::ScopedLockType lock(cacheLock_);
    return cachedHostName_;
}

inline HostCapabilityLevel HostCompatibility::getCapabilityForHost(const juce::String& hostName)
{
    juce::String lowerName = hostName.toLowerCase();
    
    for (const auto& [pattern, level] : knownHosts_)
    {
        if (lowerName.contains(pattern.toLowerCase()))
        {
            return level;
        }
    }
    
    return HostCapabilityLevel::Full;
}

inline void HostCompatibility::setOverride(HostCapabilityLevel* level)
{
    overrideLevel_ = level;
}

inline bool HostCompatibility::isStandalone()
{
    juce::String hostName = getHostName();
    return hostName.isEmpty() || 
           hostName.containsIgnoreCase("Standalone") ||
           hostName.containsIgnoreCase("Oscil");
}

inline juce::String HostCompatibility::getRestrictionReason(HostCapabilityLevel level)
{
    switch (level)
    {
        case HostCapabilityLevel::Full:
            return "No restrictions - all GPU features available";
            
        case HostCapabilityLevel::NoCompute:
            return "Compute shaders disabled for this host - may cause stability issues";
            
        case HostCapabilityLevel::NoMetal:
            return "Metal disabled for this host - conflicts with host rendering";
            
        case HostCapabilityLevel::SafeMode:
            return "Safe mode enabled - only basic rendering for maximum compatibility";
            
        default:
            return "Unknown restriction level";
    }
}

inline void HostCompatibility::performDetection(juce::AudioProcessor* processor)
{
    // Try to get host info from the processor
    // Note: AudioProcessor can provide host info via getPlayHead(), but the
    // host name is more reliably obtained via PluginHostType below.
    (void)processor;  // Reserved for future use

    // Fallback: Get host application name from JUCE
    cachedHostName_ = juce::JUCEApplicationBase::isStandaloneApp() 
        ? "Standalone" 
        : juce::PluginHostType().getHostDescription();

    // If still empty, try the host type
    if (cachedHostName_.isEmpty())
    {
        juce::PluginHostType hostType;
        cachedHostName_ = hostType.getHostPath();
        
        if (cachedHostName_.isEmpty())
        {
            cachedHostName_ = "Unknown Host";
        }
    }

    // Determine capability level
    cachedLevel_ = getCapabilityForHost(cachedHostName_);

    // Log detection result
    DBG("HostCompatibility: Detected host '" << cachedHostName_ 
        << "' with capability level: " << hostCapabilityLevelToString(cachedLevel_));
}

} // namespace oscil

