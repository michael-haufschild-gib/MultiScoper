cmake_minimum_required(VERSION 3.31)

# Enable C and CXX languages explicitly for JUCE
project(MultiScoper VERSION 1.0.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Auto-install pre-commit hook during configure (if .git exists and hook is missing)
if(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND EXISTS "${CMAKE_SOURCE_DIR}/scripts/pre-commit")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit")
        file(CREATE_LINK
            "${CMAKE_SOURCE_DIR}/scripts/pre-commit"
            "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit"
            SYMBOLIC
        )
        message(STATUS "Installed pre-commit hook (symlink to scripts/pre-commit)")
    endif()
endif()

# Windows-specific compile definitions
if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# ============================================================================
# Compiler Cache (ccache) - Speeds up rebuilds by caching compiled objects
# Supports Ninja and Makefile generators. Install: brew install ccache (macOS)
# ============================================================================
find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
    message(STATUS "Found ccache: ${CCACHE_EXECUTABLE}")
    # Only set if not already defined (allows override via -D or CMakePresets.json)
    if(NOT DEFINED CMAKE_C_COMPILER_LAUNCHER)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
    endif()
else()
    message(STATUS "ccache not found - builds will not be cached")
    message(STATUS "  Install: brew install ccache (macOS) | apt install ccache (Linux)")
endif()

# Build universal binary for macOS (Intel + Apple Silicon)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
endif()

# Copy built plugins to system plugin directories (requires write permissions)
# Disabled by default for dev builds where ~/Library permissions may block the copy.
option(MULTISCOPER_COPY_PLUGIN_AFTER_BUILD "Copy built plugins to system plugin directories" ON)

# Option for OpenGL support
option(MULTISCOPER_ENABLE_OPENGL "Enable OpenGL context for GPU compositing" ON)

# Option for RealtimeSanitizer (requires Clang 18+)
# Detects real-time safety violations: allocations, locks, syscalls in audio thread
option(MULTISCOPER_ENABLE_RTSAN "Enable RealtimeSanitizer for real-time safety checking" OFF)

# Option for libFuzzer targets (requires mainline LLVM Clang, Linux-only in CI).
# AppleClang does NOT ship a working libFuzzer runtime. See fuzz/CMakeLists.txt
# and docs/decisions/010-fuzz-targets.md for the full rationale.
option(MULTISCOPER_BUILD_FUZZERS "Build libFuzzer targets (Linux + Clang only)" OFF)

# Option for Clang-Tidy Static Analysis
# Catches performance issues, bugs, and modernization opportunities
option(MULTISCOPER_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

if(MULTISCOPER_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE
        NAMES clang-tidy
        HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin
        DOC "Path to clang-tidy executable"
    )

    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        # Checks and options are configured in .clang-tidy at project root.
        # WarningsAsErrors: '*' ensures all findings are errors.
        set(MULTISCOPER_CLANG_TIDY_CMD "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "MULTISCOPER_ENABLE_CLANG_TIDY is ON but clang-tidy executable was not found in default path or Homebrew locations")
    endif()
endif()

# Option for source-size linting (prevents god files/functions)
option(MULTISCOPER_ENABLE_SIZE_LIMIT_LINT "Enable max file/function line lint checks" ON)
set(MULTISCOPER_MAX_FILE_LINES "500" CACHE STRING "Maximum allowed lines per C/C++ file")
set(MULTISCOPER_MAX_FUNCTION_LINES "75" CACHE STRING "Maximum allowed lines per C/C++ function")
set(MULTISCOPER_SIZE_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/size_lint.py")

# Option for test quality linting (prevents shallow/vibecoded tests)
option(MULTISCOPER_ENABLE_TEST_QUALITY_LINT "Enable shallow test detection lint checks" ON)
set(MULTISCOPER_MIN_ASSERTIONS_PER_TEST "1" CACHE STRING "Minimum assertions per test function")
set(MULTISCOPER_TEST_QUALITY_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_quality_lint.py")

# Option for source registry linting (prevents orphan/missing files in Sources.cmake)
option(MULTISCOPER_ENABLE_SOURCE_REGISTRY_LINT "Enable source file registration lint checks" ON)
set(MULTISCOPER_SOURCE_REGISTRY_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/source_registry_lint.py")

# Option for TODO/placeholder linting (prevents deferred work markers)
option(MULTISCOPER_ENABLE_TODO_LINT "Enable TODO/FIXME/placeholder detection" ON)
set(MULTISCOPER_TODO_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/todo_lint.py")

# Option for architecture boundary linting (prevents layer violations)
option(MULTISCOPER_ENABLE_ARCHITECTURE_LINT "Enable include dependency boundary checks" ON)
set(MULTISCOPER_ARCHITECTURE_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/architecture_lint.py")

# Option for comment quality linting (prevents empty comments, enforces doc comments on public API)
option(MULTISCOPER_ENABLE_COMMENT_LINT "Enable comment quality lint checks" ON)
set(MULTISCOPER_COMMENT_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/comment_lint.py")

# Option for forbidden-patterns linting (ratchets against past bug reintroductions)
option(MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT "Enable forbidden-patterns lint checks" ON)
set(MULTISCOPER_FORBIDDEN_PATTERNS_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/forbidden_patterns_lint.py")

# Option for harness MT-dispatch capture linting (ADR-016: lambdas passed to
# runOnTrackSync / runOnMessageThreadBlocking must own their payload via
# shared_ptr captures, not reference captures).
option(MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT "Enable harness MT-dispatch capture lint (ADR-016)" ON)
set(MULTISCOPER_HARNESS_MT_CAPTURE_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/harness_mt_capture_lint.py")

# Option for clang-format linting (matches CI's clang-format --Werror step).
# Local ctest catches formatting drift before CI does.
option(MULTISCOPER_ENABLE_FORMAT_LINT "Enable clang-format lint check" ON)
set(MULTISCOPER_FORMAT_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/format_lint.py")

# Option for clang-tidy linting via ctest (matches CI's "Run clang-tidy" step).
# Disabled by default because clang-tidy runs are slow (~1 min on a typical
# dev box) — enable locally before pushing or in CI.
option(MULTISCOPER_ENABLE_CLANG_TIDY_LINT "Enable clang-tidy lint CTest" OFF)
set(MULTISCOPER_CLANG_TIDY_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/clang_tidy_lint.py")

if(MULTISCOPER_ENABLE_SIZE_LIMIT_LINT OR MULTISCOPER_ENABLE_TEST_QUALITY_LINT
   OR MULTISCOPER_ENABLE_SOURCE_REGISTRY_LINT OR MULTISCOPER_ENABLE_TODO_LINT
   OR MULTISCOPER_ENABLE_ARCHITECTURE_LINT OR MULTISCOPER_ENABLE_COMMENT_LINT
   OR MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT
   OR MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT
   OR MULTISCOPER_ENABLE_FORMAT_LINT
   OR MULTISCOPER_ENABLE_CLANG_TIDY_LINT)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

if(MULTISCOPER_ENABLE_SIZE_LIMIT_LINT)
    if(NOT EXISTS "${MULTISCOPER_SIZE_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_SIZE_LIMIT_LINT is ON but lint script was not found: ${MULTISCOPER_SIZE_LINT_SCRIPT}")
    endif()
endif()

if(MULTISCOPER_ENABLE_TEST_QUALITY_LINT)
    if(NOT EXISTS "${MULTISCOPER_TEST_QUALITY_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_TEST_QUALITY_LINT is ON but lint script was not found: ${MULTISCOPER_TEST_QUALITY_LINT_SCRIPT}")
    endif()
endif()

if(MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT)
    if(NOT EXISTS "${MULTISCOPER_FORBIDDEN_PATTERNS_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT is ON but lint script was not found: ${MULTISCOPER_FORBIDDEN_PATTERNS_LINT_SCRIPT}")
    endif()
endif()

if(MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT)
    if(NOT EXISTS "${MULTISCOPER_HARNESS_MT_CAPTURE_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT is ON but lint script was not found: ${MULTISCOPER_HARNESS_MT_CAPTURE_LINT_SCRIPT}")
    endif()
endif()

if(MULTISCOPER_ENABLE_FORMAT_LINT)
    if(NOT EXISTS "${MULTISCOPER_FORMAT_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_FORMAT_LINT is ON but lint script was not found: ${MULTISCOPER_FORMAT_LINT_SCRIPT}")
    endif()
    # clang-format is optional at configure time — the test is gated at
    # runtime with a helpful message. Check availability to let users
    # know up front that the test will skip if the tool is missing.
    find_program(CLANG_FORMAT_EXECUTABLE clang-format)
    if(NOT CLANG_FORMAT_EXECUTABLE)
        message(STATUS "clang-format not found on PATH — FormatLint will report FAILED at ctest time. Install: brew install clang-format (macOS) / apt-get install clang-format (Linux)")
    endif()
endif()

if(MULTISCOPER_ENABLE_CLANG_TIDY_LINT)
    if(NOT EXISTS "${MULTISCOPER_CLANG_TIDY_LINT_SCRIPT}")
        message(FATAL_ERROR "MULTISCOPER_ENABLE_CLANG_TIDY_LINT is ON but lint script was not found: ${MULTISCOPER_CLANG_TIDY_LINT_SCRIPT}")
    endif()
endif()

# Option for Code Coverage
option(MULTISCOPER_ENABLE_COVERAGE "Enable code coverage generation" OFF)

if(MULTISCOPER_ENABLE_COVERAGE)
    message(STATUS "Code Coverage Enabled")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Code coverage not supported for this compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Fetch CPM.cmake
include(cmake/CPM.cmake)

# JUCE
CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.12
    OPTIONS "JUCE_BUILD_EXAMPLES OFF" "JUCE_BUILD_EXTRAS OFF"
)

# cpp-httplib
CPMAddPackage(
    NAME httplib
    GITHUB_REPOSITORY yhirose/cpp-httplib
    GIT_TAG v0.38.0
)

# nlohmann/json
CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.12.0
)

# CLAP format support via clap-juce-extensions
# Pinned to a specific commit for reproducible builds.
# Tags (0.24.0-0.26.0) are incompatible with JUCE 8.0.12; main branch tracks JUCE 8.
CPMAddPackage(
    NAME clap-juce-extensions
    GITHUB_REPOSITORY free-audio/clap-juce-extensions
    GIT_TAG e1f67893cc409a40c1154fa2e78c97046da24ce0
)

# Include source file definitions
include(cmake/Sources.cmake)

# Define the plugin
juce_add_plugin(MultiScoper
    COMPANY_NAME "MultiScoper"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${MULTISCOPER_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Msco
    PLUGIN_CODE MSc1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "MultiScoper"
    BUNDLE_ID "com.multiscoper.MultiScoper"
    VST3_CATEGORIES "Fx" "Analyzer"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Enable CLAP format via clap-juce-extensions
clap_juce_extensions_plugin(
    TARGET MultiScoper
    CLAP_ID "com.multiscoper.MultiScoper"
    CLAP_FEATURES "audio-effect" "analyzer" "visualizer"
)

# Binary resources (SVG icons from icomoon)
juce_add_binary_data(MultiScoperBinaryData
    HEADER_NAME "BinaryData.h"
    NAMESPACE BinaryData
    SOURCES
        resources/icomoon/SVG/bin.svg
        resources/icomoon/SVG/cog.svg
        resources/icomoon/SVG/eye.svg
        resources/icomoon/SVG/eye-blocked.svg
        resources/icomoon/SVG/cross.svg
        resources/icomoon/SVG/checkmark.svg
        resources/icomoon/SVG/stats-bars.svg
        resources/icomoon/SVG/pause2.svg
        resources/icomoon/SVG/play3.svg
        resources/icomoon/SVG/redo.svg
        resources/shaders/basic.vert
        resources/shaders/basic.frag
        resources/shaders/neon_glow.vert
        resources/shaders/neon_glow.frag
        resources/shaders/dual_outline.vert
        resources/shaders/dual_outline.frag
        resources/shaders/gradient_fill.vert
        resources/shaders/gradient_fill.frag
        resources/fonts/PTRootUI-Light.ttf
        resources/fonts/PTRootUI-Regular.ttf
        resources/fonts/PTRootUI-Medium.ttf
        resources/fonts/PTRootUI-Bold.ttf
        resources/fonts/OFL.txt
)

# Source files
target_sources(MultiScoper PRIVATE ${MULTISCOPER_SOURCES})

# Test-server (HTTP automation endpoint inside the plugin). OFF for ship
# builds — a release binary must NOT start an HTTP server on port 9876.
# The standalone test harness builds this in via its own CMakeLists.
option(MULTISCOPER_ENABLE_TEST_SERVER
    "Compile the PluginTestServer into the plugin (Standalone HTTP API on port 9876). Disable for release." OFF)

if(MULTISCOPER_ENABLE_TEST_SERVER)
    target_sources(MultiScoper PRIVATE ${MULTISCOPER_TEST_SERVER_SOURCES})
    target_compile_definitions(MultiScoper PRIVATE MULTISCOPER_ENABLE_TEST_SERVER=1)
endif()

# Include directories
target_include_directories(MultiScoper
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(MultiScoper SYSTEM PRIVATE ${httplib_SOURCE_DIR})

# JUCE compile definitions
# Note: JUCE_DISPLAY_SPLASH_SCREEN removed - JUCE 8+ no longer uses splash screen
target_compile_definitions(MultiScoper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        $<$<BOOL:${MULTISCOPER_ENABLE_OPENGL}>:MULTISCOPER_ENABLE_OPENGL=1>
)

# Strict warnings — applied via interface library so only project targets link it.
# JUCE and other deps are not affected. JUCE headers use SYSTEM includes.
add_library(MultiScoperStrictWarnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(MultiScoperStrictWarnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(MultiScoperStrictWarnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
elseif(MSVC)
    target_compile_options(MultiScoperStrictWarnings INTERFACE /W4 /WX)
endif()

# Define interface library for warning suppressions to ensure they come last
add_library(MultiScoperWarningSuppressions INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(MultiScoperWarningSuppressions INTERFACE
        -Wno-unknown-warning-option
        -Wno-nontrivial-memcall
        -Wno-nontrivial-memaccess
        -Wno-switch-enum
    )
endif()

# Link JUCE modules
target_link_libraries(MultiScoper
    PRIVATE
        MultiScoperBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_dsp
        nlohmann_json::nlohmann_json
        $<$<BOOL:${MULTISCOPER_ENABLE_OPENGL}>:juce::juce_opengl>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        MultiScoperStrictWarnings      # -Werror for project code
        MultiScoperWarningSuppressions # Must be after JUCE warnings to override
)

# Enable Clang thread-safety analysis ONLY on InstanceRegistry.cpp.
# Applied per-source rather than globally because types not yet annotated
# with capability/guarded_by would produce false negatives if -Wthread-safety
# fired against them. Expand this list as more types adopt multiscoper::Mutex.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/InstanceRegistry.cpp
        PROPERTIES
            COMPILE_OPTIONS "-Wthread-safety;-Wthread-safety-analysis;-Werror=thread-safety"
    )
endif()

# Apply clang-tidy only to our source files (excluding JUCE)
if(MULTISCOPER_CLANG_TIDY_CMD)
    get_target_property(MULTISCOPER_ALL_SOURCES MultiScoper SOURCES)
    foreach(source ${MULTISCOPER_ALL_SOURCES})
        # Only check files in src/ (handles relative and absolute paths)
        if(source MATCHES "^src/" OR source MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/src/")
             set_source_files_properties(${source} PROPERTIES CXX_CLANG_TIDY "${MULTISCOPER_CLANG_TIDY_CMD}")
        endif()
    endforeach()
endif()

# Repo-wide source size linting target
if(MULTISCOPER_ENABLE_SIZE_LIMIT_LINT)
    add_custom_target(size_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_SIZE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --max-file-lines ${MULTISCOPER_MAX_FILE_LINES}
            --max-function-lines ${MULTISCOPER_MAX_FUNCTION_LINES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running source size lint checks"
        VERBATIM
    )
endif()

# Repo-wide test quality linting target
if(MULTISCOPER_ENABLE_TEST_QUALITY_LINT)
    add_custom_target(test_quality_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_TEST_QUALITY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths tests
            --min-assertions ${MULTISCOPER_MIN_ASSERTIONS_PER_TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running test quality lint checks"
        VERBATIM
    )
endif()

# Repo-wide source registry linting target
if(MULTISCOPER_ENABLE_SOURCE_REGISTRY_LINT)
    add_custom_target(source_registry_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_SOURCE_REGISTRY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running source registry lint checks"
        VERBATIM
    )
endif()

# Repo-wide TODO/placeholder linting target
if(MULTISCOPER_ENABLE_TODO_LINT)
    add_custom_target(todo_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_TODO_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running TODO/placeholder lint checks"
        VERBATIM
    )
endif()

# Repo-wide architecture boundary linting target
if(MULTISCOPER_ENABLE_ARCHITECTURE_LINT)
    add_custom_target(architecture_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_ARCHITECTURE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running architecture boundary lint checks"
        VERBATIM
    )
endif()

# Repo-wide comment quality linting target
if(MULTISCOPER_ENABLE_COMMENT_LINT)
    add_custom_target(comment_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_COMMENT_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --doc-paths include
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running comment quality lint checks"
        VERBATIM
    )
endif()

# Repo-wide forbidden-patterns linting target (ratchets past-bug regressions)
if(MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT)
    add_custom_target(forbidden_patterns_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_FORBIDDEN_PATTERNS_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running forbidden-patterns lint checks"
        VERBATIM
    )
endif()

# Repo-wide harness MT-dispatch capture linting target (ADR 016)
if(MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT)
    add_custom_target(harness_mt_capture_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_HARNESS_MT_CAPTURE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths test_harness/src test_harness/include
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running harness MT-dispatch capture lint checks"
        VERBATIM
    )
endif()

# Repo-wide clang-format linting target (matches CI format gate)
if(MULTISCOPER_ENABLE_FORMAT_LINT)
    add_custom_target(format_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_FORMAT_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format lint checks"
        VERBATIM
    )
endif()

# Unit tests for the lint SCRIPTS themselves (seed synthetic trees).
# Exposed as a convenience target so `cmake --build --target lint_scripts`
# runs them without going through ctest.
if(MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT OR MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT)
    add_custom_target(lint_scripts
        COMMAND ${Python3_EXECUTABLE} -m unittest discover
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/lint
        COMMENT "Running lint-script unit tests"
        VERBATIM
    )
endif()

# Repo-wide clang-tidy lint target (opt-in — slow).
if(MULTISCOPER_ENABLE_CLANG_TIDY_LINT)
    add_custom_target(clang_tidy_lint
        COMMAND ${Python3_EXECUTABLE} ${MULTISCOPER_CLANG_TIDY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --build-dir ${CMAKE_CURRENT_BINARY_DIR}
            --paths src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy on project sources"
        VERBATIM
    )
endif()

# Unified lint target — runs all enabled lint checks in one command
# Usage: cmake --build --preset dev --target lint
set(MULTISCOPER_LINT_DEPENDENCIES "")
if(MULTISCOPER_ENABLE_SIZE_LIMIT_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES size_lint)
endif()
if(MULTISCOPER_ENABLE_TEST_QUALITY_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES test_quality_lint)
endif()
if(MULTISCOPER_ENABLE_SOURCE_REGISTRY_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES source_registry_lint)
endif()
if(MULTISCOPER_ENABLE_TODO_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES todo_lint)
endif()
if(MULTISCOPER_ENABLE_ARCHITECTURE_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES architecture_lint)
endif()
if(MULTISCOPER_ENABLE_COMMENT_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES comment_lint)
endif()
if(MULTISCOPER_ENABLE_FORBIDDEN_PATTERNS_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES forbidden_patterns_lint)
endif()
if(MULTISCOPER_ENABLE_HARNESS_MT_CAPTURE_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES harness_mt_capture_lint)
endif()
if(MULTISCOPER_ENABLE_FORMAT_LINT)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES format_lint)
endif()
if(MULTISCOPER_ENABLE_CLANG_TIDY_LINT AND TARGET clang_tidy_lint)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES clang_tidy_lint)
endif()
if(TARGET lint_scripts)
    list(APPEND MULTISCOPER_LINT_DEPENDENCIES lint_scripts)
endif()
if(MULTISCOPER_LINT_DEPENDENCIES)
    add_custom_target(lint DEPENDS ${MULTISCOPER_LINT_DEPENDENCIES})
endif()

# Suppress warnings from JUCE third-party dependencies (Harfbuzz, VST3 SDK)
# Handled by MultiScoperWarningSuppressions interface library above

# RealtimeSanitizer flags (LLVM Clang 20+ only - not Apple Clang, not Ubuntu's packaged Clang)
# Catches allocations, mutex locks, and blocking syscalls in audio thread
# Note: RTSan is only used for testing (MultiScoperTests target), not for plugin builds.
# Plugin targets (VST3, AU, Standalone) are shared libraries that have complex
# linking requirements with RTSan that are not worth the effort to maintain.
if(MULTISCOPER_ENABLE_RTSAN)
    # Check for LLVM Clang (not AppleClang) version 20+
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20)
        message(STATUS "RealtimeSanitizer enabled (Clang ${CMAKE_CXX_COMPILER_VERSION})")
        # Compile with RTSan instrumentation for the shared code library
        target_compile_options(MultiScoper PRIVATE -fsanitize=realtime)
        target_link_options(MultiScoper PRIVATE -fsanitize=realtime)
    else()
        message(WARNING "RealtimeSanitizer requires LLVM Clang 20+. Current compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# Tests
option(MULTISCOPER_BUILD_TESTS "Build unit tests" ON)

if(MULTISCOPER_BUILD_TESTS)
    include(cmake/Tests.cmake)
endif()

# E2E Test Harness (optional)
option(MULTISCOPER_BUILD_TEST_HARNESS "Build E2E test harness application" OFF)

if(MULTISCOPER_BUILD_TEST_HARNESS)
    add_subdirectory(test_harness)
endif()

# libFuzzer targets (optional, Clang + Linux only — AppleClang is not supported).
# See fuzz/CMakeLists.txt and docs/decisions/010-fuzz-targets.md.
if(MULTISCOPER_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()

# Google Benchmark microbenchmarks (optional developer tool / CI artifact).
# NO regression gate — see docs/decisions/012-benchmarks-no-regression-gate.md.
option(MULTISCOPER_BUILD_BENCHMARKS "Build Google Benchmark microbenchmarks" OFF)

if(MULTISCOPER_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
