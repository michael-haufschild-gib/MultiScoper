"""
E2E Test: Transport Control and Timing Behavior

Tests that the plugin correctly responds to DAW transport events:
1. Play/Stop - waveforms should only display when playing
2. BPM changes - timing-dependent features should update
3. Different waveform types display correctly
"""

import sys
import os
import time

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup(client: OscilTestClient):
    """Reset state and open editor before test"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.3)

    # Ensure transport is playing (harness auto-starts now)
    if not client.is_playing():
        client.transport_play()
        time.sleep(0.3)

    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)


def teardown(client: OscilTestClient):
    """Clean up after test"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


# =============================================================================
# Transport Play/Stop Tests
# =============================================================================

def test_transport_state_api(client: OscilTestClient):
    """
    Test: Transport State API
    Verifies that transport state can be queried and controlled via API.
    """
    print("\n[TEST] Transport State API")

    # Get initial state
    state = client.get_transport_state()
    client.assert_true(state is not None, "Should get transport state")
    print(f"[INFO] Initial state: playing={state.get('playing')}, bpm={state.get('bpm')}")

    # Verify playing status matches API
    is_playing = client.is_playing()
    client.assert_true(
        is_playing == state.get('playing'),
        f"is_playing() should match state (got {is_playing}, expected {state.get('playing')})"
    )

    # Verify BPM is reasonable
    bpm = client.get_bpm()
    client.assert_true(
        20 <= bpm <= 300,
        f"BPM should be in valid range (got {bpm})"
    )

    print("[PASS] Transport state API works correctly")


def test_transport_play_stop(client: OscilTestClient):
    """
    Test: Transport Play/Stop Control
    Verifies that play/stop commands work and affect waveform display.
    """
    print("\n[TEST] Transport Play/Stop Control")

    # Ensure we start playing
    client.transport_play()
    time.sleep(0.5)

    client.assert_true(client.is_playing(), "Should be playing after transport_play()")

    # Add an oscillator so we have something to display
    sources = client.get_sources()
    if len(sources) > 0:
        source_id = sources[0].get('id')
        osc_id = client.add_oscillator(source_id, name="Transport Test Osc")
        time.sleep(0.5)  # Wait for UI update

        print(f"[INFO] Added oscillator with source {source_id}")

    # Stop playback
    print("[ACTION] Stopping transport...")
    client.transport_stop()
    time.sleep(0.5)

    client.assert_true(not client.is_playing(), "Should be stopped after transport_stop()")

    # Get transport state to verify position freezes
    state1 = client.get_transport_state()
    time.sleep(0.3)
    state2 = client.get_transport_state()

    # Position should not advance when stopped
    client.assert_true(
        state1.get('positionSamples') == state2.get('positionSamples'),
        "Position should not advance when stopped"
    )

    # Resume playback
    print("[ACTION] Resuming transport...")
    client.transport_play()
    time.sleep(0.5)

    client.assert_true(client.is_playing(), "Should be playing after resume")

    # Verify position advances
    state3 = client.get_transport_state()
    time.sleep(0.3)
    state4 = client.get_transport_state()

    client.assert_true(
        state4.get('positionSamples') > state3.get('positionSamples'),
        "Position should advance when playing"
    )

    print("[PASS] Transport play/stop works correctly")


def test_transport_rapid_toggle(client: OscilTestClient):
    """
    Test: Rapid Play/Stop Toggle
    Verifies stability under rapid transport changes.
    """
    print("\n[TEST] Rapid Transport Toggle")

    # Rapidly toggle play/stop
    for i in range(5):
        client.transport_stop()
        time.sleep(0.1)
        client.transport_play()
        time.sleep(0.1)

    # Should end in a stable state
    time.sleep(0.3)
    state = client.get_transport_state()
    client.assert_true(state is not None, "Should get valid state after rapid toggles")
    client.assert_true(client.is_playing(), "Should be playing after rapid toggles ending with play")

    print("[PASS] Rapid toggle handled correctly")


