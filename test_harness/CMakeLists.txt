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
include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Fetch nlohmann/json for JSON handling (header-only)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

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
)

set(TEST_HARNESS_HEADERS
    include/TestAudioGenerator.h
    include/TestTransport.h
    include/TestTrack.h
    include/TestDAW.h
    include/TestHttpServer.h
    include/TestElementRegistry.h
    include/TestUIController.h
    include/TestScreenshot.h
    include/TestableComponent.h
)

# Plugin source files needed by the test harness
set(PLUGIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/InstanceRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SharedCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/OscilState.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Pane.cpp
    ${CMAKE_SOURCE_DIR}/src/core/WindowLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Source.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/SignalProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/TimingEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/WaveformComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/PaneComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ThemeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ThemeEditorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ColorPickerComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/StatusBarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SourceSelectorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SettingsDropdown.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SidebarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SourceItemComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/SegmentedButtonBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorListToolbar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorListItem.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorConfigPopup.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/TimingSidebarSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/MasterControlsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/TriggerSettingsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/DisplayOptionsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/SourceCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/ThemeCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/LayoutCoordinator.cpp
)

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
    ${PLUGIN_SOURCES}
)

# Include directories
target_include_directories(OscilTestHarness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${httplib_SOURCE_DIR}
)

# Link against JUCE and libraries
target_link_libraries(OscilTestHarness PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
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
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    TEST_HARNESS=1
)
