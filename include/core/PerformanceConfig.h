/*
    Oscil - Performance Configuration Entity
    Manages quality modes and performance monitoring thresholds
    PRD aligned: Entities -> PerformanceConfig, StatusBarConfig
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <string>
#include <cmath>

namespace oscil
{

/**
 * Quality mode for rendering
 * PRD aligned with performance/quality tradeoff levels
 */
enum class QualityMode
{
    HIGHEST,      // 60fps, full resolution, all effects
    HIGH,         // 60fps, 75% resolution
    BALANCED,     // 45fps, 50% resolution
    PERFORMANCE,  // 30fps, 25% resolution
    MAXIMUM       // 15fps, minimal quality - emergency mode
};

inline juce::String qualityModeToString(QualityMode mode)
{
    switch (mode)
    {
        case QualityMode::HIGHEST:     return "HIGHEST";
        case QualityMode::HIGH:        return "HIGH";
        case QualityMode::BALANCED:    return "BALANCED";
        case QualityMode::PERFORMANCE: return "PERFORMANCE";
        case QualityMode::MAXIMUM:     return "MAXIMUM";
        default:                       return "BALANCED";
    }
}

inline QualityMode stringToQualityMode(const juce::String& str)
{
    if (str == "HIGHEST")     return QualityMode::HIGHEST;
    if (str == "HIGH")        return QualityMode::HIGH;
    if (str == "BALANCED")    return QualityMode::BALANCED;
    if (str == "PERFORMANCE") return QualityMode::PERFORMANCE;
    if (str == "MAXIMUM")     return QualityMode::MAXIMUM;
    return QualityMode::BALANCED;
}

/**
 * Rendering mode (OpenGL or Software)
 */
enum class RenderingMode
{
    OPENGL,    // GPU-accelerated rendering
    SOFTWARE   // CPU-based fallback rendering
};

inline juce::String renderingModeToString(RenderingMode mode)
{
    return mode == RenderingMode::OPENGL ? "OPENGL" : "SOFTWARE";
}

inline RenderingMode stringToRenderingMode(const juce::String& str)
{
    return str == "OPENGL" ? RenderingMode::OPENGL : RenderingMode::SOFTWARE;
}

/**
 * Status bar position preference
 */
enum class StatusBarPosition
{
    BOTTOM,
    TOP
};

/**
 * Performance configuration structure
 * PRD aligned: Entities -> PerformanceConfig
 */
struct PerformanceConfig
{
    // Quality settings
    QualityMode qualityMode = QualityMode::BALANCED;
    float refreshRate = 60.0f;       // 15-120 fps target refresh rate
    int decimationLevel = 0;         // 0-4 level of sample decimation

    // Status bar settings
    bool statusBarVisible = true;
    bool measurePerformance = true;  // Tied to status bar visibility

    // Thresholds (PRD defined)
    float cpuWarningThreshold = 10.0f;   // 5.0-20.0% - Warn at this CPU usage
    float cpuCriticalThreshold = 15.0f;  // 10.0-25.0% - Critical at this CPU usage
    int memoryWarningThreshold = 500;    // 200-1000 MB - Warn at this memory usage
    int memoryCriticalThreshold = 800;   // 500-1500 MB - Critical at this memory usage

    // Auto-adaptation
    bool autoQualityReduction = true;    // Allow automatic quality reduction

    // Constraints from PRD
    static constexpr float MIN_REFRESH_RATE = 15.0f;
    static constexpr float MAX_REFRESH_RATE = 120.0f;
    static constexpr int MIN_DECIMATION = 0;
    static constexpr int MAX_DECIMATION = 4;

    static constexpr float MIN_CPU_WARNING = 5.0f;
    static constexpr float MAX_CPU_WARNING = 20.0f;
    static constexpr float MIN_CPU_CRITICAL = 10.0f;
    static constexpr float MAX_CPU_CRITICAL = 25.0f;

    static constexpr int MIN_MEMORY_WARNING = 200;
    static constexpr int MAX_MEMORY_WARNING = 1000;
    static constexpr int MIN_MEMORY_CRITICAL = 500;
    static constexpr int MAX_MEMORY_CRITICAL = 1500;

