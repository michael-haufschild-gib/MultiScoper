# Oscil4 E2E Test Harness
# Standalone application for autonomous testing of the plugin
# Must be included from the main CMakeLists.txt as a subdirectory

cmake_minimum_required(VERSION 3.22)

# Build only for native architecture (not universal) to avoid linker issues
# with libraries that don't have x86_64 versions
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
endif()

# Fetch cpp-httplib for HTTP server (header-only)
CPMAddPackage(
    NAME httplib
    GITHUB_REPOSITORY yhirose/cpp-httplib
    GIT_TAG v0.28.0
)

# Fetch nlohmann/json for JSON handling (header-only)
CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
)

# Test harness sources
set(TEST_HARNESS_SOURCES
    src/Main.cpp
    src/TestAudioGenerator.cpp
    src/TestTransport.cpp
    src/TestTrack.cpp
    src/TestDAW.cpp
    src/TestHttpServer.cpp
    src/TestElementRegistry.cpp
    src/TestUIController.cpp
    src/TestScreenshot.cpp
    src/TestMetrics.cpp
)

# Add platform-specific sources
if(APPLE)
    list(APPEND TEST_HARNESS_SOURCES src/TestScreenshotMac.mm)
endif()

set(TEST_HARNESS_HEADERS
    include/TestAudioGenerator.h
    include/TestTransport.h
    include/TestTrack.h
    include/TestDAW.h
    include/TestHttpServer.h
    include/TestElementRegistry.h
    include/TestUIController.h
    include/TestScreenshot.h
    include/TestMetrics.h
    include/TestableComponent.h
)

# Include shared source definitions
include(${CMAKE_SOURCE_DIR}/cmake/Sources.cmake)

# Create JUCE standalone application
juce_add_gui_app(OscilTestHarness
    PRODUCT_NAME "Oscil Test Harness"
    COMPANY_NAME "OscilAudio"
    BUNDLE_ID "com.oscilaudio.testharness"
)

# Add sources to the target
target_sources(OscilTestHarness PRIVATE
    ${TEST_HARNESS_SOURCES}
    ${TEST_HARNESS_HEADERS}
    ${OSCIL_SOURCES}
)

# Include directories
target_include_directories(OscilTestHarness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${httplib_SOURCE_DIR}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(OscilTestHarness PRIVATE -Wno-switch-enum)
endif()

# Link against JUCE and libraries
target_link_libraries(OscilTestHarness PRIVATE
    OscilBinaryData
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    httplib::httplib
    nlohmann_json::nlohmann_json
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(OscilTestHarness PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    TEST_HARNESS=1
    $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
)
