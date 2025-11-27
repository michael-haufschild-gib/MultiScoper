cmake_minimum_required(VERSION 3.20)

# Enable C and CXX languages explicitly for JUCE
project(Oscil VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build universal binary for macOS (Intel + Apple Silicon)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
endif()

# Option for OpenGL support
option(OSCIL_ENABLE_OPENGL "Enable OpenGL context for GPU compositing" ON)

# Fetch JUCE
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE httplib json)

# Define the plugin
juce_add_plugin(Oscil
    COMPANY_NAME "OscilAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Osci
    PLUGIN_CODE Osc1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "oscil4"
    BUNDLE_ID "com.oscilaudio.oscil4"
    VST3_CATEGORIES "Fx" "Analyzer"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Source files
target_sources(Oscil
    PRIVATE
        # Core
        src/core/PluginProcessor.cpp
        src/core/InstanceRegistry.cpp
        src/core/SharedCaptureBuffer.cpp
        src/core/OscilState.cpp
        src/core/Oscillator.cpp
        src/core/Pane.cpp
        src/core/WindowLayout.cpp
        src/core/Source.cpp

        # DSP
        src/dsp/SignalProcessor.cpp
        src/dsp/TimingEngine.cpp

        # UI
        src/ui/PluginEditor.cpp
        src/ui/WaveformComponent.cpp
        src/ui/OscillatorPanel.cpp
        src/ui/PaneComponent.cpp
        src/ui/ThemeManager.cpp
        src/ui/ThemeEditorComponent.cpp
        src/ui/ColorPickerComponent.cpp
        src/ui/StatusBarComponent.cpp
        src/ui/SourceSelectorComponent.cpp
        src/ui/SettingsDropdown.cpp
        src/ui/SidebarComponent.cpp
        src/ui/SourceItemComponent.cpp
        src/ui/SegmentedButtonBar.cpp
        src/ui/OscillatorListToolbar.cpp
        src/ui/OscillatorListItem.cpp
        src/ui/OscillatorConfigPopup.cpp
        src/ui/PluginTestServer.cpp
        # UI Sections
        src/ui/sections/TimingSidebarSection.cpp
        src/ui/sections/MasterControlsSection.cpp
        src/ui/sections/TriggerSettingsSection.cpp
        src/ui/sections/DisplayOptionsSection.cpp
        # UI Coordinators
        src/ui/coordinators/SourceCoordinator.cpp
        src/ui/coordinators/ThemeCoordinator.cpp
        src/ui/coordinators/LayoutCoordinator.cpp
)

# Include directories
target_include_directories(Oscil
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${httplib_SOURCE_DIR}
)

# JUCE compile definitions
target_compile_definitions(Oscil
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
)

# Link JUCE modules
target_link_libraries(Oscil
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        nlohmann_json::nlohmann_json
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Tests
option(OSCIL_BUILD_TESTS "Build unit tests" ON)

if(OSCIL_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(OscilTests
        tests/test_main.cpp
        tests/test_signal_processor.cpp
        tests/test_instance_registry.cpp
        tests/test_shared_capture_buffer.cpp
        tests/test_oscillator.cpp
        tests/test_theme_manager.cpp
        tests/test_state_persistence.cpp
        tests/test_source.cpp
        tests/test_timing_config.cpp

        # Include necessary source files for testing
        src/core/InstanceRegistry.cpp
        src/core/SharedCaptureBuffer.cpp
        src/core/OscilState.cpp
        src/core/Oscillator.cpp
        src/core/Pane.cpp
        src/core/WindowLayout.cpp
        src/core/Source.cpp
        src/dsp/SignalProcessor.cpp
        src/ui/ThemeManager.cpp
    )

    target_include_directories(OscilTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(OscilTests
        PRIVATE
            GTest::gtest
            GTest::gtest_main
            juce::juce_core
            juce::juce_data_structures
            juce::juce_graphics
            juce::juce_events
            juce::juce_audio_basics
    )

    target_compile_definitions(OscilTests
        PRIVATE
            JUCE_STANDALONE_APPLICATION=1
            JUCE_USE_CURL=0
            JUCE_WEB_BROWSER=0
    )

    include(GoogleTest)
    gtest_discover_tests(OscilTests)
endif()

# E2E Test Harness (optional)
option(OSCIL_BUILD_TEST_HARNESS "Build E2E test harness application" OFF)

if(OSCIL_BUILD_TEST_HARNESS)
    add_subdirectory(test_harness)
endif()
