# =============================================================================
# Google Benchmark microbenchmarks for Oscil.
#
# Guarded by OSCIL_BUILD_BENCHMARKS (see top-level CMakeLists.txt). These
# benchmarks are a developer tool and CI artifact only — there is NO
# regression gate. See docs/decisions/012-benchmarks-no-regression-gate.md
# for the rationale (GitHub-hosted runner variance exceeds practical gate
# thresholds on tight microbenchmarks).
#
# Design mirrors fuzz/CMakeLists.txt: rather than factor a new library out
# of the main plugin target, filter OSCIL_SOURCES to the src/core/ subtree
# (the transitive closure of the benchmarked types: SharedCaptureBuffer,
# DecimatingCaptureBuffer, SignalProcessor, TimingEngine). No rendering/,
# ui/, plugin/, or tools/ sources are pulled in.
#
# Build type: benchmarks compile with -O2 regardless of the parent build
# type, so results are meaningful even when the surrounding tree is Debug.
# -g is kept for symbol quality in perf/Instruments traces.
# =============================================================================

if(NOT OSCIL_BUILD_BENCHMARKS)
    return()
endif()

# Google Benchmark v1.9.5 (latest tagged release as of 2026-01-21).
# ADR-006: prefer tagged releases for upstreams that publish stable tags.
CPMAddPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    GIT_TAG v1.9.5
    OPTIONS
        "BENCHMARK_ENABLE_TESTING OFF"
        "BENCHMARK_ENABLE_INSTALL OFF"
        "BENCHMARK_ENABLE_GTEST_TESTS OFF"
        "BENCHMARK_INSTALL_DOCS OFF"
)

# -----------------------------------------------------------------------------
# Filter OSCIL_SOURCES to just the src/core/ subtree. Populated by
# cmake/Sources.cmake which the top-level CMakeLists.txt includes before
# add_subdirectory(benchmarks), so OSCIL_SOURCES is already in scope here.
# -----------------------------------------------------------------------------
set(OSCIL_BENCH_CORE_SOURCES "")
foreach(_src ${OSCIL_SOURCES})
    if(_src MATCHES "/src/core/")
        list(APPEND OSCIL_BENCH_CORE_SOURCES "${_src}")
    endif()
endforeach()

if(NOT OSCIL_BENCH_CORE_SOURCES)
    message(FATAL_ERROR
        "OSCIL_BENCH_CORE_SOURCES is empty — expected OSCIL_SOURCES to contain "
        "src/core/**/*.cpp entries. Check that cmake/Sources.cmake is included "
        "before add_subdirectory(benchmarks).")
endif()

add_executable(OscilBenchmarks
    bench_main.cpp
    bench_shared_capture_buffer.cpp
    bench_decimating_capture_buffer.cpp
    bench_signal_processor.cpp
    bench_timing_engine.cpp
    bench_analysis.cpp
    ${OSCIL_BENCH_CORE_SOURCES}
)

target_include_directories(OscilBenchmarks PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# JUCE modules needed by the benchmarked types:
#   juce_core               — juce::String, logging
#   juce_data_structures    — juce::ValueTree (TimingEngine serialization)
#   juce_events             — juce::MessageManager (SignalProcessor guard)
#   juce_graphics           — juce::Colour (Oscillator.h)
#   juce_audio_basics       — juce::AudioBuffer
#   juce_dsp                — FIR::Filter (DecimationFilter)
#   juce_audio_processors   — juce::AudioPlayHead::PositionInfo (TimingEngine)
target_link_libraries(OscilBenchmarks PRIVATE
    benchmark::benchmark
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_audio_processors
    juce::juce_recommended_config_flags
)

# Force release-like optimization regardless of parent CMAKE_BUILD_TYPE. Keep
# -g for symbol quality in perf/Instruments captures. On MSVC, /O2 is the
# equivalent optimization level.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(OscilBenchmarks PRIVATE -O2 -g)
elseif(MSVC)
    target_compile_options(OscilBenchmarks PRIVATE /O2)
endif()

# JUCE headers expose a few warnings that are not actionable in benchmark
# code. Suppress the subset seen from juce_audio_processors on Clang/GCC.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(OscilBenchmarks PRIVATE
        -Wno-unknown-warning-option
        -Wno-nontrivial-memcall
        -Wno-nontrivial-memaccess
    )
endif()

message(STATUS "Benchmark target enabled: OscilBenchmarks (Google Benchmark v1.9.5)")
