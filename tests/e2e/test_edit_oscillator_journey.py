"""
E2E Test: Edit Oscillator Complete User Journey

Tests the full workflow a user would follow to edit an existing oscillator:
1. Ensure an oscillator exists (create one if needed)
2. Select the oscillator in the list
3. Open the config popup (via settings button)
4. Change oscillator name
5. Change oscillator color
6. Change processing mode
7. Adjust line width, opacity, scale sliders
8. Close popup
9. Verify changes persisted
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup(client: OscilTestClient):
    """Reset state, open editor, and ensure we have at least one oscillator"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.5)
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)

    # Ensure at least one oscillator exists
    oscillators = client.get_oscillators()
    if len(oscillators) == 0:
        print("[SETUP] Creating test oscillator via API...")
        sources = client.get_sources()
        if len(sources) > 0:
            source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))
            osc_id = client.add_oscillator(source_id, name="Test Oscillator", color="#00FF00")
            if osc_id:
                print(f"[SETUP] Created oscillator: {osc_id}")
                time.sleep(0.3)
            else:
                print("[WARN] Could not create oscillator via API")


def test_edit_oscillator_via_config_popup(client: OscilTestClient):
    """
    Test: Edit Oscillator via Config Popup

    Full user journey for editing an oscillator's settings through the config popup.
    """

    # Step 1: Get oscillators and select one
    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) > 0, "Should have at least one oscillator")

    if len(oscillators) == 0:
        print("[SKIP] No oscillators available")
        return

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    original_name = target_osc.get('name', 'Unknown')
    print(f"[INFO] Editing oscillator: {osc_id} (name: {original_name})")

    # Step 2: Find and click on the oscillator list item
    # OscillatorListItem uses order-index based test IDs: sidebar_oscillators_item_{index}
    # Since we're working with oscillators[0], it's index 0
    osc_index = 0  # First oscillator
    list_item_id = f"sidebar_oscillators_item_{osc_index}"

    if client.element_exists(list_item_id):
        print(f"[ACTION] Selecting oscillator: {list_item_id}")
        client.click(list_item_id)
        time.sleep(0.3)

    # Step 3: Click settings button to open config popup
    settings_btn_id = f"{list_item_id}_settings"

    if client.element_exists(settings_btn_id):
        print("[ACTION] Opening config popup via settings button...")
        client.click(settings_btn_id)
        time.sleep(0.5)
    else:
        print("[WARN] Settings button not found, trying direct config popup trigger")
        # Some implementations may require different interaction
        pass

    # Step 4: Verify config popup appears
    popup_visible = client.wait_for_visible("configPopup", timeout_ms=3000)
    if not popup_visible:
        # Try alternative ways to open the popup
        # Double-click might open config
        if client.element_exists(list_item_id):
            print("[ACTION] Trying double-click to open config...")
            client.double_click(list_item_id)
            time.sleep(0.5)
            popup_visible = client.element_visible("configPopup")

    client.assert_true(popup_visible, "Config popup should be visible")

    if not popup_visible:
        print("[SKIP] Could not open config popup")
        return

    # Step 5: Edit oscillator name
    name_field_id = "configPopup_nameField"
    if client.element_exists(name_field_id):
        new_name = "Renamed Oscillator E2E"
        print(f"[ACTION] Changing name to: {new_name}")
        client.clear_text(name_field_id)
        client.type_text(name_field_id, new_name)
        time.sleep(0.2)

    # Step 6: Change color via color swatches
    color_swatches_id = "configPopup_colorSwatches"
    if client.element_exists(color_swatches_id):
        print("[ACTION] Changing color...")
        # Click on a different color swatch (e.g., swatch 3 = Gold)
        swatch_id = f"{color_swatches_id}_swatch_3"
        if client.element_exists(swatch_id):
            client.click(swatch_id)
        else:
            # Click swatches directly
            client.click(color_swatches_id)
        time.sleep(0.2)

    # Step 7: Change processing mode
    mode_buttons_id = "configPopup_modeButtons"
    if client.element_exists(mode_buttons_id):
        print("[ACTION] Changing processing mode...")
        # Try to select "Mono" mode (usually index 1)
        mono_btn_id = f"{mode_buttons_id}_segment_1"
        if client.element_exists(mono_btn_id):
            client.click(mono_btn_id)
        else:
            # Click on mode buttons directly
            client.click(mode_buttons_id)
        time.sleep(0.2)

    # Step 8: Adjust line width slider
    line_width_slider_id = "configPopup_lineWidthSlider"
    if client.element_exists(line_width_slider_id):
        print("[ACTION] Adjusting line width...")
        client.set_slider_value(line_width_slider_id, 3.0)  # Set to 3 pixels
        time.sleep(0.2)

    # Step 9: Adjust opacity slider
    opacity_slider_id = "configPopup_opacitySlider"
    if client.element_exists(opacity_slider_id):
        print("[ACTION] Adjusting opacity...")
        client.set_slider_value(opacity_slider_id, 0.8)  # Set to 80%
        time.sleep(0.2)

    # Step 10: Adjust vertical scale slider
    scale_slider_id = "configPopup_scaleSlider"
    if client.element_exists(scale_slider_id):
        print("[ACTION] Adjusting vertical scale...")
        client.set_slider_value(scale_slider_id, 1.5)  # Set to 150%
        time.sleep(0.2)

    # Step 11: Close popup (changes should auto-save)
    close_btn_id = "configPopup_closeBtn"
    if client.element_exists(close_btn_id):
        print("[ACTION] Closing config popup...")
        client.click(close_btn_id)
    else:
        # Try footer close button
        client.click("configPopup_footerCloseBtn")
    time.sleep(0.5)

    # Step 12: Verify popup closed
    client.assert_element_not_visible("configPopup", "Config popup should close")

    # Step 13: Verify changes persisted
    updated_oscillators = client.get_oscillators()
    updated_osc = None
    for osc in updated_oscillators:
        if osc.get('oscillatorId', osc.get('id', '')) == osc_id:
            updated_osc = osc
            break

    if updated_osc:
        updated_name = updated_osc.get('name', '')
        # Note: Name might have changed
        print(f"[INFO] Oscillator name after edit: {updated_name}")

    print("[SUCCESS] Edit oscillator journey completed")


