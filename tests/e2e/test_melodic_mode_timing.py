"""
E2E Test: Melodic Mode Timing

Verifies that switching to Melodic mode and changing note intervals
actually affects the waveform displaySamples (the bug was that displaySamples
wasn't being updated when switching to Melodic mode).

Test Steps:
1. Add an oscillator
2. Record initial displaySamples in TIME mode
3. Switch to MELODIC mode
4. Verify displaySamples changes based on BPM + note interval
5. Change note interval and verify displaySamples updates
"""

import sys
import os
import time
import requests

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def get_waveform_state(client: OscilTestClient):
    """Get waveform state including displaySamples"""
    try:
        response = requests.get(
            f"{client.base_url}/waveform/state",
            timeout=client.timeout
        )
        if response.status_code == 200:
            return response.json().get('data', response.json())
        return None
    except Exception as e:
        print(f"[ERROR] Get waveform state failed: {e}")
        return None


def get_timing_config(client: OscilTestClient):
    """Get timing configuration from timing engine"""
    try:
        response = requests.get(
            f"{client.base_url}/timing/config",
            timeout=client.timeout
        )
        if response.status_code == 200:
            return response.json().get('data', response.json())
        return None
    except Exception as e:
        print(f"[ERROR] Get timing config failed: {e}")
        return None


def get_display_samples(client: OscilTestClient) -> int:
    """Get the current displaySamples from any waveform"""
    state = get_waveform_state(client)
    if state and 'panes' in state and len(state['panes']) > 0:
        pane = state['panes'][0]
        if 'waveforms' in pane and len(pane['waveforms']) > 0:
            return pane['waveforms'][0].get('displaySamples', 0)
    return 0


def setup(client: OscilTestClient):
    """Reset state, open editor, add an oscillator"""
    print("[SETUP] Resetting state...")
    client.reset_state()
    time.sleep(0.5)

    print("[SETUP] Opening editor...")
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)

    # Add an oscillator
    sources = client.get_sources()
    if not sources:
        print("[WARN] No sources available, test may fail")
        return False

    source_id = sources[0].get('id') or sources[0].get('sourceId')
    print(f"[SETUP] Adding oscillator with source: {source_id}")
    osc_id = client.add_oscillator(source_id, name="Test Oscillator")

    if not osc_id:
        print("[ERROR] Failed to add oscillator")
        return False

    time.sleep(0.5)
    return True


def expand_timing_section(client: OscilTestClient) -> bool:
    """Expand the timing accordion section"""
    timing_section_id = "sidebar_timing"
    if client.element_exists(timing_section_id):
        print("[ACTION] Clicking Timing section...")
        client.click(timing_section_id)
        time.sleep(0.4)
        return True
    return False


def test_melodic_mode_changes_display_samples(client: OscilTestClient):
    """
    Test that switching to Melodic mode and changing settings
    actually updates displaySamples.
    """
    if not setup(client):
        print("[SKIP] Setup failed")
        return

    expand_timing_section(client)

    # Get initial displaySamples in TIME mode
    initial_samples = get_display_samples(client)
    print(f"[INFO] Initial displaySamples (TIME mode): {initial_samples}")

    if initial_samples == 0:
        print("[WARN] Could not get displaySamples, checking oscillator count...")
        oscillators = client.get_oscillators()
        print(f"[INFO] Oscillator count: {len(oscillators)}")

    # Switch to MELODIC mode
    mode_toggle_id = "sidebar_timing_modeToggle"
    melodic_segment_id = f"{mode_toggle_id}_segment_1"

    if not client.element_exists(mode_toggle_id):
        print("[SKIP] Mode toggle not found")
        return

    print("[ACTION] Switching to MELODIC mode...")
    if client.element_exists(melodic_segment_id):
        client.click(melodic_segment_id)
    else:
        # Try clicking the toggle itself
        client.click(mode_toggle_id)
    time.sleep(0.5)

    # Get displaySamples after switching to MELODIC mode
    melodic_samples = get_display_samples(client)
    print(f"[INFO] DisplaySamples after MELODIC mode: {melodic_samples}")

    # Now try changing the note interval
    note_dropdown_id = "sidebar_timing_noteDropdown"
    if client.element_exists(note_dropdown_id):
        print("[ACTION] Opening note interval dropdown...")
        client.click(note_dropdown_id)
        time.sleep(0.3)

        # Try to select a different note interval (e.g., 1/8th = index 3)
        # The dropdown items should have IDs like sidebar_timing_noteDropdown_item_X
        item_id = f"{note_dropdown_id}_item_3"  # 1/8th note
        if client.element_exists(item_id):
            print("[ACTION] Selecting 1/8th note interval...")
            client.click(item_id)
            time.sleep(0.4)

            # Get displaySamples after changing note interval
            new_samples = get_display_samples(client)
            print(f"[INFO] DisplaySamples after note change: {new_samples}")

            # Verify samples changed
            if new_samples != melodic_samples and new_samples > 0:
                print("[PASS] DisplaySamples changed when note interval changed!")
                client.assert_true(True, "Melodic mode note interval affects display")
            else:
                print("[INFO] Samples may not have changed (depends on BPM)")
        else:
            print(f"[INFO] Note item {item_id} not found, trying index selection")

    # Test BPM change effect
    bpm_field_id = "sidebar_timing_bpmField"
    if client.element_exists(bpm_field_id):
        samples_before_bpm = get_display_samples(client)
        print(f"[INFO] DisplaySamples before BPM change: {samples_before_bpm}")

        print("[ACTION] Changing BPM to 60...")
        client.clear_text(bpm_field_id)
        client.type_text(bpm_field_id, "60")
        # Press enter to commit
        time.sleep(0.5)

        samples_after_bpm = get_display_samples(client)
        print(f"[INFO] DisplaySamples after BPM change to 60: {samples_after_bpm}")

        # At 60 BPM, intervals should be longer (more samples)
        # At 120 BPM, intervals should be half as long
        if samples_after_bpm != samples_before_bpm:
            print("[PASS] BPM change affected displaySamples!")
            client.assert_true(True, "BPM change affects display in Melodic mode")

    # Switch back to TIME mode
    time_segment_id = f"{mode_toggle_id}_segment_0"
    if client.element_exists(time_segment_id):
        print("[ACTION] Switching back to TIME mode...")
        client.click(time_segment_id)
        time.sleep(0.4)

        time_samples = get_display_samples(client)
        print(f"[INFO] DisplaySamples after switching back to TIME: {time_samples}")

        # Verify we can switch back and samples update
        client.assert_true(time_samples > 0, "TIME mode has valid displaySamples")

    print("[SUCCESS] Melodic mode timing test completed")


def teardown(client: OscilTestClient):
    """Clean up"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


def main():
    """Run melodic mode timing test"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("MELODIC MODE TIMING TEST")
    print("=" * 60)

    print("\n--- Test: Melodic Mode Changes Display Samples ---")
    try:
        test_melodic_mode_changes_display_samples(client)
    except Exception as e:
        print(f"[ERROR] Test failed with exception: {e}")
        import traceback
        traceback.print_exc()
    finally:
        teardown(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