# =============================================================================
# BPM Control Tests
# =============================================================================

def test_bpm_change(client: OscilTestClient):
    """
    Test: BPM Change
    Verifies that BPM can be changed and is reflected in transport state.
    """
    print("\n[TEST] BPM Change")

    # Get initial BPM
    initial_bpm = client.get_bpm()
    print(f"[INFO] Initial BPM: {initial_bpm}")

    # Test various BPM values
    test_bpms = [60.0, 120.0, 180.0, 90.0]

    for target_bpm in test_bpms:
        print(f"[ACTION] Setting BPM to {target_bpm}...")
        success = client.set_bpm(target_bpm)
        client.assert_true(success, f"Should successfully set BPM to {target_bpm}")

        time.sleep(0.2)

        actual_bpm = client.get_bpm()
        client.assert_true(
            abs(actual_bpm - target_bpm) < 0.1,
            f"BPM should be {target_bpm} (got {actual_bpm})"
        )

    # Restore original BPM
    client.set_bpm(initial_bpm)

    print("[PASS] BPM changes work correctly")


def test_bpm_boundary_values(client: OscilTestClient):
    """
    Test: BPM Boundary Values
    Tests BPM at minimum and maximum values.
    """
    print("\n[TEST] BPM Boundary Values")

    initial_bpm = client.get_bpm()

    # Test minimum BPM (20)
    print("[ACTION] Testing minimum BPM (20)...")
    client.set_bpm(20.0)
    time.sleep(0.2)
    bpm = client.get_bpm()
    client.assert_true(
        abs(bpm - 20.0) < 0.1,
        f"Should accept minimum BPM 20 (got {bpm})"
    )

    # Test maximum BPM (300)
    print("[ACTION] Testing maximum BPM (300)...")
    client.set_bpm(300.0)
    time.sleep(0.2)
    bpm = client.get_bpm()
    client.assert_true(
        abs(bpm - 300.0) < 0.1,
        f"Should accept maximum BPM 300 (got {bpm})"
    )

    # Restore
    client.set_bpm(initial_bpm)

    print("[PASS] BPM boundary values handled correctly")


def test_bpm_affects_timing_display(client: OscilTestClient):
    """
    Test: BPM Affects Timing Display
    Verifies that changing BPM updates timing-related UI elements.
    """
    print("\n[TEST] BPM Affects Timing Display")

    # This test checks that the timing section shows correct BPM
    # First, add an oscillator
    sources = client.get_sources()
    if len(sources) > 0:
        source_id = sources[0].get('id')
        client.add_oscillator(source_id, name="BPM Test Osc")
        time.sleep(0.5)

    # Set a specific BPM
    client.set_bpm(140.0)
    time.sleep(0.3)

    # Check if timing section exists and displays BPM
    timing_section_id = "timingSidebarSection"
    if client.element_exists(timing_section_id):
        print("[INFO] Timing section found, checking BPM display...")
        # The timing section should reflect the current BPM
        # This is primarily a smoke test - full verification would require
        # reading the displayed text value
        client.assert_element_visible(timing_section_id, "Timing section should be visible")
    else:
        print("[SKIP] Timing section not found in UI")

    # Restore BPM
    client.set_bpm(120.0)

    print("[PASS] BPM display test completed")


# =============================================================================
# Waveform Display Tests
# =============================================================================

