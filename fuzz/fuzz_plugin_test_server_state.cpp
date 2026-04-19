// =============================================================================
// libFuzzer target: PluginTestServer JSON parse path
//
// Every HTTP handler in src/tools/test_server/ calls nlohmann::json::parse on
// req.body before touching any editor/processor state. Even though the server
// binds only to 127.0.0.1, the JSON input is still untrusted (any process on
// the loopback interface can reach it, and end-to-end harness suites pass
// fixtures authored outside the plugin). The real handlers are coupled to a
// live juce::AudioProcessorEditor and cannot be constructed without a full
// JUCE GUI host; exercising them here would require bringing up the message
// manager and a MultiScoper editor, which is out of scope for a stateless fuzz
// target.
//
// We therefore fuzz the common attack surface: nlohmann::json::parse itself.
// A crash here would affect every handler; a clean run demonstrates the JSON
// pre-parse is UB-safe on arbitrary byte sequences. Subsequent work can grow
// this target to exercise handler-specific validators (e.g. oscillator-id
// whitelists) once those are extracted into a headless, testable form.
//
// Seeds: fuzz/corpus/test_server_state/*.json
// Runtime: libFuzzer driver (-fsanitize=fuzzer,address,undefined).
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Replicate the handler contract: body may be empty; handlers fall back to
    // "{}" in that case (see SourceHandler.cpp:58). Do the same here so the
    // fuzzer exercises the same branch the production code takes.
    const std::string body = size == 0 ? std::string{"{}"} : std::string(reinterpret_cast<const char*>(data), size);

    try
    {
        // parse(..., nullptr, /*allow_exceptions=*/true) — we swallow the
        // exception ourselves to match the try/catch pattern in the real
        // handlers. The goal is to confirm parse never triggers ASan/UBSan
        // findings on adversarial input, regardless of whether it throws.
        auto parsed = nlohmann::json::parse(body);

        // Touch a few common fields the handlers actually read, so the fuzzer
        // is rewarded for finding inputs that exercise member access. value()
        // with a default makes this safe on any JSON shape.
        (void) parsed.value("id", std::string{});
        (void) parsed.value("name", std::string{});
        (void) parsed.value("paneId", 0);
        (void) parsed.is_object();
        (void) parsed.dump();
    }
    catch (const nlohmann::json::exception&)
    {
        // Expected on malformed input. Production handlers also catch here.
    }

    return 0;
}
