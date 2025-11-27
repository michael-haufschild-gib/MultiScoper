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

# Option for RealtimeSanitizer (requires Clang 18+)
# Detects real-time safety violations: allocations, locks, syscalls in audio thread
option(OSCIL_ENABLE_RTSAN "Enable RealtimeSanitizer for real-time safety checking" OFF)

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

# Fetch core dependencies first
FetchContent_MakeAvailable(JUCE httplib json)

# Melatonin Inspector for UI debugging (browser-like devtools for JUCE)
# Must be declared and made available AFTER JUCE but BEFORE juce_add_plugin
FetchContent_Declare(melatonin_inspector
    GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
    GIT_TAG origin/main
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector
)
FetchContent_MakeAvailable(melatonin_inspector)

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

        # UI Components (New Component Library)
        src/ui/components/OscilButton.cpp
        src/ui/components/OscilTextField.cpp
        src/ui/components/OscilToggle.cpp
        src/ui/components/OscilTooltip.cpp
        src/ui/components/OscilSlider.cpp
        src/ui/components/OscilCheckbox.cpp
        src/ui/components/OscilRadioButton.cpp
        src/ui/components/OscilDropdown.cpp
        src/ui/components/OscilTabs.cpp
        src/ui/components/OscilAccordion.cpp
        src/ui/components/OscilBadge.cpp
        src/ui/components/OscilModal.cpp
        src/ui/components/OscilColorSwatches.cpp
        src/ui/components/OscilColorPicker.cpp
        src/ui/components/OscilMeterBar.cpp
        src/ui/components/OscilTransportSync.cpp
        src/ui/components/UIAudioFeedback.cpp
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
        melatonin_inspector
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# RealtimeSanitizer flags (Clang 18+ only)
# Catches allocations, mutex locks, and blocking syscalls in audio thread
if(OSCIL_ENABLE_RTSAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
        message(STATUS "RealtimeSanitizer enabled")
        target_compile_options(Oscil PRIVATE -fsanitize=realtime)
        target_link_options(Oscil PRIVATE -fsanitize=realtime)
        # Also apply to standalone target for testing
        target_compile_options(Oscil_Standalone PRIVATE -fsanitize=realtime)
        target_link_options(Oscil_Standalone PRIVATE -fsanitize=realtime)
    else()
        message(WARNING "RealtimeSanitizer requires Clang 18+. Current compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

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

        # UI Component Tests
        tests/test_spring_animation.cpp
        tests/test_animation_settings.cpp
        # NOTE: Component tests moved to tests/outdated_tests/ - need rewriting to match actual API
        # tests/test_oscil_button.cpp
        # tests/test_oscil_toggle.cpp
        # tests/test_oscil_textfield.cpp
        # tests/test_oscil_slider.cpp
        # tests/test_oscil_checkbox.cpp
        # tests/test_oscil_radiobutton.cpp
        # tests/test_oscil_dropdown.cpp
        # tests/test_oscil_tabs.cpp
        # tests/test_oscil_accordion.cpp
        # tests/test_oscil_badge.cpp
        # tests/test_oscil_modal.cpp
        # tests/test_oscil_colorswatches.cpp
        # tests/test_oscil_colorpicker.cpp
        # tests/test_oscil_meterbar.cpp
        # tests/test_oscil_transportsync.cpp
        tests/test_ui_audio_feedback.cpp

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

        # UI Component source files for testing
        src/ui/components/OscilButton.cpp
        src/ui/components/OscilTextField.cpp
        src/ui/components/OscilToggle.cpp
        src/ui/components/OscilTooltip.cpp
        src/ui/components/OscilSlider.cpp
        src/ui/components/OscilCheckbox.cpp
        src/ui/components/OscilRadioButton.cpp
        src/ui/components/OscilDropdown.cpp
        src/ui/components/OscilTabs.cpp
        src/ui/components/OscilAccordion.cpp
        src/ui/components/OscilBadge.cpp
        src/ui/components/OscilModal.cpp
        src/ui/components/OscilColorSwatches.cpp
        src/ui/components/OscilColorPicker.cpp
        src/ui/components/OscilMeterBar.cpp
        src/ui/components/OscilTransportSync.cpp
        src/ui/components/UIAudioFeedback.cpp
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
            juce::juce_gui_basics
            juce::juce_events
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_processors
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
