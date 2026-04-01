cmake_minimum_required(VERSION 3.31)

# Enable C and CXX languages explicitly for JUCE
project(Oscil VERSION 1.0.0 LANGUAGES C CXX OBJCXX)

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
option(OSCIL_COPY_PLUGIN_AFTER_BUILD "Copy built plugins to system plugin directories" ON)

# Option for OpenGL support
option(OSCIL_ENABLE_OPENGL "Enable OpenGL context for GPU compositing" ON)

# Option for Melatonin Inspector (UI debugging tool)
# Disabled by default for CI/release builds - creates a separate window that can timeout in headless environments
option(OSCIL_ENABLE_INSPECTOR "Enable Melatonin Inspector for UI debugging (development only)" OFF)

# Option for RealtimeSanitizer (requires Clang 18+)
# Detects real-time safety violations: allocations, locks, syscalls in audio thread
option(OSCIL_ENABLE_RTSAN "Enable RealtimeSanitizer for real-time safety checking" OFF)

# Option for Clang-Tidy Static Analysis
# Catches performance issues, bugs, and modernization opportunities
option(OSCIL_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

if(OSCIL_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE
        NAMES clang-tidy
        HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin
        DOC "Path to clang-tidy executable"
    )

    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        # Checks and options are configured in .clang-tidy at project root.
        # WarningsAsErrors: '*' ensures all findings are errors.
        set(OSCIL_CLANG_TIDY_CMD "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "OSCIL_ENABLE_CLANG_TIDY is ON but clang-tidy executable was not found in default path or Homebrew locations")
    endif()
endif()

# Option for source-size linting (prevents god files/functions)
option(OSCIL_ENABLE_SIZE_LIMIT_LINT "Enable max file/function line lint checks" ON)
set(OSCIL_MAX_FILE_LINES "500" CACHE STRING "Maximum allowed lines per C/C++ file")
set(OSCIL_MAX_FUNCTION_LINES "60" CACHE STRING "Maximum allowed lines per C/C++ function")
set(OSCIL_SIZE_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/size_lint.py")

# Option for test quality linting (prevents shallow/vibecoded tests)
option(OSCIL_ENABLE_TEST_QUALITY_LINT "Enable shallow test detection lint checks" ON)
set(OSCIL_MIN_ASSERTIONS_PER_TEST "1" CACHE STRING "Minimum assertions per test function")
set(OSCIL_TEST_QUALITY_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_quality_lint.py")

# Option for source registry linting (prevents orphan/missing files in Sources.cmake)
option(OSCIL_ENABLE_SOURCE_REGISTRY_LINT "Enable source file registration lint checks" ON)
set(OSCIL_SOURCE_REGISTRY_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/source_registry_lint.py")

# Option for TODO/placeholder linting (prevents deferred work markers)
option(OSCIL_ENABLE_TODO_LINT "Enable TODO/FIXME/placeholder detection" ON)
set(OSCIL_TODO_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/todo_lint.py")

# Option for architecture boundary linting (prevents layer violations)
option(OSCIL_ENABLE_ARCHITECTURE_LINT "Enable include dependency boundary checks" ON)
set(OSCIL_ARCHITECTURE_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/architecture_lint.py")

# Option for comment quality linting (prevents empty comments, enforces doc comments on public API)
option(OSCIL_ENABLE_COMMENT_LINT "Enable comment quality lint checks" ON)
set(OSCIL_COMMENT_LINT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/comment_lint.py")

if(OSCIL_ENABLE_SIZE_LIMIT_LINT OR OSCIL_ENABLE_TEST_QUALITY_LINT
   OR OSCIL_ENABLE_SOURCE_REGISTRY_LINT OR OSCIL_ENABLE_TODO_LINT
   OR OSCIL_ENABLE_ARCHITECTURE_LINT OR OSCIL_ENABLE_COMMENT_LINT)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

if(OSCIL_ENABLE_SIZE_LIMIT_LINT)
    if(NOT EXISTS "${OSCIL_SIZE_LINT_SCRIPT}")
        message(FATAL_ERROR "OSCIL_ENABLE_SIZE_LIMIT_LINT is ON but lint script was not found: ${OSCIL_SIZE_LINT_SCRIPT}")
    endif()
endif()

if(OSCIL_ENABLE_TEST_QUALITY_LINT)
    if(NOT EXISTS "${OSCIL_TEST_QUALITY_LINT_SCRIPT}")
        message(FATAL_ERROR "OSCIL_ENABLE_TEST_QUALITY_LINT is ON but lint script was not found: ${OSCIL_TEST_QUALITY_LINT_SCRIPT}")
    endif()
endif()

# Option for Code Coverage
option(OSCIL_ENABLE_COVERAGE "Enable code coverage generation" OFF)

if(OSCIL_ENABLE_COVERAGE)
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

# Melatonin Inspector for UI debugging (browser-like devtools for JUCE)
# Only fetch/build when explicitly enabled
if(OSCIL_ENABLE_INSPECTOR)
    CPMAddPackage(
        NAME melatonin_inspector
        GITHUB_REPOSITORY sudara/melatonin_inspector
        GIT_TAG origin/main
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector
    )
endif()

# Include source file definitions
include(cmake/Sources.cmake)

# Define the plugin
juce_add_plugin(Oscil
    COMPANY_NAME "OscilAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${OSCIL_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Osci
    PLUGIN_CODE Osc1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "oscil4"
    BUNDLE_ID "com.oscilaudio.oscil4"
    VST3_CATEGORIES "Fx" "Analyzer"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Enable CLAP format via clap-juce-extensions
clap_juce_extensions_plugin(
    TARGET Oscil
    CLAP_ID "com.oscilaudio.oscil4"
    CLAP_FEATURES "audio-effect" "analyzer" "visualizer"
)

# Binary resources (SVG icons from icomoon)
juce_add_binary_data(OscilBinaryData
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
)

# Source files
target_sources(Oscil PRIVATE ${OSCIL_SOURCES})

# Include directories
target_include_directories(Oscil
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(Oscil SYSTEM PRIVATE ${httplib_SOURCE_DIR})

# JUCE compile definitions
# Note: JUCE_DISPLAY_SPLASH_SCREEN removed - JUCE 8+ no longer uses splash screen
target_compile_definitions(Oscil
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
        $<$<BOOL:${OSCIL_ENABLE_INSPECTOR}>:OSCIL_ENABLE_INSPECTOR=1>
)

# Strict warnings — applied via interface library so only project targets link it.
# JUCE and other deps are not affected. JUCE headers use SYSTEM includes.
add_library(OscilStrictWarnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(OscilStrictWarnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(OscilStrictWarnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
elseif(MSVC)
    target_compile_options(OscilStrictWarnings INTERFACE /W4 /WX)
endif()

# Define interface library for warning suppressions to ensure they come last
add_library(OscilWarningSuppressions INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(OscilWarningSuppressions INTERFACE
        -Wno-unknown-warning-option
        -Wno-nontrivial-memcall
        -Wno-nontrivial-memaccess
        -Wno-switch-enum
    )
endif()

# Link JUCE modules
target_link_libraries(Oscil
    PRIVATE
        OscilBinaryData
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
        $<$<BOOL:${OSCIL_ENABLE_INSPECTOR}>:melatonin_inspector>
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        OscilStrictWarnings      # -Werror for project code
        OscilWarningSuppressions # Must be after JUCE warnings to override
)

# Apply clang-tidy only to our source files (excluding JUCE)
if(OSCIL_CLANG_TIDY_CMD)
    get_target_property(OSCIL_ALL_SOURCES Oscil SOURCES)
    foreach(source ${OSCIL_ALL_SOURCES})
        # Only check files in src/ (handles relative and absolute paths)
        if(source MATCHES "^src/" OR source MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/src/")
             set_source_files_properties(${source} PROPERTIES CXX_CLANG_TIDY "${OSCIL_CLANG_TIDY_CMD}")
        endif()
    endforeach()
endif()

# Repo-wide source size linting target
if(OSCIL_ENABLE_SIZE_LIMIT_LINT)
    add_custom_target(size_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_SIZE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --max-file-lines ${OSCIL_MAX_FILE_LINES}
            --max-function-lines ${OSCIL_MAX_FUNCTION_LINES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running source size lint checks"
        VERBATIM
    )
endif()

# Repo-wide test quality linting target
if(OSCIL_ENABLE_TEST_QUALITY_LINT)
    add_custom_target(test_quality_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_TEST_QUALITY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths tests
            --min-assertions ${OSCIL_MIN_ASSERTIONS_PER_TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running test quality lint checks"
        VERBATIM
    )
endif()

# Repo-wide source registry linting target
if(OSCIL_ENABLE_SOURCE_REGISTRY_LINT)
    add_custom_target(source_registry_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_SOURCE_REGISTRY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running source registry lint checks"
        VERBATIM
    )
endif()

# Repo-wide TODO/placeholder linting target
if(OSCIL_ENABLE_TODO_LINT)
    add_custom_target(todo_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_TODO_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running TODO/placeholder lint checks"
        VERBATIM
    )
endif()

# Repo-wide architecture boundary linting target
if(OSCIL_ENABLE_ARCHITECTURE_LINT)
    add_custom_target(architecture_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_ARCHITECTURE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running architecture boundary lint checks"
        VERBATIM
    )
endif()

# Repo-wide comment quality linting target
if(OSCIL_ENABLE_COMMENT_LINT)
    add_custom_target(comment_lint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_COMMENT_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --doc-paths include
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running comment quality lint checks"
        VERBATIM
    )
endif()

# Unified lint target — runs all enabled lint checks in one command
# Usage: cmake --build --preset dev --target lint
set(OSCIL_LINT_DEPENDENCIES "")
if(OSCIL_ENABLE_SIZE_LIMIT_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES size_lint)
endif()
if(OSCIL_ENABLE_TEST_QUALITY_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES test_quality_lint)
endif()
if(OSCIL_ENABLE_SOURCE_REGISTRY_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES source_registry_lint)
endif()
if(OSCIL_ENABLE_TODO_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES todo_lint)
endif()
if(OSCIL_ENABLE_ARCHITECTURE_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES architecture_lint)
endif()
if(OSCIL_ENABLE_COMMENT_LINT)
    list(APPEND OSCIL_LINT_DEPENDENCIES comment_lint)
endif()
if(OSCIL_LINT_DEPENDENCIES)
    add_custom_target(lint DEPENDS ${OSCIL_LINT_DEPENDENCIES})
endif()

# Suppress warnings from JUCE third-party dependencies (Harfbuzz, VST3 SDK)
# Handled by OscilWarningSuppressions interface library above

# RealtimeSanitizer flags (LLVM Clang 20+ only - not Apple Clang, not Ubuntu's packaged Clang)
# Catches allocations, mutex locks, and blocking syscalls in audio thread
# Note: RTSan is only used for testing (OscilTests target), not for plugin builds.
# Plugin targets (VST3, AU, Standalone) are shared libraries that have complex
# linking requirements with RTSan that are not worth the effort to maintain.
if(OSCIL_ENABLE_RTSAN)
    # Check for LLVM Clang (not AppleClang) version 20+
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20)
        message(STATUS "RealtimeSanitizer enabled (Clang ${CMAKE_CXX_COMPILER_VERSION})")
        # Compile with RTSan instrumentation for the shared code library
        target_compile_options(Oscil PRIVATE -fsanitize=realtime)
        target_link_options(Oscil PRIVATE -fsanitize=realtime)
    else()
        message(WARNING "RealtimeSanitizer requires LLVM Clang 20+. Current compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# Tests
option(OSCIL_BUILD_TESTS "Build unit tests" ON)

if(OSCIL_BUILD_TESTS)
    include(cmake/Tests.cmake)
endif()

# E2E Test Harness (optional)
option(OSCIL_BUILD_TEST_HARNESS "Build E2E test harness application" OFF)

if(OSCIL_BUILD_TEST_HARNESS)
    add_subdirectory(test_harness)
endif()
