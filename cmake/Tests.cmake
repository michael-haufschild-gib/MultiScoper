# Oscil Test Configuration
# Defines the OscilTests executable, test sources, lint test targets,
# and test harness build option.

enable_testing()

CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.17.0
)

# Test source files
set(OSCIL_TEST_SOURCES
    # Signal processor tests
    tests/test_signal_processor.cpp
    tests/test_signal_processor_edge.cpp
    tests/test_signal_processor_stereo_mode.cpp

    # Core entity tests - split for better isolation
    tests/test_source_id.cpp
    tests/test_source_state.cpp
    tests/test_source_ownership.cpp
    tests/test_source_buffer.cpp
    tests/test_instance_registry_crud.cpp
    tests/test_instance_registry_sync.cpp
    tests/test_instance_registry_lifecycle.cpp
    tests/test_plugin_processor_lifecycle.cpp
    tests/test_plugin_processor_state.cpp
    tests/test_plugin_processor_audio.cpp

    # Capture buffer tests - split for better isolation
    tests/test_capture_buffer_core.cpp
    tests/test_capture_buffer_metadata.cpp
    tests/test_capture_buffer_threading.cpp
    tests/test_capture_buffer_concurrent.cpp
    tests/test_capture_buffer_integrity.cpp
    tests/test_capture_buffer_lifecycle.cpp

    # Oscillator and pane tests
    tests/test_pane_drag_state.cpp
    tests/test_oscillator_core.cpp
    tests/test_oscillator_dsp.cpp
    tests/test_pane.cpp
    tests/test_theme_manager_crud.cpp
    tests/test_theme_manager_apply.cpp
    tests/test_theme_manager_persistence.cpp

    # State persistence tests - split for better isolation
    tests/test_state_persistence_save.cpp
    tests/test_state_persistence_load.cpp
    tests/test_state_persistence_migration.cpp
    tests/test_state_persistence_edge.cpp

    # Timing tests
    tests/test_timing_config.cpp
    tests/test_timing_config_edge.cpp
    tests/test_timing_config_serialization.cpp
    tests/test_timing_config_validation.cpp
    tests/test_timing_engine.cpp
    tests/test_timing_engine_host.cpp
    tests/test_timing_engine_host_edge.cpp
    tests/test_timing_engine_serialization.cpp
    tests/test_timing_engine_serialization_edge.cpp
    tests/test_timing_engine_listeners.cpp

    # Analysis tests
    tests/test_analysis_engine.cpp
    tests/test_transient_detector.cpp

    # Rendering and config tests
    tests/test_stats_overlay.cpp
    tests/test_capture_quality_config.cpp
    tests/test_capture_quality_config_edge.cpp
    tests/test_decimating_capture_buffer.cpp
    tests/test_decimating_capture_buffer_edge.cpp
    tests/test_decimating_capture_buffer_concurrent.cpp
    tests/test_memory_budget_manager.cpp
    tests/test_memory_budget_manager_edge.cpp

    # UI component tests - split for better isolation
    tests/test_spring_physics.cpp
    tests/test_spring_interaction.cpp
    tests/test_spring_edge.cpp
    tests/test_animation_settings_validation.cpp
    tests/test_animation_settings_presets.cpp
    tests/test_animation_settings_persistence.cpp

    # UI behavior tests
    tests/test_ui_audio_feedback.cpp
    tests/test_timing_calculations.cpp
    tests/test_add_oscillator_dialog.cpp
    tests/test_accordion_sections_e2e.cpp
    tests/test_sidebar_theme_population.cpp
    tests/test_sidebar_component.cpp
    tests/test_preset_manager.cpp
    tests/test_shaders.cpp
    tests/test_effect_chain.cpp
    tests/test_framebuffer_pool.cpp
    tests/test_waveform_presenter.cpp
    tests/test_timing_presenter.cpp
    tests/test_pane_layout.cpp
    tests/test_pane_layout_edge.cpp
    tests/test_pane_body.cpp
    tests/test_pane_action_bar.cpp
    tests/test_pane_header.cpp
    tests/test_config_structs.cpp

    # Coverage initiative tests
    tests/test_window_layout.cpp
    tests/test_coordinator_lifecycle.cpp
    tests/test_coordinator_events.cpp
    tests/test_coordinator_state.cpp
    tests/test_visual_config_validation.cpp
    tests/test_visual_config_application.cpp
    tests/test_visual_config_presets.cpp
    tests/test_plugin_editor.cpp

    # UI component widget tests
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
    tests/test_oscillator_list_component.cpp
    tests/test_oscillator_color_dialog.cpp
    tests/test_pane_closing_bug.cpp
    tests/test_pane_selector.cpp
    tests/test_select_pane_dialog.cpp
    tests/test_oscillator_panel_controller.cpp

    # Concurrency primitives
    tests/test_seqlock.cpp

    # UI logic controllers
    tests/test_magnetic_snap_controller.cpp

    # OscilState unit tests
    tests/test_oscil_state.cpp

    # Integration tests
    tests/test_state_integration.cpp
    tests/test_processor_integration.cpp

    # Global preferences
    tests/test_global_preferences.cpp

    # Plugin processor edge cases
    tests/test_plugin_processor_edge.cpp

    # Instance registry stress tests
    tests/test_instance_registry_stress.cpp

    # Performance benchmarks
    tests/test_performance_benchmarks.cpp
)

