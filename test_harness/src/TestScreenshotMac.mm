/*
    Oscil Test Harness - macOS Native Screenshot Capture
    Uses macOS screencapture command to capture full window including OpenGL
*/

#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#include <cstdlib>
#include <unistd.h>

namespace oscil::test
{

// Forward declarations for native window capture functions
juce::Image captureNativeWindowMac(juce::Component* component, const juce::String& outputPath);
juce::Image captureNativeWindowMac(juce::Component* component);

juce::Image captureNativeWindowMac(juce::Component* component, const juce::String& outputPath)
{
    if (component == nullptr)
        return {};

    // Get the native window handle
    auto* peer = component->getPeer();
    if (peer == nullptr)
        return {};

    void* nativeHandle = peer->getNativeHandle();
    if (nativeHandle == nullptr)
        return {};

    NSView* view = (__bridge NSView*)nativeHandle;
    NSWindow* window = [view window];
    if (window == nil)
        return {};

    // Get the window ID
    CGWindowID windowID = (CGWindowID)[window windowNumber];

    // Use screencapture command with window ID
    // -l<windowid> captures specific window
    // -o removes shadow
    // -x no sound
    juce::String tempPath = outputPath.isEmpty()
        ? juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("oscil_screenshot_temp.png").getFullPathName()
        : outputPath;

    juce::String command = juce::String::formatted(
        "screencapture -l%u -o -x \"%s\"",
        (unsigned int)windowID,
        tempPath.toRawUTF8()
    );

    int result = std::system(command.toRawUTF8());

    if (result != 0)
    {
        DBG("screencapture command failed with code: " << result);
        return {};
    }

    // Give the system a moment to write the file
    usleep(100000); // 100ms

    // Load the captured image
    juce::File imageFile(tempPath);
    if (!imageFile.existsAsFile())
    {
        DBG("Screenshot file was not created: " << tempPath);
        return {};
    }

    juce::Image capturedImage = juce::ImageFileFormat::loadFrom(imageFile);

    // Clean up temp file if we created one
    if (outputPath.isEmpty())
        imageFile.deleteFile();

    return capturedImage;
}

// Overload for backward compatibility - just capture without saving
juce::Image captureNativeWindowMac(juce::Component* component)
{
    return captureNativeWindowMac(component, juce::String());
}

} // namespace oscil::test

#endif // JUCE_MAC
