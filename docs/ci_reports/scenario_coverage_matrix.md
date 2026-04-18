# Scenario Coverage Matrix

Cross-reference of `docs/test_scenarios.md` TC-* user-flow scenarios against actual test coverage.

**Sources audited:**
- Python E2E: `tests/e2e/test_*.py` (27 files, 435 test functions)
- C++ unit: `tests/test_*.cpp` (approx. 80 files)

**Methodology:** For each TC, located tests asserting the scenario's listed *Expected Results* and *Verification Points*. Coverage marks:
- `y` — scenario's core asserts verified by a real test (not just presence/non-crash)
- `partial` — some Verification Points covered, others absent
- `n` — no test asserts the scenario's expected results

## Matrix

| TC-ID | Title | Coverage | Evidence (file:line) | Gap |
|---|---|---|---|---|
| TC-INIT-001 | Fresh Plugin Load | partial | tests/e2e/test_edge_cases.py:86 (test_editor_opens_with_empty_state), tests/e2e/test_edge_cases.py:94 (test_sidebar_exists_with_no_oscillators); tests/test_oscil_state.cpp:47-70 (default-state fields) | No assert on default window size (1200x800), default sidebar width (280px), toolbar widget presence, status-bar metrics displayed on empty editor |
| TC-INIT-002 | Plugin Load with Saved State | partial | tests/e2e/test_compound_flows.py:771 (test_all_properties_survive_roundtrip); tests/e2e/test_edge_cases.py:244 (test_save_and_load_state); tests/test_oscil_state.cpp:378 (ConstructFromValidXmlRestoresState) | Does not verify specifically the sidebar-collapsed-state restore nor the 2-oscillator + custom theme + 2-column combo described |
| TC-SRC-001 | Automatic Source Discovery | y | tests/e2e/test_multi_instance.py:24 (test_all_tracks_register_sources), tests/e2e/test_multi_instance.py:34 (test_source_ids_differ_across_tracks), tests/e2e/test_multi_instance.py:312 (test_add_track_registers_new_source), tests/e2e/test_multi_instance.py:360 (test_other_instances_see_new_source) | — |
| TC-SRC-002 | Source Removal | y | tests/e2e/test_multi_instance.py:336 (test_remove_track_deregisters_source), tests/e2e/test_multi_instance.py:388 (test_remove_track_with_bound_oscillators), tests/e2e/test_multi_instance.py:880 (test_oscillator_bound_to_nonexistent_source) | — |
| TC-OSC-001 | Create Oscillator via Add Button | y | tests/e2e/test_oscillator_crud.py:27 (test_add_via_dialog), tests/e2e/test_oscillator_crud.py:72 (test_add_button_exists_and_is_clickable), tests/e2e/test_oscillator_crud.py:160 (test_add_multiple_oscillators_increments_count) | — |
| TC-OSC-002 | Create Oscillator from Source | n | none | No test exercises the "Add to Pane" per-source dropdown creation flow |
| TC-OSC-003 | Configure Oscillator via Config Popup | y | tests/e2e/test_config_popup.py:25-467 (entire file); tests/e2e/test_complete_workflows.py:481 (test_all_popup_properties_persist); tests/e2e/test_compound_flows.py:297 (test_config_popup_targets_correct_oscillator) | — |
| TC-OSC-004 | Processing Mode Changes | y | tests/e2e/test_oscillator_crud.py:387 (test_default_mode_is_full_stereo), tests/e2e/test_oscillator_crud.py:396 (test_all_modes_via_config_popup); tests/e2e/test_config_popup.py:290 (test_all_mode_buttons_update_state); tests/e2e/test_integration_flows.py:590 (test_mode_change_preserves_waveform_data) | L/R-labels rendering and Side-channel silence for mono content not asserted |
| TC-OSC-005 | Delete Oscillator | y | tests/e2e/test_oscillator_crud.py:180 (test_delete_from_list_item), tests/e2e/test_oscillator_crud.py:192 (test_delete_all_one_by_one), tests/e2e/test_oscillator_crud.py:206 (test_deleted_oscillator_id_gone_from_state); tests/e2e/test_edge_cases.py:462 (test_delete_nonexistent_oscillator) | — |
| TC-OSC-006 | Oscillator Visibility Toggle | y | tests/e2e/test_waveform_display.py:128 (test_oscillator_visibility_affects_rendering); tests/e2e/test_oscillator_crud.py:320 (test_toggle_changes_state), tests/e2e/test_oscillator_crud.py:344 (test_toggle_roundtrip); tests/e2e/test_integration_flows.py:627 (test_visibility_toggle_during_playback_waveform_recovers) | — |
| TC-OSC-007 | Oscillator Reordering | partial | tests/e2e/test_sidebar.py:212 (test_reorder_via_api); tests/e2e/test_edge_cases.py:418 (test_reorder_survives_save_load); tests/test_oscillator_list_component.cpp:349 (MoveRequestEmitsReorderWithinBounds) | Only reorders via API, not drag-and-drop; no drag preview or drop indicator asserted |
| TC-OSC-008 | Oscillator Selection and Expanded Controls | partial | tests/e2e/test_sidebar.py:98 (test_clicking_item_expands_it), tests/e2e/test_sidebar.py:125 (test_selecting_different_item_switches_expansion); tests/test_oscillator_list_component.cpp:474 (ItemExpansionUpdatesListLayout) | Compact/expanded heights (~56/~100px) and deselect-on-outside-click not verified |
| TC-OSC-009 | Oscillator Drag Handle in Sidebar | n | none | No test distinguishes drag-handle zone vs center click; no grab-cursor assertion |
| TC-THM-001 | Switch Theme | y | tests/e2e/test_theme.py:37 (test_dropdown_has_themes), tests/e2e/test_theme.py:51 (test_selecting_different_theme_updates_state), tests/e2e/test_theme.py:88 (test_all_themes_selectable_without_error), tests/e2e/test_theme.py:117 (test_theme_persists_across_editor_lifecycle), tests/e2e/test_theme.py:247 (test_theme_survives_state_save_load) | — |
| TC-THM-002 | Create Custom Theme | y | tests/test_theme_manager_crud.cpp:88 (CreateCustomTheme), tests/test_theme_manager_crud.cpp:137-226 (edge cases) | No E2E test of the Theme Editor UI create flow |
| TC-THM-003 | Clone Theme | y | tests/test_theme_manager_crud.cpp:101 (CloneTheme); tests/test_theme_manager_apply.cpp:231 (ClonePreservesAllProperties), tests/test_theme_manager_apply.cpp:300 (ClonePreservesGlassFields) | — |
| TC-THM-004 | Delete Custom Theme | y | tests/test_theme_manager_crud.cpp:123 (DeleteCustomTheme), tests/test_theme_manager_crud.cpp:260 (DeleteNonexistentTheme), tests/test_theme_manager_crud.cpp:378 (DeleteCurrentThemeSwitchesTheme) | — |
| TC-THM-005 | System Theme Protection | y | tests/test_theme_manager_crud.cpp:52 (SystemThemesImmutable), tests/test_theme_manager_crud.cpp:284 (UpdateSystemThemeFails), tests/test_theme_manager_crud.cpp:415 (RenameSystemThemeFails); tests/test_theme_manager_persistence.cpp:59 (ImportSystemThemeNameRejected) | — |
| TC-THM-006 | Theme Export/Import | y | tests/test_theme_manager_persistence.cpp:101 (ExportImportRoundtrip), :38-94 (import-invalid-xml / empty / non-xml / unsafe-name) | No E2E UI-level export/import |
| TC-LAY-001 | Column Layout Switching | y | tests/e2e/test_pane_layout.py:150 (test_two_column_two_panes_half_width), :172 (three_column), :200/:229 (switch 2<->1), :256 (cycle_1_2_3_1), :338 (test_dropdown_selection_changes_column_count), :499 (test_column_layout_survives_save_load); tests/e2e/test_options_controls.py:388 (test_layout_dropdown_selection_persists_through_save_load) | — |
| TC-LAY-002 | Sidebar Resize | partial | tests/e2e/test_sidebar.py:177 (test_drag_changes_width); tests/e2e/test_ui_interactions.py:209 (test_drag_offset_on_resize_handle) | Min (200) / max (800) bounds not asserted; persistence of custom width across reopen not verified in these tests |
| TC-LAY-003 | Sidebar Collapse/Expand | n | none | Only accordion-section collapse (test_sidebar.py:46) is tested; full sidebar collapse/expand toggle + state persistence not covered |
| TC-LAY-004 | Pane Drag Within Same Column | partial | tests/e2e/test_pane_layout.py:394 (test_move_pane_changes_order); tests/test_pane.cpp, tests/test_pane_drag_state.cpp | Uses move API, not physical drag from header; no drop-indicator assertion |
| TC-LAY-005 | Pane Drag Across Columns | partial | tests/e2e/test_pane_layout.py:424 (test_move_in_multi_column_preserves_column_assignment) | Same: API-level move, not actual drag; no assertion on target-column drop indicator |
| TC-LAY-006 | Pane Drag Edge Cases | n | none | No test for Escape-cancels-drag, drag-out-of-window, self-drop, empty-column-after-drag |
| TC-TIM-001 | TIME Mode Configuration | y | tests/e2e/test_timing.py:71 (test_switch_back_to_time_mode), :97 (test_field_accepts_values), :114 (test_interval_change_affects_display_samples), :159 (test_extreme_values); tests/test_timing_engine.cpp | — |
| TC-TIM-002 | MELODIC Mode Configuration | y | tests/e2e/test_timing.py:48 (test_switch_to_melodic_mode), :177 (test_melodic_mode_display_samples_differ_from_time_mode), :209 (test_bpm_change_affects_melodic_samples), :263 (test_note_dropdown_exists_in_melodic_mode), :283 (test_note_dropdown_selection_affects_display_samples); tests/test_timing_calculations.cpp | — |
| TC-TIM-003 | Host Sync Enable/Disable | partial | tests/e2e/test_timing.py:383 (test_sync_toggle_clickable), :398 (does_not_crash_during_playback), :431 (test_sync_toggle_changes_state); tests/test_timing_engine_host.cpp | Synced/Not-Synced status indicator not asserted; alignment-to-host-events behavior not verified |
| TC-TIM-004 | Reset on Play | n | none | No E2E or unit test exercising a "reset on play" UI toggle and its effect on waveform after transport start |
| TC-DIS-001 | Show Grid Toggle | y | tests/e2e/test_options_controls.py:83 (test_grid_toggle_clickable), :272 (test_grid_toggle_state_persists_through_save_load); tests/test_oscil_state.cpp:292 (SetShowGridEnabledPersists) | "All panes simultaneously" not asserted — only state toggle |
| TC-DIS-002 | Auto-Scale Toggle | partial | tests/e2e/test_options_controls.py:103 (test_auto_scale_toggle_clickable); tests/test_oscil_state.cpp:298 (SetAutoScaleEnabledPersists) | No assertion that waveform amplitude actually changes on toggle |
| TC-DIS-003 | Hold Display Toggle | n | none | No test for a Hold Display toggle — feature presence not verified in tests |
| TC-TRG-001 | Trigger Mode Selection | n | none | No E2E/unit test exercising Free Running vs Triggered mode UI and waveform behavior difference |
| TC-TRG-002 | Trigger Threshold and Edge | n | none | No test for threshold slider, rising/falling edge selection, or waveform stability change |
| TC-STB-001 | Status Bar Visibility Toggle | partial | tests/test_oscil_state.cpp:286 (SetStatusBarVisiblePersists) | No E2E asserting the toggle hides/shows status bar or that main area reflows |
| TC-STB-002 | Status Bar Metrics Display | y | tests/e2e/test_status_bar.py:30 (test_label_exists), :48 (test_oscillator_count_updates), :73 (test_source_count_positive), :97 (test_fps_positive_during_playback), :133 (test_oscillator_count_after_delete), :177 (test_render_mode_label_updates_after_gpu_toggle), :221 (test_all_labels_have_text_during_activity) | — |
| TC-PER-001 | Project Save and Load | y | tests/e2e/test_complete_workflows.py:173 (test_complex_state_roundtrip); tests/e2e/test_compound_flows.py:771 (test_all_properties_survive_roundtrip); tests/e2e/test_integration_flows.py:367 (test_save_load_with_multiple_oscillators_different_modes); tests/e2e/test_edge_cases.py:270 (test_save_load_preserves_source_and_pane_ids); tests/test_state_integration.cpp | — |
| TC-PER-002 | Global Preferences Persistence | y | tests/test_global_preferences.cpp:63-141 (DefaultThemeIsDarkProfessional, SetAndGetTheme, EmptyThemeNameRoundTrip, SpecialCharactersInThemeName) | No E2E for DAW-restart preference persistence |
| TC-SBR-001 | Section Expand/Collapse | y | tests/e2e/test_sidebar.py:21 (test_timing_section_expand_reveals_content), :46 (test_timing_section_collapse_hides_content), :70 (test_options_section_expand); tests/test_oscil_accordion.cpp (section state) | — |
| TC-SBR-002 | Oscillator List Filtering | y | tests/e2e/test_oscillator_filter.py:27 (test_filter_tabs_exist), :60 (test_visible_filter_hides_invisible_oscillators), :101 (test_hidden_filter_shows_only_hidden), :274 (test_visibility_toggle_updates_filtered_list), :323 (test_filter_count_matches_state); tests/test_oscillator_list_component.cpp:194 (FilteringVisibility), :393 (ToolbarCountBadgeReflectsVisibleTotals) | — |
| TC-ERR-001 | Source Disconnection Recovery | y | tests/e2e/test_multi_instance.py:388 (test_remove_track_with_bound_oscillators), :880 (test_oscillator_bound_to_nonexistent_source), :1184 (test_remove_middle_track_cascade), :1692 (test_rebind_oscillator_to_different_source) | — |
| TC-ERR-002 | Invalid Theme Handling | y | tests/test_oscil_state.cpp:392 (ConstructFromInvalidXmlFallsBackToDefaults), :401 (ConstructFromEmptyXmlFallsBackToDefaults); tests/test_theme_manager_persistence.cpp:38 (ImportInvalidXml), :45 (ImportEmptyString), :52 (ImportNonXmlContent), :151 (ValueTreeEmpty), :164 (ValueTreeWrongType), :178 (ValueTreeMissingProperties) | — |
| TC-PFM-001 | High Oscillator Count Performance | partial | tests/e2e/test_performance.py:160 (test_fps_stable_with_multiple_oscillators); tests/e2e/test_edge_cases.py:677 (test_ten_oscillators_no_crash); tests/e2e/test_sixteen_instances.py:241 (test_sixteen_instances_sustained_playback_no_leak) | Scenario says 20 oscillators with FPS > 30 + CPU < 10% — tests use 10, and don't assert specific FPS/CPU thresholds |
| TC-PFM-002 | Window Resize Performance | n | none | No test drags the window corner or measures resize smoothness/artifacts |
| TC-MC-001 | Timebase Slider Adjustment | n | none | No UI slider labeled "Timebase" exercised in tests. Timing interval range 1-60000ms not asserted at UI level |
| TC-MC-002 | Gain Slider Adjustment | y | tests/e2e/test_options_controls.py:122 (test_gain_slider_accepts_values), :138 (test_gain_slider_increment_decrement), :338 (test_gain_slider_value_persists_through_save_load) | dB range (-60 to +12) bounds not asserted; effect on waveform display not measured |
| TC-OLT-001 | Filter Tabs (All/Visible/Hidden) | y | tests/e2e/test_oscillator_filter.py:27-323 (entire file) | — |
| TC-OLT-002 | Oscillator Count Display | y | tests/e2e/test_oscillator_filter.py:323 (test_filter_count_matches_state); tests/test_oscillator_list_component.cpp:393 (ToolbarCountBadgeReflectsVisibleTotals) | — |
| TC-OCP-001 | Line Width Slider | y | tests/e2e/test_config_popup.py:102 (test_line_width_slider_adjustable), :380 (test_line_width_boundary_values); tests/e2e/test_waveform_display.py:376 (test_opacity_and_line_width_via_api) | — |
| TC-OCP-002 | Vertical Scale Slider | n | none | No test exercises vertical-scale slider on oscillator config popup |
| TC-OCP-003 | Vertical Offset Slider | n | none | No test exercises vertical-offset slider |
| TC-OCP-004 | Name Editing | y | tests/e2e/test_oscillator_crud.py:255 (test_name_change_persists); tests/e2e/test_config_popup.py:32 (test_name_field_editable); tests/e2e/test_waveform_display.py:419 (test_name_survives_state_roundtrip); tests/e2e/test_edge_cases.py:597-643 (unicode/empty/long/special-char names); tests/test_oscillator_list_component.cpp:313 (NameChangePropagates) | — |
| TC-OCP-005 | Source Selector | partial | tests/e2e/test_config_popup.py:57 (test_source_selector_present); tests/e2e/test_multi_instance.py:1692 (test_rebind_oscillator_to_different_source) | "Present" only — no assertion that selecting a different source in the popup actually changes audio binding |
| TC-KEY-001 | Escape Key Closes Popups | partial | tests/e2e/test_keyboard_nav.py:146 (test_escape_closes_dialog), :174 (test_escape_with_no_dialog_does_not_crash) | Only covers one dialog path; theme editor, config popup, settings popup not individually tested |
| TC-KEY-002 | Tab Focus Trap in Modals | partial | tests/e2e/test_keyboard_nav.py:58 (test_focus_next_changes_focused_element), :82 (test_focus_previous_reverses_direction), :110 (test_focus_cycle_does_not_crash) | No assertion that focus is trapped inside a modal (cannot escape to background elements) |
| TC-KEY-003 | Arrow Keys on Sliders | n | none | No test presses arrow/Home/End on a slider and asserts value change |
| TC-KEY-004 | Arrow Keys on Dropdowns | n | none | No test uses arrow keys to navigate open dropdown options |
| TC-KEY-005 | Space/Enter Activates Controls | n | none | No test presses Space/Enter on a focused toggle/button/checkbox |
| TC-KEY-006 | Delete Key on Oscillator List Item | n | none | No test presses Delete/Backspace on a focused oscillator row |
| TC-SET-001 | Status Bar Toggle | partial | tests/test_oscil_state.cpp:286 (SetStatusBarVisiblePersists) | Settings popup UI toggle flow not exercised |
| TC-SET-002 | Layout Icons | y | tests/e2e/test_options_controls.py:22 (test_layout_dropdown_exists), :35, :50, :388; tests/e2e/test_pane_layout.py:338 (test_dropdown_selection_changes_column_count) | Present as dropdown — scenario mentions "icons" — but selection path is covered |
| TC-SET-003 | Edit Theme Button | n | none | No test for a Theme Editor open button in Settings popup |
| TC-SRC-003 | Source Activity Indicator | n | none | No test for an activity indicator dim/pulse state on source items |
| TC-SRC-004 | Add to Pane Dropdown | n | none | No test for a per-source "Add to Pane" dropdown to create oscillators |
| TC-SCR-001 | Sidebar Sections Scrollable | n | none | No test for sidebar overflow/scrollbar |
| TC-SCR-002 | Oscillator List Scrollable | partial | tests/e2e/test_ui_interactions.py:99 (test_scroll_oscillator_list), :118 (test_scroll_up_and_down) | Does not verify selection works at all scroll positions nor that drag still works while scrolled |
| TC-SCR-003 | Mouse Wheel on Sliders | n | none | No test for mouse-wheel slider adjustment |
| TC-SCR-004 | Double-Click to Reset Slider | partial | tests/e2e/test_ui_interactions.py:55 (test_double_click_list_item) — lists items, not sliders | No test for double-click reset on any slider |
| TC-TIP-001 | Tooltip Appearance on Hover | n | none | Hover tests exist but none assert tooltip text appears after delay |
| TC-TIP-002 | Tooltips Show Keyboard Shortcuts | n | none | No tooltip-text assertions |
| TC-CNF-001 | Delete Oscillator Confirmation | n | none | tests/e2e/test_oscillator_crud.py:180/192/206 delete without any confirmation path tested |
| TC-CNF-002 | Delete Custom Theme Confirmation | n | none | tests/test_theme_manager_crud.cpp:123 deletes directly; no UI confirmation dialog tested |

