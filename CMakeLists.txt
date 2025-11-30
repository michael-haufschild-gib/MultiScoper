cmake_minimum_required(VERSION 3.20)

# Enable C and CXX languages explicitly for JUCE
project(Oscil VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
        # We exclude 'modernize-use-trailing-return-type' as it often conflicts with JUCE style
        set(OSCIL_CLANG_TIDY_CMD "${CLANG_TIDY_EXE};-checks=bugprone-*,performance-*,modernize-*,-modernize-use-trailing-return-type")
    else()
        message(WARNING "OSCIL_ENABLE_CLANG_TIDY is ON but clang-tidy executable was not found in default path or Homebrew locations")
    endif()
endif()

# Fetch CPM.cmake
include(cmake/CPM.cmake)

# JUCE
CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.5
    OPTIONS "JUCE_BUILD_EXAMPLES OFF" "JUCE_BUILD_EXTRAS OFF"
)

# cpp-httplib
CPMAddPackage(
    NAME httplib
    GITHUB_REPOSITORY yhirose/cpp-httplib
    GIT_TAG v0.28.0
)

# nlohmann/json
CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
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

# Binary resources (SVG icons from icomoon)
juce_add_binary_data(OscilBinaryData
    HEADER_NAME "BinaryData.h"
    NAMESPACE BinaryData
    SOURCES
        resources/icomoon/SVG/bin.svg
        resources/icomoon/SVG/cog.svg
        resources/icomoon/SVG/eye.svg
        resources/icomoon/SVG/eye-blocked.svg
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
        src/ui/components/SegmentedButtonBar.cpp
        src/ui/OscillatorListToolbar.cpp
        src/ui/OscillatorListItem.cpp
        src/ui/OscillatorConfigPopup.cpp
        src/ui/AddOscillatorDialog.cpp
        src/ui/PluginTestServer.cpp
        # UI Sections
        src/ui/sections/TimingSidebarSection.cpp
        src/ui/sections/OptionsSection.cpp
        # UI Coordinators
        src/ui/coordinators/SourceCoordinator.cpp
        src/ui/coordinators/ThemeCoordinator.cpp
        src/ui/coordinators/LayoutCoordinator.cpp

        # UI Managers
        src/ui/managers/OpenGLLifecycleManager.cpp

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

        # Rendering (Shaders)
        src/rendering/WaveformShader.cpp
        src/rendering/ShaderRegistry.cpp
        src/rendering/shaders/BasicShader.cpp
        src/rendering/shaders/NeonGlowShader.cpp
        src/rendering/shaders/GradientFillShader.cpp
        src/rendering/shaders/DualOutlineShader.cpp
        src/rendering/shaders/PlasmaSineShader.cpp
        src/rendering/shaders/DigitalGlitchShader.cpp
        src/rendering/WaveformGLRenderer.cpp

        # Rendering (Render Engine)
        src/rendering/Framebuffer.cpp
        src/rendering/FramebufferPool.cpp
        src/rendering/RenderEngine.cpp
        src/rendering/VisualConfiguration.cpp
        src/rendering/WaveformRenderState.cpp
        src/rendering/Camera3D.cpp

        # Rendering (Post-Processing Effects)
        src/rendering/effects/PostProcessEffect.cpp
        src/rendering/effects/VignetteEffect.cpp
        src/rendering/effects/FilmGrainEffect.cpp
        src/rendering/effects/BloomEffect.cpp
        src/rendering/effects/TrailsEffect.cpp
        src/rendering/effects/ColorGradeEffect.cpp
        src/rendering/effects/ChromaticAberrationEffect.cpp
        src/rendering/effects/ScanlineEffect.cpp
        src/rendering/effects/DistortionEffect.cpp
        src/rendering/effects/GlitchEffect.cpp

        # Rendering (Particles)
        src/rendering/particles/ParticlePool.cpp
        src/rendering/particles/ParticleEmitter.cpp
        src/rendering/particles/ParticleSystem.cpp

        # Rendering (3D Shaders)
        src/rendering/shaders3d/WaveformShader3D.cpp
        src/rendering/shaders3d/VolumetricRibbonShader.cpp
        src/rendering/shaders3d/WireframeMeshShader.cpp
        src/rendering/shaders3d/VectorFlowShader.cpp
        src/rendering/shaders3d/StringTheoryShader.cpp

        # Rendering (Materials)
        src/rendering/materials/MaterialShader.cpp
        src/rendering/materials/EnvironmentMapManager.cpp
        src/rendering/materials/GlassRefractionShader.cpp
        src/rendering/materials/LiquidChromeShader.cpp
        src/rendering/materials/CrystallineShader.cpp
)

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
        nlohmann_json::nlohmann_json
        $<$<BOOL:${OSCIL_ENABLE_INSPECTOR}>:melatonin_inspector>
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
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

