// =============================================================================
// libFuzzer target: multiscoper::MultiScoperState XML round-trip preservation.
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
// Seeds: fuzz/corpus/multiscoper_state/*.xml (shared with fuzz_multiscoper_state_fromxml).
// Runtime: libFuzzer driver (-fsanitize=fuzzer,address,undefined).
// =============================================================================

#include "core/MultiScoperState.h"

#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace multiscoper
{
// Core fuzz driver. Lives in the project namespace per repo conventions;
// LLVMFuzzerTestOneInput below is a thin extern "C" ABI shim that forwards
// to this helper.
static int fuzzMultiScoperStateRoundtrip(const uint8_t* data, size_t size)
{
    // Guard against negative size cast to int in juce::String::fromUTF8.
    if (size > static_cast<size_t>(INT_MAX))
    {
        return 0;
    }

    const auto xml = juce::String::fromUTF8(reinterpret_cast<const char*>(data), static_cast<int>(size));

    multiscoper::MultiScoperState first;
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
    multiscoper::MultiScoperState second;
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

    // Re-serializing the already-parsed value must preserve the *semantic*
    // XML tree. A byte-for-byte comparison of the two serialized strings
    // would false-positive: JUCE's XmlElement::toString normalizes
    // whitespace, line wrapping, and attribute spacing per TextFormat, so
    // two logically identical serializations can still differ as strings.
    // Use XmlElement::isEquivalentTo so structure, attributes, and values
    // are checked while format noise is ignored.
    const juce::String reserialized = second.toXmlString();

    const auto firstTree = juce::parseXML(serialized);
    const auto secondTree = juce::parseXML(reserialized);
    if (firstTree == nullptr || secondTree == nullptr)
    {
        // Our own serializer produced XML that can't be re-parsed as XML at
        // all — that's a serializer bug, abort so the fuzzer captures it.
        std::abort();
    }
    if (!firstTree->isEquivalentTo(secondTree.get(), /*ignoreOrderOfAttributes=*/true))
    {
        std::abort();
    }

    return 0;
}
} // namespace multiscoper

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    return multiscoper::fuzzMultiScoperStateRoundtrip(data, size);
}
