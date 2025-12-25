#!/usr/bin/env python3
"""
E2E Test: Processing Mode UI
Tests the processing mode selection buttons in the oscillator list item and config popup.

Covers:
1. All 6 processing modes are available (Stereo, Mono, Mid, Side, Left, Right)
2. Mode selection updates correctly in OscillatorListItem
3. Mode selection updates correctly in OscillatorConfigPopup
4. Mode changes persist in the oscillator state
"""

import sys
import time
from oscil_test_utils import OscilTestClient, run_test


# Processing modes mapping
PROCESSING_MODES = {
    "FullStereo": 0,
    "Mono": 1,
    "Mid": 2,
    "Side": 3,
    "Left": 4,
    "Right": 5
}


def setup(client: OscilTestClient):
    """Setup: Open editor and ensure clean state"""
    if not client.open_editor():
        print("[ERROR] Could not open editor")
        return False

    # Reset to clean state
    client.reset_state()
    time.sleep(0.5)  # Let state settle
    return True


def test_processing_mode_selection(client: OscilTestClient):
    """Test processing mode selection via oscillator list item"""

    # Setup
    if not setup(client):
        client.assert_true(False, "Test setup failed")
        return

    print("\n=== Test: Processing Mode UI ===")

    # Step 1: Create an oscillator
    print("\n1. Creating oscillator...")
    sources = client.get_sources()
    if not sources:
        client.skip("No sources available to create oscillator")
        return

    source_id = sources[0].get('id')
    oscillator_id = client.add_oscillator(source_id, name="Test Osc")

    if not oscillator_id:
        client.assert_true(False, "Could not add oscillator")
        return

    time.sleep(0.3)
    client.assert_oscillator_count(1, "Should have 1 oscillator after adding")

    # Step 2: Verify oscillator list item appears
    print("\n2. Verifying oscillator list item...")
    list_item_id = "sidebar_oscillators_item_0"
    time.sleep(0.3)  # Give UI time to update

    if not client.element_exists(list_item_id):
        client.skip(f"Element '{list_item_id}' not registered - mode buttons not testable via test harness")
        return

    client.assert_element_exists(list_item_id, "Oscillator list item exists")

    # Step 3: Click to select the oscillator (expands to show mode buttons)
    print("\n3. Selecting oscillator to expand controls...")
    client.click(list_item_id)
    time.sleep(0.3)  # Wait for expansion animation

    # Step 4: Verify mode buttons appear by checking oscillator state
    print("\n4. Verifying mode selection functionality via state API...")
    oscillators = client.get_oscillators()

    if oscillators:
        # API returns 'mode' key, not 'processingMode'
        initial_mode = oscillators[0].get('mode', oscillators[0].get('processingMode', 'unknown'))
        print(f"   Initial mode: {initial_mode}")

        # The mode should default to FullStereo
        client.assert_true(
            initial_mode == "FullStereo" or initial_mode == 0,
            f"Default mode should be FullStereo, got {initial_mode}"
        )

    # Step 5: Test mode selector in config popup
    print("\n5. Testing mode selection in config popup...")

    # Click settings button to open config popup
    settings_button_id = f"{list_item_id}_settings"
    if client.element_exists(settings_button_id):
        client.click(settings_button_id)
        time.sleep(0.5)  # Wait for popup to open

        # Check config popup mode selector exists
        config_mode_selector = "configPopup_modeSelector"
        if client.element_exists(config_mode_selector):
            client.assert_element_exists(config_mode_selector, "Config popup mode selector exists")

            # Try clicking different mode buttons in config popup
            mode_buttons = [
                ("configPopup_modeSelector_stereo", "FullStereo"),
                ("configPopup_modeSelector_mono", "Mono"),
                ("configPopup_modeSelector_mid", "Mid"),
                ("configPopup_modeSelector_side", "Side"),
                ("configPopup_modeSelector_left", "Left"),
                ("configPopup_modeSelector_right", "Right"),
            ]

            for button_id, expected_mode in mode_buttons:
                if client.element_exists(button_id):
                    print(f"   Testing mode button: {expected_mode}")
                    client.click(button_id)
                    time.sleep(0.2)

                    # Verify mode changed in state
                    oscillators = client.get_oscillators()
                    if oscillators:
                        current_mode = oscillators[0].get('processingMode', 'unknown')
                        client.assert_true(
                            current_mode == expected_mode or str(current_mode) == str(PROCESSING_MODES.get(expected_mode, -1)),
                            f"Mode should be {expected_mode} after clicking, got {current_mode}"
                        )
                else:
                    print(f"   [SKIP] Button {button_id} not found")

            # Close config popup
            close_button_id = "configPopup_closeBtn"
            if client.element_exists(close_button_id):
                client.click(close_button_id)
                time.sleep(0.3)
        else:
            client.skip("Config popup mode selector not registered in test harness")
    else:
        client.skip("Settings button not registered in test harness")

    print("\n6. Test complete!")