## Totals

**Covered: 27 / Partial: 17 / Absent: 26**

(Total TCs audited: 70)

## Top 10 High-Risk Uncovered Scenarios

Ranked by user-trust impact — plugin load, state, crash-surface, discovery, and data-destruction paths first.

1. **TC-LAY-006 — Pane Drag Edge Cases.** Escape-cancel, drag-out-of-window, empty-column handling. Uncovered crash surfaces in a daily-used interaction.
2. **TC-LAY-003 — Sidebar Collapse/Expand.** Core layout affordance; absence means a regression that breaks collapse would ship unnoticed.
3. **TC-CNF-001 — Delete Oscillator Confirmation.** Destructive action; no confirmation means user trust loss on accidental delete. Whether confirmation is implemented at all is not verified.
4. **TC-OSC-002 — Create Oscillator from Source.** One of two primary oscillator-creation entry points. Zero coverage means the "Add to Pane" dropdown flow is invisible to tests.
5. **TC-SRC-004 — Add to Pane Dropdown.** Same mechanism as TC-OSC-002 from the source side. Source-to-pane routing is untested.
6. **TC-TRG-001 / TC-TRG-002 — Trigger Mode + Threshold/Edge.** Feature category entirely untested. If trigger mode is a shipping feature it has no regression guard.
7. **TC-DIS-003 — Hold Display Toggle.** Ditto — zero coverage; unclear if the feature exists. Freezes are common user-reported bugs.
8. **TC-TIM-004 — Reset on Play.** Host-sync-adjacent; timing quirks are DAW-specific and fragile. Untested.
9. **TC-MC-001 — Timebase Slider Adjustment.** Core oscilloscope control; the display-time window. No slider test means a broken slider would ship.
10. **TC-KEY-003 / TC-KEY-005 / TC-KEY-006 — Keyboard Navigation (arrow/space/delete).** Accessibility baseline; missing coverage means keyboard-only users hit regressions silently.

## Notes on Methodology and Caveats

- Many C++ unit tests cover *state-level* persistence (theme, grid, auto-scale, sidebar width, status-bar visibility) but the corresponding UI-level E2E interaction is often absent. Those rows are marked `partial`.
- Pane drag-and-drop TCs are marked `partial` because the codebase tests the *reorder API* but not a real pointer drag with drop indicators and cancel semantics.
- Theme Editor UI flow (TC-THM-002 create/modify via editor, TC-SET-003 open editor) is absent at E2E level; CRUD is covered at unit level for ThemeManager.
- Scenarios tagged `n` have no evidence — they are not silently passing via an unrelated test; they are truly unasserted.
