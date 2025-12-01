# Oscil Source Files
# Shared between main plugin target and test harness

set(OSCIL_SOURCES
    # Core
    ${CMAKE_SOURCE_DIR}/src/core/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/InstanceRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SharedCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/OscilState.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Pane.cpp
    ${CMAKE_SOURCE_DIR}/src/core/WindowLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Source.cpp

    # DSP
    ${CMAKE_SOURCE_DIR}/src/dsp/SignalProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/TimingEngine.cpp

    # UI
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
    ${CMAKE_SOURCE_DIR}/src/ui/PluginTestServer.cpp
    # UI Sections
    ${CMAKE_SOURCE_DIR}/src/ui/sections/TimingSidebarSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/sections/OptionsSection.cpp
    # UI Coordinators
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/SourceCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/ThemeCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/coordinators/LayoutCoordinator.cpp

    # UI Managers
    ${CMAKE_SOURCE_DIR}/src/ui/managers/OpenGLLifecycleManager.cpp

    # UI Components (New Component Library)
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTextField.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilToggle.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTooltip.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilSlider.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilCheckbox.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilRadioButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilDropdown.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTabs.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilAccordion.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilBadge.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilModal.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilColorSwatches.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilColorPicker.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilMeterBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTransportSync.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/UIAudioFeedback.cpp

    # Rendering (Shaders)
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ShaderRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/BasicShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/NeonGlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/GradientFillShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DualOutlineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/PlasmaSineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DigitalGlitchShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformGLRenderer.cpp

    # Rendering (Render Engine)
    ${CMAKE_SOURCE_DIR}/src/rendering/Framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/FramebufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RenderEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/VisualConfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformRenderState.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Camera3D.cpp

    # Rendering (Post-Processing Effects)
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
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/RadialBlurEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/LensFlareEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/TiltShiftEffect.cpp

    # Rendering (Particles)
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticlePool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleEmitter.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleSystem.cpp

    # Rendering (3D Shaders)
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WaveformShader3D.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VolumetricRibbonShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WireframeMeshShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VectorFlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/StringTheoryShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/ElectricFlowerShader.cpp

    # Rendering (Materials)
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/MaterialShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/EnvironmentMapManager.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/GlassRefractionShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/LiquidChromeShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/CrystallineShader.cpp
)

# Add platform-specific sources
if(APPLE)
    list(APPEND OSCIL_SOURCES
        ${CMAKE_SOURCE_DIR}/src/ui/components/AnimationSettings.mm
    )
endif()
