# =============================================================================
# libFuzzer targets for Oscil.
#
# Guarded by OSCIL_BUILD_FUZZERS (see top-level CMakeLists.txt). Clang-only:
# AppleClang does not ship a working libFuzzer runtime, and MSVC's
# /fsanitize=fuzzer is out of scope here. Supported toolchain: mainline LLVM
# Clang >= 14 on Linux (see .github/workflows/fuzz.yml for the reference
# environment).
#
# Design: rather than factor a new OscilLib INTERFACE library out of the main
# CMakeLists.txt (which would touch the juce_add_plugin target and its
# warning/suppression wiring), we scope the fuzz link closure to the minimum
# needed by each fuzz target:
#
#   * fuzz_oscil_state_fromxml  -> compiles every src/core/**/*.cpp source
#     listed in OSCIL_SOURCES. That is exactly what OscilState::fromXmlString
#     depends on transitively (Oscillator, Pane, Source, SchemaMigration,
#     AnalysisEngine, etc.). No rendering/, ui/, plugin/, or tools/ sources
#     are pulled in, so the fuzzer does not drag in OpenGL/X11/gui_basics.
#
#   * fuzz_plugin_test_server_state -> links only nlohmann_json. The real
#     handlers are editor/processor-coupled and cannot be exercised without
#     a full JUCE GUI app context, so we fuzz the JSON parse entry point
#     that every handler calls (nlohmann::json::parse on req.body) and
#     confirm no UB on adversarial input.
#
# Each binary takes the corpus directory as its first positional argument
# (libFuzzer convention).
# =============================================================================

if(NOT OSCIL_BUILD_FUZZERS)
    return()
endif()

# AppleClang's compiler-id is literally "AppleClang". STREQUAL "Clang"
# excludes it intentionally — its libFuzzer runtime is not functional.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "OSCIL_BUILD_FUZZERS requires mainline LLVM Clang (>= 14). "
        "Current compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
        "AppleClang does NOT ship a working libFuzzer runtime. "
        "On macOS install LLVM via Homebrew and point CMAKE_CXX_COMPILER "
        "at /opt/homebrew/opt/llvm/bin/clang++.")
endif()

# Common fuzzer flags.
# -fsanitize=fuzzer,address,undefined enables the libFuzzer driver plus ASan
# (use-after-free, heap overflow) and UBSan (signed overflow, null deref,
# alignment). -g / -O1 keep symbols for crash triage while still letting the
# fuzzer make forward progress.
set(OSCIL_FUZZ_FLAGS
    -fsanitize=fuzzer,address,undefined
    -fno-omit-frame-pointer
    -g
    -O1
)

# -----------------------------------------------------------------------------
# Fuzzer 1: OscilState::fromXmlString
# -----------------------------------------------------------------------------
# Filter OSCIL_SOURCES to just the src/core/ subtree. This is populated by
# cmake/Sources.cmake which the top-level CMakeLists.txt includes before
# add_subdirectory(fuzz), so OSCIL_SOURCES is already in scope here.
set(OSCIL_FUZZ_CORE_SOURCES "")
foreach(_src ${OSCIL_SOURCES})
    if(_src MATCHES "/src/core/")
        list(APPEND OSCIL_FUZZ_CORE_SOURCES "${_src}")
    endif()
endforeach()

if(NOT OSCIL_FUZZ_CORE_SOURCES)
    message(FATAL_ERROR
        "OSCIL_FUZZ_CORE_SOURCES is empty — expected OSCIL_SOURCES to contain "
        "src/core/**/*.cpp entries. Check that cmake/Sources.cmake is included "
        "before add_subdirectory(fuzz).")
endif()

add_executable(fuzz_oscil_state_fromxml
    fuzz_oscil_state_fromxml.cpp
    ${OSCIL_FUZZ_CORE_SOURCES}
)

target_include_directories(fuzz_oscil_state_fromxml PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# JUCE modules needed by the core/ subtree:
#   juce_core             — juce::String, juce::XmlDocument, juce::Logger
#   juce_data_structures  — juce::ValueTree, juce::Identifier
#   juce_events           — message-manager symbols referenced by juce_core
#   juce_graphics         — juce::Colour (pulled by Oscillator.h)
#   juce_audio_basics     — juce::AudioBuffer (analysis/AnalysisEngine.h)
#   juce_dsp              — DSP primitives referenced from core/dsp headers
target_link_libraries(fuzz_oscil_state_fromxml PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_options(fuzz_oscil_state_fromxml PRIVATE ${OSCIL_FUZZ_FLAGS})
target_link_options(fuzz_oscil_state_fromxml PRIVATE ${OSCIL_FUZZ_FLAGS})

# -----------------------------------------------------------------------------
# Fuzzer 2: Plugin test-server JSON parse path
# -----------------------------------------------------------------------------
# The real StateHandler/OscillatorHandler/SourceHandler validators are
# editor-coupled and cannot be fuzzed without a full JUCE GUI host. The common
# attack surface every handler shares is nlohmann::json::parse(req.body) — that
# is what we fuzz here. Goal: confirm no UB (ASan/UBSan clean) on adversarial
# JSON.
add_executable(fuzz_plugin_test_server_state
    fuzz_plugin_test_server_state.cpp
)

target_link_libraries(fuzz_plugin_test_server_state PRIVATE
    nlohmann_json::nlohmann_json
)

target_compile_options(fuzz_plugin_test_server_state PRIVATE ${OSCIL_FUZZ_FLAGS})
target_link_options(fuzz_plugin_test_server_state PRIVATE ${OSCIL_FUZZ_FLAGS})

message(STATUS "Fuzz targets enabled: fuzz_oscil_state_fromxml, fuzz_plugin_test_server_state")