    /**
     * Get target FPS for current quality mode
     */
    float getTargetFps() const
    {
        switch (qualityMode)
        {
            case QualityMode::HIGHEST:     return 60.0f;
            case QualityMode::HIGH:        return 60.0f;
            case QualityMode::BALANCED:    return 45.0f;
            case QualityMode::PERFORMANCE: return 30.0f;
            case QualityMode::MAXIMUM:     return 15.0f;
            default:                       return 45.0f;
        }
    }

    /**
     * Get decimation level for current quality mode
     */
    int getDecimationForQuality() const
    {
        switch (qualityMode)
        {
            case QualityMode::HIGHEST:     return 0;
            case QualityMode::HIGH:        return 1;
            case QualityMode::BALANCED:    return 2;
            case QualityMode::PERFORMANCE: return 3;
            case QualityMode::MAXIMUM:     return 4;
            default:                       return 2;
        }
    }

    /**
     * Get resolution scale factor for current quality mode (0.0-1.0)
     */
    float getResolutionScale() const
    {
        switch (qualityMode)
        {
            case QualityMode::HIGHEST:     return 1.0f;
            case QualityMode::HIGH:        return 0.75f;
            case QualityMode::BALANCED:    return 0.50f;
            case QualityMode::PERFORMANCE: return 0.25f;
            case QualityMode::MAXIMUM:     return 0.125f;
            default:                       return 0.50f;
        }
    }

    /**
     * Check if CPU usage is above warning threshold
     */
    bool isCpuWarning(float cpuUsage) const
    {
        return cpuUsage >= cpuWarningThreshold;
    }

    /**
     * Check if CPU usage is critical
     */
    bool isCpuCritical(float cpuUsage) const
    {
        return cpuUsage >= cpuCriticalThreshold;
    }

    /**
     * Check if memory usage is above warning threshold
     */
    bool isMemoryWarning(int memoryMb) const
    {
        return memoryMb >= memoryWarningThreshold;
    }

    /**
     * Check if memory usage is critical
     */
    bool isMemoryCritical(int memoryMb) const
    {
        return memoryMb >= memoryCriticalThreshold;
    }

    /**
     * Suggest quality reduction based on CPU usage
     * @return suggested QualityMode, or current if no change needed
     */
    QualityMode suggestQualityForCpu(float cpuUsage) const
    {
        if (!autoQualityReduction) return qualityMode;

        if (cpuUsage >= cpuCriticalThreshold)
            return QualityMode::MAXIMUM;
        if (cpuUsage >= cpuWarningThreshold)
        {
            // Step down one level
            switch (qualityMode)
            {
                case QualityMode::HIGHEST:     return QualityMode::HIGH;
                case QualityMode::HIGH:        return QualityMode::BALANCED;
                case QualityMode::BALANCED:    return QualityMode::PERFORMANCE;
                case QualityMode::PERFORMANCE: return QualityMode::MAXIMUM;
                default:                       return qualityMode;
            }
        }
        return qualityMode;
    }

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("PerformanceConfig");
        tree.setProperty("qualityMode", qualityModeToString(qualityMode), nullptr);
        tree.setProperty("refreshRate", refreshRate, nullptr);
        tree.setProperty("decimationLevel", decimationLevel, nullptr);
        tree.setProperty("statusBarVisible", statusBarVisible, nullptr);
        tree.setProperty("measurePerformance", measurePerformance, nullptr);
        tree.setProperty("cpuWarningThreshold", cpuWarningThreshold, nullptr);
        tree.setProperty("cpuCriticalThreshold", cpuCriticalThreshold, nullptr);
        tree.setProperty("memoryWarningThreshold", memoryWarningThreshold, nullptr);
        tree.setProperty("memoryCriticalThreshold", memoryCriticalThreshold, nullptr);
        tree.setProperty("autoQualityReduction", autoQualityReduction, nullptr);
        return tree;
    }

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid()) return;

        auto sanitizeFloat = [](float value, float fallback)
        {
            return std::isfinite(value) ? value : fallback;
        };

        qualityMode = stringToQualityMode(tree.getProperty("qualityMode", "BALANCED").toString());
        refreshRate = juce::jlimit(MIN_REFRESH_RATE, MAX_REFRESH_RATE,
                                   sanitizeFloat(static_cast<float>(tree.getProperty("refreshRate", 60.0f)), 60.0f));
        decimationLevel = juce::jlimit(MIN_DECIMATION, MAX_DECIMATION,
                                       static_cast<int>(tree.getProperty("decimationLevel", 0)));
        statusBarVisible = static_cast<bool>(tree.getProperty("statusBarVisible", true));
        measurePerformance = static_cast<bool>(tree.getProperty("measurePerformance", true));
        cpuWarningThreshold = juce::jlimit(MIN_CPU_WARNING, MAX_CPU_WARNING,
                                           sanitizeFloat(static_cast<float>(tree.getProperty("cpuWarningThreshold", 10.0f)), 10.0f));
        cpuCriticalThreshold = juce::jlimit(MIN_CPU_CRITICAL, MAX_CPU_CRITICAL,
                                            sanitizeFloat(static_cast<float>(tree.getProperty("cpuCriticalThreshold", 15.0f)), 15.0f));
        memoryWarningThreshold = juce::jlimit(MIN_MEMORY_WARNING, MAX_MEMORY_WARNING,
                                              static_cast<int>(tree.getProperty("memoryWarningThreshold", 500)));
        memoryCriticalThreshold = juce::jlimit(MIN_MEMORY_CRITICAL, MAX_MEMORY_CRITICAL,
                                               static_cast<int>(tree.getProperty("memoryCriticalThreshold", 800)));
        autoQualityReduction = static_cast<bool>(tree.getProperty("autoQualityReduction", true));
    }
};