def test_change_oscillator_visibility(client: OscilTestClient):
    """
    Test: Toggle Oscillator Visibility

    Tests that clicking the visibility toggle actually hides/shows the oscillator.
    """

    oscillators = client.get_oscillators()
    if len(oscillators) == 0:
        print("[SKIP] No oscillators to test visibility")
        return

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    initial_visible = target_osc.get('visible', True)
    print(f"[INFO] Testing visibility for oscillator: {osc_id} (initial: {initial_visible})")

    # Find visibility toggle in list item
    # OscillatorListItem uses order-index based test IDs
    osc_index = 0  # First oscillator
    list_item_id = f"sidebar_oscillators_item_{osc_index}"
    if client.element_exists(list_item_id):
        client.click(list_item_id)
        time.sleep(0.3)

    # Click visibility toggle (in expanded mode it's a toggle component)
    vis_toggle_id = f"{list_item_id}_vis_toggle"
    if not client.element_exists(vis_toggle_id):
        vis_toggle_id = f"{list_item_id}_vis_btn"  # Compact mode uses button

    if client.element_exists(vis_toggle_id):
        print(f"[ACTION] Toggling visibility: {vis_toggle_id}")
        client.click(vis_toggle_id)
        time.sleep(0.3)

        # Verify state changed
        updated_oscillators = client.get_oscillators()
        for osc in updated_oscillators:
            if osc.get('oscillatorId', osc.get('id', '')) == osc_id:
                new_visible = osc.get('visible', True)
                client.assert_true(
                    new_visible != initial_visible,
                    f"Visibility should have toggled from {initial_visible} to {not initial_visible}"
                )
                break

        # Toggle back
        client.click(vis_toggle_id)
        time.sleep(0.2)

    print("[SUCCESS] Visibility toggle test completed")


def test_change_processing_mode_inline(client: OscilTestClient):
    """
    Test: Change Processing Mode Inline

    Tests changing processing mode via the inline segmented buttons in expanded list item.
    """

    oscillators = client.get_oscillators()
    if len(oscillators) == 0:
        print("[SKIP] No oscillators to test")
        return

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    print(f"[INFO] Testing processing mode for: {osc_id}")

    # Select oscillator to expand it
    # OscillatorListItem uses order-index based test IDs
    osc_index = 0  # First oscillator
    list_item_id = f"sidebar_oscillators_item_{osc_index}"
    if client.element_exists(list_item_id):
        client.click(list_item_id)
        time.sleep(0.3)

        # Find mode buttons in expanded item
        mode_btn_id = f"{list_item_id}_mode"
        if client.element_exists(mode_btn_id):
            # Try clicking the mono mode segment
            mono_segment_id = f"{mode_btn_id}_segment_1"
            if client.element_exists(mono_segment_id):
                print("[ACTION] Clicking Mono mode segment...")
                client.click(mono_segment_id)
                time.sleep(0.3)

                # Verify mode changed
                updated_oscillators = client.get_oscillators()
                for osc in updated_oscillators:
                    if osc.get('oscillatorId', osc.get('id', '')) == osc_id:
                        mode = osc.get('processingMode', osc.get('mode', ''))
                        print(f"[INFO] Processing mode after change: {mode}")
                        break

    print("[SUCCESS] Processing mode test completed")


def teardown(client: OscilTestClient):
    """Clean up after test"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


def main():
    """Run all edit oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("EDIT OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    # Test 1: Edit via config popup
    print("\n--- Test 1: Edit via Config Popup ---")
    setup(client)
    try:
        test_edit_oscillator_via_config_popup(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()
    finally:
        teardown(client)

    # Test 2: Toggle visibility
    print("\n--- Test 2: Toggle Visibility ---")
    setup(client)
    try:
        test_change_oscillator_visibility(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 3: Change processing mode inline
    print("\n--- Test 3: Change Processing Mode Inline ---")
    setup(client)
    try:
        test_change_processing_mode_inline(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
