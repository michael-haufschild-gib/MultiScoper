/*
    Oscil - Per-Oscillator Quality Override
    Allows individual oscillators to use different quality than the global setting
*/

#pragma once

#include "core/dsp/QualityPreset.h"

#include <juce_core/juce_core.h>

namespace oscil
{

/**
 * Quality override for individual oscillators
 * Allows specific tracks to use different quality than global setting
 */
enum class QualityOverride
{
    UseGlobal, // Use the global CaptureQualityConfig setting
    Eco,
    Standard,
    High,
    Ultra
};

inline juce::String qualityOverrideToString(QualityOverride override)
{
    switch (override)
    {
        case QualityOverride::UseGlobal:
            return "UseGlobal";
        case QualityOverride::Eco:
            return "Eco";
        case QualityOverride::Standard:
            return "Standard";
        case QualityOverride::High:
            return "High";
        case QualityOverride::Ultra:
            return "Ultra";
    }
    return "UseGlobal";
}

inline QualityOverride stringToQualityOverride(const juce::String& str)
{
    if (str == "Eco")
        return QualityOverride::Eco;
    if (str == "Standard")
        return QualityOverride::Standard;
    if (str == "High")
        return QualityOverride::High;
    if (str == "Ultra")
        return QualityOverride::Ultra;
    return QualityOverride::UseGlobal;
}

inline juce::String qualityOverrideToDisplayName(QualityOverride override)
{
    switch (override)
    {
        case QualityOverride::UseGlobal:
            return "Use Global";
        case QualityOverride::Eco:
            return "Eco (11 kHz)";
        case QualityOverride::Standard:
            return "Standard (22 kHz)";
        case QualityOverride::High:
            return "High (44 kHz)";
        case QualityOverride::Ultra:
            return "Ultra (Source Rate)";
    }
    return "Use Global";
}

/**
 * Resolve override to actual quality preset
 * @param override The per-oscillator override setting
 * @param globalPreset The global quality preset (used when override is UseGlobal)
 * @return The resolved quality preset
 */
inline QualityPreset resolveQualityOverride(QualityOverride override, QualityPreset globalPreset)
{
    switch (override)
    {
        case QualityOverride::UseGlobal:
            return globalPreset;
        case QualityOverride::Eco:
            return QualityPreset::Eco;
        case QualityOverride::Standard:
            return QualityPreset::Standard;
        case QualityOverride::High:
            return QualityPreset::High;
        case QualityOverride::Ultra:
            return QualityPreset::Ultra;
    }
    return globalPreset;
}

} // namespace oscil
