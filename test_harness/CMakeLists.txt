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
    ${CMAKE_SOURCE_DIR}/src/ui/components/SegmentedButtonBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorListToolbar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorListItem.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/OscillatorConfigPopup.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/AddOscillatorDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/TimingSidebarSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/OptionsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/SourceCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/ThemeCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/LayoutCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/PluginTestServer.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilAccordion.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilBadge.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilCheckbox.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilColorPicker.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilColorSwatches.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilDropdown.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilMeterBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilModal.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilRadioButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilSlider.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTabs.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTextField.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilToggle.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTooltip.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTransportSync.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/UIAudioFeedback.cpp
    # Rendering sources
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ShaderRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/BasicShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/NeonGlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/GradientFillShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DualOutlineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/PlasmaSineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DigitalGlitchShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformGLRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/VisualConfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RenderEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/FramebufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformRenderState.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Camera3D.cpp
    # Post-processing effects
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/PostProcessEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/VignetteEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/FilmGrainEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/BloomEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/TrailsEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/ColorGradeEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/ChromaticAberrationEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/ScanlineEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/DistortionEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/GlitchEffect.cpp
    # Particle system
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticlePool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleEmitter.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleSystem.cpp
    # 3D shaders
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WaveformShader3D.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VolumetricRibbonShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WireframeMeshShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VectorFlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/StringTheoryShader.cpp
    # Material shaders
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/MaterialShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/GlassRefractionShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/LiquidChromeShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/EnvironmentMapManager.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/CrystallineShader.cpp
    # UI managers
    ${CMAKE_SOURCE_DIR}/src/ui/managers/OpenGLLifecycleManager.cpp
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
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    TEST_HARNESS=1
    $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
)
