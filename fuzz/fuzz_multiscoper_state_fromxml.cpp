// =============================================================================
// libFuzzer target: multiscoper::MultiScoperState::fromXmlString
//
// Entry point for the DAW-persisted plugin state loader. JUCE's
// juce::XmlDocument::parse + juce::ValueTree::fromXml are fed untrusted bytes
// that came from a project file, so the loader must treat every input as
// hostile. This fuzzer drives the public API with random byte sequences and
// asserts that fromXmlString never crashes (ASan / UBSan clean, no assertion
// trips, no infinite loops).
//
// Seeds: fuzz/corpus/multiscoper_state/*.xml
// Runtime: libFuzzer driver (-fsanitize=fuzzer,address,undefined).
// =============================================================================

#include "core/MultiScoperState.h"

#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Guard juce::String::fromUTF8 against negative size cast. libFuzzer hands
    // us size_t, JUCE takes int — clamp to INT_MAX so we never UB on the cast.
    if (size > static_cast<size_t>(INT_MAX))
    {
        return 0;
    }

    const auto xml = juce::String::fromUTF8(reinterpret_cast<const char*>(data), static_cast<int>(size));

    multiscoper::MultiScoperState state;
    // Return value intentionally discarded: a `false` return on malformed
    // input is the desired behavior. We only care that the call completes
    // without crashing, asserting, or corrupting memory.
    (void) state.fromXmlString(xml);

    return 0;
}
