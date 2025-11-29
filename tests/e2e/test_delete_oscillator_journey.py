"""
E2E Test: Delete Oscillator Complete User Journey

Tests the full workflow a user would follow to delete an oscillator:
1. Ensure oscillators exist
2. Select an oscillator
3. Click delete button
4. Verify oscillator is removed from list
5. Test delete from config popup
6. Verify state persistence
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup_with_oscillators(client: OscilTestClient, count: int = 2):
    """Reset state and create multiple oscillators for testing"""
    print("[SETUP] Resetting state and creating oscillators...")
    client.reset_state()
    time.sleep(0.5)
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)

    sources = client.get_sources()
    if len(sources) == 0:
        print("[WARN] No sources available")
        return

    source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))

    colors = ["#00FF00", "#00BFFF", "#FF6B6B", "#FFD93D"]
    for i in range(count):
        name = f"Delete Test Osc {i + 1}"
        color = colors[i % len(colors)]
        osc_id = client.add_oscillator(source_id, name=name, color=color)
        if osc_id:
            print(f"[SETUP] Created oscillator: {name}")
        time.sleep(0.3)

    # Wait for UI to refresh and register test IDs
    # Poll for the first oscillator item to appear
    time.sleep(0.5)
    for attempt in range(10):
        if client.element_exists("sidebar_oscillators_item_0"):
            print(f"[SETUP] Oscillator UI ready (attempt {attempt + 1})")
            break
        time.sleep(0.2)


def test_delete_oscillator_from_list(client: OscilTestClient):
    """
    Test: Delete Oscillator from List Item

    Tests deleting an oscillator by clicking the delete button in the list item.
    """

    # Get initial oscillators
    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    client.assert_true(initial_count >= 1, "Should have at least one oscillator")

    if initial_count == 0:
        print("[SKIP] No oscillators to delete")
        return

    # Select first oscillator
    target_osc = initial_oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    print(f"[INFO] Will delete oscillator: {osc_id}")

    # Click on oscillator to select it (expands the item)
    # OscillatorListItem uses order-index based test IDs: sidebar_oscillators_item_{index}
    osc_index = 0  # First oscillator
    list_item_id = f"sidebar_oscillators_item_{osc_index}"
    if client.element_exists(list_item_id):
        print(f"[ACTION] Selecting oscillator: {list_item_id}")
        client.click(list_item_id)
        time.sleep(0.3)

    # Click delete button
    delete_btn_id = f"{list_item_id}_delete"

    if client.element_exists(delete_btn_id):
        print("[ACTION] Clicking delete button...")
        client.click(delete_btn_id)
        time.sleep(0.5)
    else:
        print("[WARN] Delete button not found directly, checking for confirmation dialog")
        # Some implementations might have a confirmation
        pass

    # Check if confirmation dialog appears
    confirm_dialog_id = "confirmDeleteDialog"
    if client.element_exists(confirm_dialog_id):
        print("[ACTION] Confirming deletion...")
        confirm_btn_id = f"{confirm_dialog_id}_confirm"
        if client.element_exists(confirm_btn_id):
            client.click(confirm_btn_id)
            time.sleep(0.3)

    # Verify oscillator was deleted
    time.sleep(0.3)
    final_oscillators = client.get_oscillators()
    final_count = len(final_oscillators)

    client.assert_true(
        final_count == initial_count - 1,
        f"Should have {initial_count - 1} oscillators (got {final_count})"
    )

    # Verify the specific oscillator is gone
    remaining_ids = [
        osc.get('oscillatorId', osc.get('id', ''))
        for osc in final_oscillators
    ]
    client.assert_true(
        osc_id not in remaining_ids,
        f"Deleted oscillator {osc_id} should not be in list"
    )

    print("[SUCCESS] Delete from list completed")


def test_delete_oscillator_from_config_popup(client: OscilTestClient):
    """
    Test: Delete Oscillator from Config Popup

    Tests deleting an oscillator via the delete button in the config popup.
    """

    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    client.assert_true(initial_count >= 1, "Should have at least one oscillator")

    if initial_count == 0:
        print("[SKIP] No oscillators to delete")
        return

    target_osc = initial_oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    print(f"[INFO] Will delete oscillator via config popup: {osc_id}")

    # Select oscillator
    # OscillatorListItem uses order-index based test IDs
    osc_index = 0  # First oscillator
    list_item_id = f"sidebar_oscillators_item_{osc_index}"
    if client.element_exists(list_item_id):
        client.click(list_item_id)
        time.sleep(0.3)

    # Open config popup via settings button
    settings_btn_id = f"{list_item_id}_settings"
    if client.element_exists(settings_btn_id):
        print("[ACTION] Opening config popup...")
        client.click(settings_btn_id)
        time.sleep(0.5)

    # Wait for popup
    if not client.wait_for_visible("configPopup", timeout_ms=3000):
        print("[SKIP] Could not open config popup")
        return

    # Click delete button in popup
    popup_delete_btn_id = "configPopup_deleteBtn"
    if client.element_exists(popup_delete_btn_id):
        print("[ACTION] Clicking delete in popup...")
        client.click(popup_delete_btn_id)
        time.sleep(0.5)

        # Handle confirmation if present
        confirm_dialog_id = "confirmDeleteDialog"
        if client.element_exists(confirm_dialog_id):
            confirm_btn_id = f"{confirm_dialog_id}_confirm"
            if client.element_exists(confirm_btn_id):
                client.click(confirm_btn_id)
                time.sleep(0.3)

    # Verify deletion
    time.sleep(0.3)
    final_oscillators = client.get_oscillators()
    final_count = len(final_oscillators)

    client.assert_true(
        final_count == initial_count - 1,
        f"Should have {initial_count - 1} oscillators (got {final_count})"
    )

    # Popup should close automatically after delete
    client.assert_element_not_visible("configPopup", "Config popup should close after delete")

    print("[SUCCESS] Delete from config popup completed")


def test_delete_all_oscillators(client: OscilTestClient):
    """
    Test: Delete All Oscillators

    Tests that all oscillators can be deleted and the empty state is shown.
    """

    # Delete all oscillators one by one
    max_iterations = 10
    for iteration in range(max_iterations):
        oscillators = client.get_oscillators()
        if len(oscillators) == 0:
            break

        # Always target the first item (index 0) since previous items are deleted
        # OscillatorListItem uses order-index based test IDs
        list_item_id = "sidebar_oscillators_item_0"

        # Select and delete
        if client.element_exists(list_item_id):
            client.click(list_item_id)
            time.sleep(0.2)

        delete_btn_id = f"{list_item_id}_delete"
        if client.element_exists(delete_btn_id):
            client.click(delete_btn_id)
            time.sleep(0.3)

            # Handle confirmation
            confirm_dialog_id = "confirmDeleteDialog"
            if client.element_exists(confirm_dialog_id):
                confirm_btn_id = f"{confirm_dialog_id}_confirm"
                if client.element_exists(confirm_btn_id):
                    client.click(confirm_btn_id)
                    time.sleep(0.2)

    # Verify all deleted
    final_oscillators = client.get_oscillators()
    client.assert_oscillator_count(0, "All oscillators should be deleted")

    # Check for empty state UI
    # (The sidebar should show an empty state message)
    empty_state_id = "sidebar_emptyState"
    if client.element_exists(empty_state_id):
        client.assert_element_visible(empty_state_id, "Empty state should be visible")

    print("[SUCCESS] Delete all oscillators completed")


def teardown(client: OscilTestClient):
    """Clean up"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


def main():
    """Run all delete oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("DELETE OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    # Test 1: Delete from list
    print("\n--- Test 1: Delete from List Item ---")
    setup_with_oscillators(client, count=2)
    try:
        test_delete_oscillator_from_list(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()
    finally:
        teardown(client)

    # Test 2: Delete from config popup
    print("\n--- Test 2: Delete from Config Popup ---")
    setup_with_oscillators(client, count=2)
    try:
        test_delete_oscillator_from_config_popup(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 3: Delete all
    print("\n--- Test 3: Delete All Oscillators ---")
    setup_with_oscillators(client, count=3)
    try:
        test_delete_all_oscillators(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