add_executable(OscilTests
    tests/test_main.cpp
    ${OSCIL_TEST_SOURCES}

    # Test Harness Dependencies
    test_harness/src/TestElementRegistry.cpp

    # Source files (from Sources.cmake)
    ${OSCIL_SOURCES}
)

target_include_directories(OscilTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/helpers
        ${CMAKE_CURRENT_SOURCE_DIR}/test_harness/include
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
        juce::juce_dsp
        nlohmann_json::nlohmann_json
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:juce::juce_opengl>
        OscilStrictWarnings
        OscilWarningSuppressions
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
        OSCIL_ENABLE_TEST_IDS=1
        $<$<BOOL:${OSCIL_ENABLE_OPENGL}>:OSCIL_ENABLE_OPENGL=1>
)

# Apply RTSan to tests (tests are executables, not shared libraries)
if(OSCIL_ENABLE_RTSAN AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20)
    target_compile_options(OscilTests PRIVATE -fsanitize=realtime)
    target_link_options(OscilTests PRIVATE -fsanitize=realtime)
endif()

include(GoogleTest)
gtest_discover_tests(OscilTests
    DISCOVERY_TIMEOUT 30
    PROPERTIES TIMEOUT 30
)

# ============================================================================
# Lint Test Targets
# ============================================================================

if(OSCIL_ENABLE_SIZE_LIMIT_LINT)
    add_test(NAME SizeLimitsLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_SIZE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --max-file-lines ${OSCIL_MAX_FILE_LINES}
            --max-function-lines ${OSCIL_MAX_FUNCTION_LINES}
    )
    set_tests_properties(SizeLimitsLint PROPERTIES LABELS "lint")
endif()

if(OSCIL_ENABLE_TEST_QUALITY_LINT)
    add_test(NAME TestQualityLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_TEST_QUALITY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths tests
            --min-assertions ${OSCIL_MIN_ASSERTIONS_PER_TEST}
    )
    set_tests_properties(TestQualityLint PROPERTIES LABELS "lint")
endif()

if(OSCIL_ENABLE_SOURCE_REGISTRY_LINT)
    add_test(NAME SourceRegistryLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_SOURCE_REGISTRY_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(SourceRegistryLint PROPERTIES LABELS "lint")
endif()

if(OSCIL_ENABLE_TODO_LINT)
    add_test(NAME TodoLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_TODO_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
    )
    set_tests_properties(TodoLint PROPERTIES LABELS "lint")
endif()

if(OSCIL_ENABLE_ARCHITECTURE_LINT)
    add_test(NAME ArchitectureLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_ARCHITECTURE_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src
    )
    set_tests_properties(ArchitectureLint PROPERTIES LABELS "lint")
endif()

if(OSCIL_ENABLE_COMMENT_LINT)
    add_test(NAME CommentLint
        COMMAND ${Python3_EXECUTABLE} ${OSCIL_COMMENT_LINT_SCRIPT}
            --root ${CMAKE_CURRENT_SOURCE_DIR}
            --paths include src tests test_harness
            --doc-paths include
            --max-violations 0
    )
    set_tests_properties(CommentLint PROPERTIES LABELS "lint")
endif()
