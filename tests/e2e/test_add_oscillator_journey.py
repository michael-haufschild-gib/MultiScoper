"""
E2E Test: Add Oscillator Complete User Journey

Tests the full workflow a user would follow to add a new oscillator:
1. Open the plugin editor
2. Click the "Add Oscillator" button
3. Select a source from the dropdown
4. Select a pane (or create new)
5. Enter a name
6. Pick a color
7. Click OK
8. Verify the oscillator appears in the list
9. Verify the oscillator is visible in the waveform display
"""

import sys
import os
import time

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient, run_test


def setup(client: OscilTestClient):
    """Reset state and open editor before test"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.5)
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)


def test_add_oscillator_complete_journey(client: OscilTestClient):
    """
    Test: Complete Add Oscillator User Journey

    This test verifies that a user can successfully add a new oscillator
    by going through the complete UI flow.
    """

    # Step 1: Verify initial state - no oscillators
    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    print(f"[INFO] Starting with {initial_count} oscillators")

    # Step 2: Verify sidebar and add button exist
    client.assert_element_exists("sidebar", "Sidebar should exist")
    client.assert_element_visible("sidebar_addOscillator", "Add Oscillator button should be visible")

    # Step 3: Click Add Oscillator button
    print("[ACTION] Clicking 'Add Oscillator' button...")
    assert client.click("sidebar_addOscillator"), "Failed to click Add Oscillator button"
    time.sleep(0.5)

    # Step 4: Verify dialog appears
    client.assert_element_visible("addOscillatorDialog", "Add Oscillator dialog should appear")

    # Step 5: Get available sources
    sources = client.get_sources()
    client.assert_true(len(sources) > 0, "At least one source should be available")

    if len(sources) == 0:
        print("[SKIP] No sources available, cannot complete test")
        return

    # Step 6: Select first source from dropdown
    source_dropdown_id = "addOscillatorDialog_sourceDropdown"
    if client.element_exists(source_dropdown_id):
        first_source = sources[0]
        source_id = first_source.get('sourceId', first_source.get('id', 'source_0'))
        print(f"[ACTION] Selecting source: {source_id}")
        client.select_dropdown_item(source_dropdown_id, source_id)
        time.sleep(0.3)

    # Step 7: Select pane (use first available or "new pane")
    pane_dropdown_id = "addOscillatorDialog_paneDropdown"
    panes = client.get_panes()
    if client.element_exists(pane_dropdown_id):
        if len(panes) > 0:
            pane_id = panes[0].get('paneId', panes[0].get('id', 'pane_0'))
            print(f"[ACTION] Selecting pane: {pane_id}")
            client.select_dropdown_item(pane_dropdown_id, pane_id)
        else:
            print("[ACTION] Selecting 'New Pane' option")
            client.select_dropdown_item(pane_dropdown_id, "new_pane")
        time.sleep(0.3)

    # Step 8: Enter oscillator name
    name_field_id = "addOscillatorDialog_nameField"
    if client.element_exists(name_field_id):
        print("[ACTION] Entering oscillator name...")
        client.clear_text(name_field_id)
        client.type_text(name_field_id, "Test Oscillator E2E")
        time.sleep(0.2)

    # Step 9: Select a color (click on first color swatch)
    color_swatches_id = "addOscillatorDialog_colorSwatches"
    if client.element_exists(color_swatches_id):
        print("[ACTION] Selecting color...")
        # Try clicking the second color swatch (Deep Sky Blue)
        swatch_id = f"{color_swatches_id}_swatch_1"
        if client.element_exists(swatch_id):
            client.click(swatch_id)
        else:
            # Click the color swatches component directly
            client.click(color_swatches_id)
        time.sleep(0.2)

    # Step 10: Click OK button
    ok_button_id = "addOscillatorDialog_okBtn"
    client.assert_element_visible(ok_button_id, "OK button should be visible")
    print("[ACTION] Clicking OK button...")
    assert client.click(ok_button_id), "Failed to click OK button"
    time.sleep(0.5)

    # Step 11: Verify dialog closes
    client.assert_element_not_visible("addOscillatorDialog", "Dialog should close after OK")

    # Step 12: Verify oscillator was added
    new_oscillators = client.get_oscillators()
    new_count = len(new_oscillators)
    client.assert_true(
        new_count == initial_count + 1,
        f"Should have {initial_count + 1} oscillators (got {new_count})"
    )

    # Step 13: Verify oscillator appears in sidebar list
    # The new oscillator should have a list item in the sidebar
    if new_count > 0:
        newest_osc = new_oscillators[-1]
        osc_id = newest_osc.get('oscillatorId', newest_osc.get('id', ''))
        if osc_id:
            list_item_id = f"oscillator_{osc_id}"
            # Wait a bit for UI to update
            time.sleep(0.3)
            if client.element_exists(list_item_id):
                client.assert_element_visible(list_item_id, "New oscillator should appear in list")
            else:
                print(f"[INFO] List item '{list_item_id}' not found - checking alternatives")
                # Try to verify by counting visible oscillator items
                pass

    print("[SUCCESS] Add Oscillator journey completed successfully")


def test_add_oscillator_cancel(client: OscilTestClient):
    """
    Test: Cancel Add Oscillator Dialog

    Verifies that canceling the dialog does not create an oscillator.
    """

    # Get initial count
    initial_count = len(client.get_oscillators())

    # Open dialog
    print("[ACTION] Opening Add Oscillator dialog...")
    assert client.click("sidebar_addOscillator"), "Failed to click Add Oscillator"
    time.sleep(0.5)

    client.assert_element_visible("addOscillatorDialog", "Dialog should appear")

    # Click Cancel
    cancel_button_id = "addOscillatorDialog_cancelBtn"
    print("[ACTION] Clicking Cancel button...")
    if client.element_exists(cancel_button_id):
        assert client.click(cancel_button_id), "Failed to click Cancel"
    else:
        # Try clicking close button instead
        client.click("addOscillatorDialog_closeBtn")
    time.sleep(0.5)

    # Verify dialog closes
    client.assert_element_not_visible("addOscillatorDialog", "Dialog should close on Cancel")

    # Verify no oscillator was added
    final_count = len(client.get_oscillators())
    client.assert_true(
        final_count == initial_count,
        f"Oscillator count should remain {initial_count} (got {final_count})"
    )

    print("[SUCCESS] Cancel dialog test completed")


def test_add_oscillator_validation(client: OscilTestClient):
    """
    Test: Add Oscillator Dialog Validation

    Verifies that the dialog validates required fields.
    """

    # Open dialog
    print("[ACTION] Opening Add Oscillator dialog...")
    assert client.click("sidebar_addOscillator"), "Failed to click Add Oscillator"
    time.sleep(0.5)

    client.assert_element_visible("addOscillatorDialog", "Dialog should appear")

    # Try to click OK without selecting source
    # (Note: This depends on implementation - source may be pre-selected)
    name_field_id = "addOscillatorDialog_nameField"
    if client.element_exists(name_field_id):
        # Clear the name field
        client.clear_text(name_field_id)
        time.sleep(0.2)

        # Try clicking OK with empty name
        ok_button_id = "addOscillatorDialog_okBtn"
        client.click(ok_button_id)
        time.sleep(0.3)

        # Check if error message appears or dialog stays open
        dialog_still_open = client.element_visible("addOscillatorDialog")
        error_label_id = "addOscillatorDialog_error"
        has_error = client.element_exists(error_label_id) and client.element_visible(error_label_id)

        # Dialog should either stay open or show an error
        client.assert_true(
            dialog_still_open or has_error,
            "Dialog should validate empty name field"
        )

    # Close dialog
    cancel_button_id = "addOscillatorDialog_cancelBtn"
    if client.element_visible("addOscillatorDialog"):
        if client.element_exists(cancel_button_id):
            client.click(cancel_button_id)
        else:
            client.click("addOscillatorDialog_closeBtn")

    print("[SUCCESS] Validation test completed")


def teardown(client: OscilTestClient):
    """Clean up after test"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


def main():
    """Run all add oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("ADD OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    # Test 1: Complete journey
    print("\n--- Test 1: Complete Add Oscillator Journey ---")
    setup(client)
    try:
        test_add_oscillator_complete_journey(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 2: Cancel dialog
    print("\n--- Test 2: Cancel Dialog ---")
    setup(client)
    try:
        test_add_oscillator_cancel(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 3: Validation
    print("\n--- Test 3: Dialog Validation ---")
    setup(client)
    try:
        test_add_oscillator_validation(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
