# MultiScoper Source Files
# Shared between main plugin target and test harness

set(MULTISCOPER_SOURCES
    # Plugin (entry points)
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginProcessorState.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginEditorSidebarHandlers.cpp
    ${CMAKE_SOURCE_DIR}/src/plugin/PluginFactory.cpp

    # Core (business logic)
    ${CMAKE_SOURCE_DIR}/src/core/InstanceRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SharedCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SharedCaptureBufferAnalysis.cpp
    ${CMAKE_SOURCE_DIR}/src/core/DecimatingCaptureBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/DecimatingCaptureBufferQueries.cpp
    ${CMAKE_SOURCE_DIR}/src/core/MemoryBudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/MemoryBudgetManagerQueries.cpp
    ${CMAKE_SOURCE_DIR}/src/core/MultiScoperState.cpp
    ${CMAKE_SOURCE_DIR}/src/core/MultiScoperStateLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SchemaMigration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/GlobalPreferences.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Source.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Oscillator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Pane.cpp
    ${CMAKE_SOURCE_DIR}/src/core/PaneLayoutManager.cpp

    # Core DSP
    ${CMAKE_SOURCE_DIR}/src/core/dsp/SignalProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/dsp/TimingEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/dsp/TimingEngineSerialization.cpp

    # Core Analysis
    ${CMAKE_SOURCE_DIR}/src/core/analysis/AnalysisEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/analysis/TransientDetector.cpp

    # Rendering (core)
    ${CMAKE_SOURCE_DIR}/src/rendering/EffectChain.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Framebuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/FramebufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/GridRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RenderEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RenderEngineWaveforms.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ShaderRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformGLRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformGLRendererPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformRenderState.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/WaveformShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/VisualConfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/VisualConfigurationSerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/PresetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/RenderBootstrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/EffectPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/subsystems/WaveformPass.cpp


    # Rendering (shaders)
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/BasicShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/NeonGlowShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/GradientFillShader.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/shaders/DualOutlineShader.cpp

    # Rendering (effects)
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/PostProcessEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/SingleShaderEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/VignetteEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/FilmGrainEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/BloomEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/BloomEffectShaders.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/TrailsEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/ColorGradeEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/ChromaticAberrationEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/RadialBlurEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/TiltShiftEffect.cpp

    # UI (components)
    ${CMAKE_SOURCE_DIR}/src/ui/components/SurfaceStyle.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/SurfacePainter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/ThemedComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperButtonPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperTextField.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperTextFieldPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperToggle.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperSlider.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperSliderPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperSliderInteraction.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MagneticSnapController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperCheckbox.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperRadioButton.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperRadioButtonPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperRadioButtonInteraction.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperDropdown.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperDropdownInteraction.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperDropdownPopup.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperDropdownPopupEvents.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperTabs.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperTabsPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperAccordion.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperAccordionSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperBadge.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/SectionHeader.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperModal.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperModalAlert.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperModalPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperColorSwatches.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperColorPicker.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperColorPickerCalc.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/MultiScoperMeterBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/SegmentedButtonBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/UIAudioFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/PaneSelectorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/components/InlineEditLabel.cpp

    # UI (panels)
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListComponentEvents.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListItem.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/OscillatorListItemPainting.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/OscillatorConfigDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/StatusBarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SourceSelectorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SourceListItem.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/SourceItemComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/WaveformComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panels/WaveformComponentRendering.cpp
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
    ${CMAKE_SOURCE_DIR}/src/ui/controllers/GpuRenderCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/controllers/OpenGLLifecycleManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/controllers/OscillatorPanelController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/controllers/OscillatorPanelControllerHandlers.cpp

    # UI (layout)
    ${CMAKE_SOURCE_DIR}/src/ui/layout/WindowLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PaneComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PaneContainerComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/PluginEditorLayout.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/SidebarComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/SidebarComponentHandlers.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/sections/TimingSidebarSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/sections/OptionsSection.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/layout/sections/OscillatorSidebarSection.cpp
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
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeManagerSerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/SystemThemes.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeEditorComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeEditorWidgets.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeEditorActions.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ColorPickerComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/MultiScoperLookAndFeel.cpp

)

# Test-server sources — separated so production plugin builds can omit them.
# These expose an HTTP server from inside the plugin when loaded as
# Standalone; shipping a release binary that opens port 9876 is a security
# and supply-chain liability.  The MultiScoperTestHarness target always compiles
# them in; the main MultiScoper plugin only does so when MULTISCOPER_ENABLE_TEST_SERVER
# is ON (default OFF).
set(MULTISCOPER_TEST_SERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/PluginTestServer.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/LayoutHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/OscillatorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/OscillatorHandlerTests.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/SourceHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/ScreenshotHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/TestRunnerHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/TestRunnerHandlerDragDrop.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/TestRunnerHandlerWaveformSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/WaveformHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/StateHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/test_server/TestWaveformGenerator.cpp
)

# Add platform-specific sources
if(APPLE)
    list(APPEND MULTISCOPER_SOURCES
        ${CMAKE_SOURCE_DIR}/src/platform/macos/AnimationSettings.mm
    )
endif()
