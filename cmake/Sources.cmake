# Oscil Source Files
# Shared between main plugin target and test harness

set(OSCIL_SOURCES
    # Plugin (entry points)
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/OpenGLLifecycleManager.cpp

    # Core (business logic)
    ${CMAKE_SOURCE_DIR}/src/core/InstanceRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SharedCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/DecimatingCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/MemoryBudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/OscilState.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Source.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Pane.cpp

    # Core DSP
    ${CMAKE_SOURCE_DIR}/src/core/dsp/SignalProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/dsp/TimingEngine.cpp

    # Core Analysis
    ${CMAKE_SOURCE_DIR}/src/core/analysis/AnalysisEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/analysis/TransientDetector.cpp

    # Rendering (core)
    ${CMAKE_SOURCE_DIR}/src/rendering/EffectChain.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/FramebufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/GridRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ParticleRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RenderEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ShaderRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformGLRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformRenderState.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/VisualConfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Camera3D.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/GpuRenderCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/RenderBootstrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/EffectPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/WaveformPass.cpp


    # Rendering (shaders)
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/BasicShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/NeonGlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/GradientFillShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DualOutlineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/PlasmaSineShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DigitalGlitchShader.cpp

    # Rendering (shaders 3D)
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WaveformShader3D.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VolumetricRibbonShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/WireframeMeshShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/VectorFlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/StringTheoryShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/ElectricFlowerShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders3d/ElectricFiligreeShader.cpp

    # Rendering (materials)
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/MaterialShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/EnvironmentMapManager.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/TextureManager.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/GlassRefractionShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/LiquidChromeShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/materials/CrystallineShader.cpp

    # Rendering (effects)
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
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/TiltShiftEffect.cpp

    # Rendering (particles)
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticlePool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleEmitter.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/particles/ParticleSystem.cpp

    # UI (components)
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilTextField.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilToggle.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilSlider.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MagneticSnapController.cpp
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
    ${CMAKE_SOURCE_DIR}/src/ui/components/SegmentedButtonBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/UIAudioFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/PaneSelectorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/InlineEditLabel.cpp

    # UI (panels)
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListToolbar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListItem.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/OscillatorConfigDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/StatusBarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SourceSelectorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SourceItemComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/WaveformComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/WaveformPresenter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SoftwareGridRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/TimingPresenter.cpp

    # UI (dialogs)
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/AddOscillatorDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/OscillatorColorDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/SelectPaneDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/managers/DialogManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/managers/DisplaySettingsManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/managers/PerformanceMetricsController.cpp

    # UI (controllers)
    ${CMAKE_SOURCE_DIR}/src/ui/controllers/OscillatorPanelController.cpp

    # UI (layout)
    ${CMAKE_SOURCE_DIR}/src/ui/layout/WindowLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PaneComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PaneContainerComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PluginEditorLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/SidebarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/sections/TimingSidebarSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/sections/OptionsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/LayoutCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/SourceCoordinator.cpp

    # UI (layout/pane)
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/PaneActionBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/PaneHeader.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/PaneBody.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/WaveformStack.cpp

    # UI (layout/pane/overlays)
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/overlays/PaneOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/overlays/StatsOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/pane/overlays/CrosshairOverlay.cpp

    # UI (theme)
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeEditorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ColorPickerComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeCoordinator.cpp

    # Tools (test server)
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/PluginTestServer.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/LayoutHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/OscillatorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/SourceHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/ScreenshotHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/TestRunnerHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/WaveformHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/StateHandler.cpp
)

# Add platform-specific sources
if(APPLE)
    list(APPEND OSCIL_SOURCES
        ${CMAKE_SOURCE_DIR}/src/platform/macos/AnimationSettings.mm
    )
endif()