def test_waveform_types(client: OscilTestClient):
    """
    Test: Different Waveform Types
    Verifies that different waveform types (sine, square, triangle, etc.)
    can be selected and displayed.
    """
    print("\n[TEST] Waveform Types")

    waveform_types = ["sine", "square", "triangle", "saw"]

    for waveform in waveform_types:
        print(f"[ACTION] Setting track 0 to {waveform} waveform...")
        success = client.set_track_audio(0, waveform=waveform, frequency=440.0, amplitude=0.8)
        client.assert_true(success, f"Should set waveform to {waveform}")

        time.sleep(0.3)

        # Verify track info
        info = client.get_track_info(0)
        if info:
            actual_waveform = info.get('waveform', '').lower()
            client.assert_true(
                actual_waveform == waveform,
                f"Track waveform should be {waveform} (got {actual_waveform})"
            )

    # Restore to sine
    client.set_track_audio(0, waveform="sine")

    print("[PASS] Waveform types work correctly")


def test_waveform_frequency(client: OscilTestClient):
    """
    Test: Waveform Frequency Changes
    Verifies that frequency changes are applied correctly.
    """
    print("\n[TEST] Waveform Frequency Changes")

    frequencies = [220.0, 440.0, 880.0, 1000.0]

    for freq in frequencies:
        print(f"[ACTION] Setting frequency to {freq} Hz...")
        success = client.set_track_audio(0, waveform="sine", frequency=freq)
        client.assert_true(success, f"Should set frequency to {freq}")

        time.sleep(0.2)

        info = client.get_track_info(0)
        if info:
            actual_freq = info.get('frequency', 0)
            client.assert_true(
                abs(actual_freq - freq) < 0.1,
                f"Frequency should be {freq} (got {actual_freq})"
            )

    # Restore
    client.set_track_audio(0, waveform="sine", frequency=440.0)

    print("[PASS] Frequency changes work correctly")


def test_waveform_amplitude(client: OscilTestClient):
    """
    Test: Waveform Amplitude Changes
    Verifies that amplitude changes affect the waveform display.
    """
    print("\n[TEST] Waveform Amplitude Changes")

    amplitudes = [0.0, 0.25, 0.5, 0.75, 1.0]

    for amp in amplitudes:
        print(f"[ACTION] Setting amplitude to {amp}...")
        success = client.set_track_audio(0, amplitude=amp)
        client.assert_true(success, f"Should set amplitude to {amp}")

        time.sleep(0.2)

        info = client.get_track_info(0)
        if info:
            actual_amp = info.get('amplitude', -1)
            client.assert_true(
                abs(actual_amp - amp) < 0.01,
                f"Amplitude should be {amp} (got {actual_amp})"
            )

    # Restore
    client.set_track_audio(0, amplitude=0.8)

    print("[PASS] Amplitude changes work correctly")


def test_silence_waveform(client: OscilTestClient):
    """
    Test: Silence Waveform
    Verifies that the silence waveform type produces no signal.
    """
    print("\n[TEST] Silence Waveform")

    # Set to silence
    success = client.set_track_audio(0, waveform="silence")
    client.assert_true(success, "Should set waveform to silence")

    time.sleep(0.3)

    info = client.get_track_info(0)
    if info:
        waveform = info.get('waveform', '').lower()
        client.assert_true(waveform == "silence", f"Waveform should be silence (got {waveform})")

    # Restore
    client.set_track_audio(0, waveform="sine")

    print("[PASS] Silence waveform works correctly")


# =============================================================================
# Combined Transport + Audio Tests
# =============================================================================

def test_stop_freezes_waveform(client: OscilTestClient):
    """
    Test: Stop Freezes Waveform
    Verifies that stopping transport freezes the waveform display.
    """
    print("\n[TEST] Stop Freezes Waveform")

    # Ensure playing with an oscillator
    client.transport_play()
    time.sleep(0.3)

    sources = client.get_sources()
    if len(sources) > 0:
        source_id = sources[0].get('id')
        client.add_oscillator(source_id, name="Freeze Test")
        time.sleep(0.5)

    # Stop and verify state is frozen
    client.transport_stop()
    time.sleep(0.3)

    state1 = client.get_transport_state()
    time.sleep(0.5)
    state2 = client.get_transport_state()

    # Position should be identical when stopped
    pos_diff = abs(state2.get('positionSamples', 0) - state1.get('positionSamples', 0))
    client.assert_true(
        pos_diff == 0,
        f"Position should not change when stopped (diff: {pos_diff})"
    )

    # Resume
    client.transport_play()

    print("[PASS] Stop correctly freezes transport position")