# Suppress warnings from JUCE third-party dependencies (Harfbuzz, VST3 SDK)
# These are internal JUCE libraries we cannot modify
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # First suppress unknown warning option warnings, then use both flag variants:
    # - LLVM Clang 20+ uses -Wno-nontrivial-memcall
    # - Apple Clang uses -Wno-nontrivial-memaccess
    target_compile_options(Oscil PRIVATE
        -Wno-unknown-warning-option  # Suppress warnings about unrecognized flags
        -Wno-nontrivial-memcall      # LLVM Clang: Harfbuzz memcpy warnings
        -Wno-nontrivial-memaccess    # Apple Clang: Harfbuzz memcpy warnings
    )
endif()

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
    enable_testing()

    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
    )

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
        tests/test_timing_engine.cpp
        tests/test_plugin_processor.cpp

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
        tests/test_timing_calculations.cpp
        tests/test_add_oscillator_dialog.cpp
        tests/test_accordion_sections_e2e.cpp
        tests/test_sidebar_theme_population.cpp
        tests/test_shaders.cpp

        # New tests - 100% coverage initiative
        tests/test_window_layout.cpp
        tests/test_coordinators.cpp
        tests/test_camera3d.cpp
        tests/test_visual_configuration.cpp

        # Updated UI Component tests
        tests/test_oscil_button.cpp
        tests/test_oscil_toggle.cpp
        tests/test_oscil_dropdown.cpp
        tests/test_oscil_accordion.cpp
        tests/test_oscil_badge.cpp
        tests/test_oscil_checkbox.cpp
        tests/test_oscil_colorpicker.cpp
        tests/test_oscil_colorswatches.cpp
        tests/test_oscil_meterbar.cpp
        tests/test_oscil_modal.cpp
        tests/test_oscil_radiobutton.cpp
        tests/test_oscil_slider.cpp
        tests/test_oscil_tabs.cpp
        tests/test_oscil_textfield.cpp
        tests/test_oscil_transportsync.cpp

        # Include necessary source files for testing
        src/core/InstanceRegistry.cpp
        src/core/SharedCaptureBuffer.cpp
        src/core/OscilState.cpp
        src/core/Oscillator.cpp
        src/core/Pane.cpp
        src/core/WindowLayout.cpp
        src/core/Source.cpp
        src/dsp/SignalProcessor.cpp
        src/dsp/TimingEngine.cpp
        src/core/PluginProcessor.cpp
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
        src/ui/components/SegmentedButtonBar.cpp
        src/ui/OscillatorListToolbar.cpp
        src/ui/OscillatorListItem.cpp
        src/ui/OscillatorConfigPopup.cpp
        src/ui/AddOscillatorDialog.cpp
        src/ui/PluginTestServer.cpp
        src/ui/sections/TimingSidebarSection.cpp
        src/ui/sections/OptionsSection.cpp
        src/ui/coordinators/SourceCoordinator.cpp
        src/ui/coordinators/ThemeCoordinator.cpp
        src/ui/coordinators/LayoutCoordinator.cpp
        src/ui/managers/OpenGLLifecycleManager.cpp

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

        # Rendering source files for testing
        src/rendering/WaveformShader.cpp
        src/rendering/ShaderRegistry.cpp
        src/rendering/shaders/BasicShader.cpp
        src/rendering/shaders/NeonGlowShader.cpp
        src/rendering/shaders/GradientFillShader.cpp
        src/rendering/shaders/DualOutlineShader.cpp
        src/rendering/shaders/PlasmaSineShader.cpp
        src/rendering/shaders/DigitalGlitchShader.cpp
        src/rendering/WaveformGLRenderer.cpp

        # Rendering (Render Engine) for testing
        src/rendering/Framebuffer.cpp
        src/rendering/FramebufferPool.cpp
        src/rendering/RenderEngine.cpp
        src/rendering/VisualConfiguration.cpp
        src/rendering/WaveformRenderState.cpp
        src/rendering/Camera3D.cpp

        # Rendering (Post-Processing Effects) for testing
        src/rendering/effects/PostProcessEffect.cpp
        src/rendering/effects/VignetteEffect.cpp
        src/rendering/effects/FilmGrainEffect.cpp
        src/rendering/effects/BloomEffect.cpp
        src/rendering/effects/TrailsEffect.cpp
        src/rendering/effects/ColorGradeEffect.cpp
        src/rendering/effects/ChromaticAberrationEffect.cpp
        src/rendering/effects/ScanlineEffect.cpp
        src/rendering/effects/DistortionEffect.cpp
        src/rendering/effects/GlitchEffect.cpp

        # Rendering (Particles) for testing
        src/rendering/particles/ParticlePool.cpp
        src/rendering/particles/ParticleEmitter.cpp
        src/rendering/particles/ParticleSystem.cpp

        # Rendering (3D Shaders) for testing
        src/rendering/shaders3d/WaveformShader3D.cpp
        src/rendering/shaders3d/VolumetricRibbonShader.cpp
        src/rendering/shaders3d/WireframeMeshShader.cpp
        src/rendering/shaders3d/VectorFlowShader.cpp
        src/rendering/shaders3d/StringTheoryShader.cpp

        # Rendering (Materials) for testing
        src/rendering/materials/MaterialShader.cpp
        src/rendering/materials/EnvironmentMapManager.cpp
        src/rendering/materials/GlassRefractionShader.cpp
        src/rendering/materials/LiquidChromeShader.cpp
        src/rendering/materials/CrystallineShader.cpp
    )

    target_include_directories(OscilTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${httplib_SOURCE_DIR}
    )

    target_link_libraries(OscilTests
        PRIVATE
            OscilBinaryData
            GTest::gtest_main  # gtest_main already includes gtest
            juce::juce_core
            juce::juce_data_structures
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_events
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_processors
            juce::juce_audio_utils
            nlohmann_json::nlohmann_json
            $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
    )

    # Apply clang-tidy to test sources
    if(OSCIL_CLANG_TIDY_CMD)
        get_target_property(TEST_SOURCES OscilTests SOURCES)
        foreach(source ${TEST_SOURCES})
            if(source MATCHES "^src/" OR source MATCHES "^tests/" OR source MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/src/" OR source MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/tests/")
                 set_source_files_properties(${source} PROPERTIES CXX_CLANG_TIDY "${OSCIL_CLANG_TIDY_CMD}")
            endif()
        endforeach()
    endif()

    target_compile_definitions(OscilTests
        PRIVATE
            JUCE_STANDALONE_APPLICATION=1
            JUCE_USE_CURL=0
            JUCE_WEB_BROWSER=0
            $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
    )

    # Apply RTSan to tests (tests are executables, not shared libraries)
    if(OSCIL_ENABLE_RTSAN AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20)
        target_compile_options(OscilTests PRIVATE -fsanitize=realtime)
        target_link_options(OscilTests PRIVATE -fsanitize=realtime)
    endif()

    include(GoogleTest)
    gtest_discover_tests(OscilTests)
endif()

# E2E Test Harness (optional)
option(OSCIL_BUILD_TEST_HARNESS "Build E2E test harness application" OFF)

if(OSCIL_BUILD_TEST_HARNESS)
    add_subdirectory(test_harness)
endif()
