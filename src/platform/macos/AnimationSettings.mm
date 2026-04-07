/*
    Oscil - Animation Settings macOS Implementation
    Detects system accessibility preferences for reduced motion
*/

#include "ui/components/AnimationSettings.h"

#if JUCE_MAC
#import <Cocoa/Cocoa.h>

namespace oscil
{

void AnimationSettings::updateFromSystem()
{
    // Query macOS accessibility setting for reduced motion
    // Available since macOS 10.12 Sierra
    if (@available(macOS 10.12, *))
    {
        bool const systemPrefersReducedMotion = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
        appPrefersReducedMotion_.store(systemPrefersReducedMotion);
    }
}

}  // namespace oscil

#endif  // JUCE_MAC