def test_bpm_while_playing(client: OscilTestClient):
    """
    Test: BPM Change While Playing
    Verifies that BPM can be changed during playback without issues.
    """
    print("\n[TEST] BPM Change While Playing")

    client.transport_play()
    time.sleep(0.3)

    # Rapidly change BPM while playing
    bpm_values = [80.0, 120.0, 160.0, 100.0, 140.0]

    for bpm in bpm_values:
        client.set_bpm(bpm)
        time.sleep(0.2)

        # Verify we're still playing
        client.assert_true(client.is_playing(), f"Should still be playing after BPM change to {bpm}")

        # Verify BPM was applied
        actual = client.get_bpm()
        client.assert_true(
            abs(actual - bpm) < 0.1,
            f"BPM should be {bpm} (got {actual})"
        )

    # Restore
    client.set_bpm(120.0)

    print("[PASS] BPM changes during playback work correctly")


def test_position_reset(client: OscilTestClient):
    """
    Test: Position Reset
    Verifies that transport position can be reset.
    """
    print("\n[TEST] Position Reset")

    client.transport_play()
    time.sleep(0.5)  # Let position advance

    state1 = client.get_transport_state()
    initial_pos = state1.get('positionSamples', 0)
    print(f"[INFO] Position before reset: {initial_pos}")

    # Reset position to 0
    success = client.set_position(0)
    client.assert_true(success, "Should reset position successfully")

    time.sleep(0.2)

    state2 = client.get_transport_state()
    reset_pos = state2.get('positionSamples', -1)

    # Position should be near 0 (some samples may have played since reset)
    client.assert_true(
        reset_pos < initial_pos,
        f"Position should be reset (was {initial_pos}, now {reset_pos})"
    )

    print("[PASS] Position reset works correctly")


# =============================================================================
# Main Test Runner
# =============================================================================

def main():
    """Run all transport and timing tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        print("[FATAL] Test harness not available")
        sys.exit(1)

    print("\n" + "=" * 60)
    print("TRANSPORT AND TIMING E2E TESTS")
    print("=" * 60)

    tests = [
        # Transport API tests
        ("Transport State API", test_transport_state_api),
        ("Transport Play/Stop", test_transport_play_stop),
        ("Rapid Transport Toggle", test_transport_rapid_toggle),

        # BPM tests
        ("BPM Change", test_bpm_change),
        ("BPM Boundary Values", test_bpm_boundary_values),
        ("BPM Affects Timing Display", test_bpm_affects_timing_display),

        # Waveform tests
        ("Waveform Types", test_waveform_types),
        ("Waveform Frequency", test_waveform_frequency),
        ("Waveform Amplitude", test_waveform_amplitude),
        ("Silence Waveform", test_silence_waveform),

        # Combined tests
        ("Stop Freezes Waveform", test_stop_freezes_waveform),
        ("BPM While Playing", test_bpm_while_playing),
        ("Position Reset", test_position_reset),
    ]

    passed = 0
    failed = 0

    for test_name, test_func in tests:
        print(f"\n--- {test_name} ---")
        setup(client)
        try:
            test_func(client)
            passed += 1
        except AssertionError as e:
            print(f"[FAIL] {e}")
            failed += 1
        except Exception as e:
            print(f"[ERROR] Unexpected error: {e}")
            failed += 1
        finally:
            teardown(client)

    # Print summary
    print("\n" + "=" * 60)
    print(f"RESULTS: {passed}/{passed + failed} tests passed")
    if failed > 0:
        print(f"FAILED: {failed} tests")
    print("=" * 60)

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