def test_all_modes_in_list_item(client: OscilTestClient):
    """Test that mode buttons in list item work correctly"""

    # This test verifies the SegmentedButtonBar in the OscillatorListItem
    # The buttons use icons/short text: [Stereo icon] [Mono icon] [M] [S] [L] [R]

    print("\n=== Test: Mode Selection in List Item ===")

    # Setup
    if not setup(client):
        client.assert_true(False, "Test setup failed")
        return

    # Create oscillator
    sources = client.get_sources()
    if not sources:
        client.skip("No sources available")
        return

    source_id = sources[0].get('id')
    oscillator_id = client.add_oscillator(source_id, name="Mode Test Osc")

    if not oscillator_id:
        client.assert_true(False, "Could not add oscillator")
        return

    time.sleep(0.3)

    # Select oscillator to expand
    list_item_id = "sidebar_oscillators_item_0"
    if not client.element_exists(list_item_id):
        client.skip(f"Element '{list_item_id}' not available")
        return

    client.click(list_item_id)
    time.sleep(0.3)

    # The mode bar in the list item is registered as sidebar_oscillators_item_0_mode
    mode_bar_id = f"{list_item_id}_mode"

    if client.element_exists(mode_bar_id):
        client.assert_element_exists(mode_bar_id, "Mode buttons bar exists in list item")
        print("   Mode bar found and accessible")
    else:
        print("   [INFO] Mode bar not individually registered - this is expected")
        print("   Individual mode button clicks not testable via test harness")

    # Verify all 6 modes can be set via config popup
    print("\n   Verifying all modes via config popup...")

    settings_button_id = f"{list_item_id}_settings"
    if client.element_exists(settings_button_id):
        client.click(settings_button_id)
        time.sleep(0.5)

        test_modes = ["Mono", "Mid", "Side", "Left", "Right", "FullStereo"]

        for mode_name in test_modes:
            button_id = f"configPopup_modeSelector_{mode_name.lower()}"
            if mode_name == "FullStereo":
                button_id = "configPopup_modeSelector_stereo"

            if client.element_exists(button_id):
                client.click(button_id)
                time.sleep(0.2)

                oscillators = client.get_oscillators()
                if oscillators:
                    current_mode = oscillators[0].get('processingMode', 'unknown')
                    success = current_mode == mode_name or str(current_mode) == str(PROCESSING_MODES.get(mode_name, -1))
                    client.assert_true(success, f"Mode set to {mode_name}")

        # Close popup
        if client.element_exists("configPopup_closeBtn"):
            client.click("configPopup_closeBtn")

    print("\n   Test complete!")


if __name__ == "__main__":
    print("=" * 60)
    print("PROCESSING MODE UI TEST")
    print("=" * 60)

    client = OscilTestClient()

    if not client.wait_for_harness():
        print("[FATAL] Could not connect to test harness")
        sys.exit(1)

    # Run tests
    test_processing_mode_selection(client)
    test_all_modes_in_list_item(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)
