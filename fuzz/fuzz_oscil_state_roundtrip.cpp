// =============================================================================
// libFuzzer target: oscil::OscilState XML round-trip preservation.
//
// For any input that fromXmlString accepts, the value produced by the
// subsequent toXmlString call MUST itself be accepted by fromXmlString, and
// the oscillator count must match across the round-trip. This catches two
// classes of bug:
//
//   (1) toXmlString producing XML that fromXmlString cannot parse — a silent
//       corruption of DAW project state across save/reload cycles.
//   (2) fromXmlString accepting a blob but dropping or duplicating oscillators
//       during the reserialize step — project-state drift.
//
// Seeds: fuzz/corpus/oscil_state/*.xml (shared with fuzz_oscil_state_fromxml).
// Runtime: libFuzzer driver (-fsanitize=fuzzer,address,undefined).
// =============================================================================

#include "core/OscilState.h"

#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Guard against negative size cast to int in juce::String::fromUTF8.
    if (size > static_cast<size_t>(INT_MAX))
    {
        return 0;
    }

    const auto xml = juce::String::fromUTF8(reinterpret_cast<const char*>(data), static_cast<int>(size));

    oscil::OscilState first;
    if (!first.fromXmlString(xml))
    {
        // Rejected input — not interesting for round-trip testing.
        return 0;
    }

    // Snapshot oscillator count before reserialization.
    const int countBefore = first.getOscillatorCount();

    // Re-serialize. This must produce well-formed XML.
    const juce::String serialized = first.toXmlString();

    // Second parse — MUST succeed. A failure here means our own serializer
    // emitted XML we ourselves cannot read back.
    oscil::OscilState second;
    if (!second.fromXmlString(serialized))
    {
        std::abort();
    }

    // Oscillator count must be preserved across the round-trip.
    const int countAfter = second.getOscillatorCount();
    if (countBefore != countAfter)
    {
        std::abort();
    }

    // Re-serializing the already-parsed value must produce byte-identical
    // XML. A weaker count-only check would pass even if the round-trip reset
    // oscillator parameters, pane layout, or source state; stability of the
    // serialized string proves the full state survived the cycle.
    const juce::String reserialized = second.toXmlString();
    if (reserialized != serialized)
    {
        std::abort();
    }

    return 0;
}