/**
 * Status bar configuration
 * PRD aligned: Entities -> StatusBarConfig
 */
struct StatusBarConfig
{
    bool enabled = true;
    StatusBarPosition position = StatusBarPosition::BOTTOM;
    bool showCPU = true;
    bool showMemory = true;
    bool showFPS = true;
    bool showRenderingMode = true;
    int updateIntervalMs = 100;  // 100-1000 ms

    static constexpr int MIN_UPDATE_INTERVAL = 100;
    static constexpr int MAX_UPDATE_INTERVAL = 1000;

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("StatusBarConfig");
        tree.setProperty("enabled", enabled, nullptr);
        tree.setProperty("position", position == StatusBarPosition::TOP ? "TOP" : "BOTTOM", nullptr);
        tree.setProperty("showCPU", showCPU, nullptr);
        tree.setProperty("showMemory", showMemory, nullptr);
        tree.setProperty("showFPS", showFPS, nullptr);
        tree.setProperty("showRenderingMode", showRenderingMode, nullptr);
        tree.setProperty("updateIntervalMs", updateIntervalMs, nullptr);
        return tree;
    }

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid()) return;

        enabled = static_cast<bool>(tree.getProperty("enabled", true));
        position = tree.getProperty("position", "BOTTOM").toString() == "TOP"
                   ? StatusBarPosition::TOP : StatusBarPosition::BOTTOM;
        showCPU = static_cast<bool>(tree.getProperty("showCPU", true));
        showMemory = static_cast<bool>(tree.getProperty("showMemory", true));
        showFPS = static_cast<bool>(tree.getProperty("showFPS", true));
        showRenderingMode = static_cast<bool>(tree.getProperty("showRenderingMode", true));
        updateIntervalMs = juce::jlimit(MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL,
                                         static_cast<int>(tree.getProperty("updateIntervalMs", 100)));
    }
};

/**
 * Real-time performance metrics
 * PRD aligned: Entities -> StatusBar (metrics)
 */
struct PerformanceMetrics
{
    float cpuUsage = 0.0f;          // 0.0-100.0%
    float memoryUsage = 0.0f;       // Current memory usage in MB
    float fps = 0.0f;               // Current frames per second
    RenderingMode renderingMode = RenderingMode::SOFTWARE;

    // Extended metrics
    float audioThreadLoad = 0.0f;   // Audio thread CPU usage
    float uiThreadLoad = 0.0f;      // UI thread CPU usage
    int activeOscillators = 0;      // Number of active oscillators
    int activeSources = 0;          // Number of active sources
};

} // namespace oscil
