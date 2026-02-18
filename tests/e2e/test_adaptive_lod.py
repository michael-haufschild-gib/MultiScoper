"""
E2E Test: Adaptive Level-of-Detail (LOD) System

Tests the adaptive waveform resolution system:
1. LOD tier is reported correctly via API
2. Waveform sample count matches LOD tier
3. LOD info is available in waveform state
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup(client: OscilTestClient):
    """Reset state, open editor, and create an oscillator"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.5)
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)
    
    # Add an oscillator
    sources = client.get_sources()
    if sources:
        source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))
        osc_id = client.add_oscillator(source_id, name="LOD Test Osc", color="#00FFFF")
        if osc_id:
            print(f"[SETUP] Created oscillator: {osc_id}")
            # Inject test waveform data
            client.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
            client.transport_play()
            time.sleep(0.5)
            return osc_id
    return None


def test_lod_tier_reported_in_api(client: OscilTestClient):
    """
    Test: LOD Tier Is Reported via Waveform State API
    
    Verifies that the waveform state endpoint includes LOD information.
    """
    # Get waveform state
    state = client.get_waveform_state()
    
    if not state:
        print("[SKIP] Could not get waveform state")
        return
        
    panes = state.get('panes', [])
    if not panes:
        print("[SKIP] No panes in waveform state")
        return
    
    # Check first waveform in first pane
    waveforms = panes[0].get('waveforms', [])
    if not waveforms:
        print("[SKIP] No waveforms in first pane")
        return
    
    wf = waveforms[0]
    
    # Verify LOD fields are present
    lod_tier = wf.get('lodTier')
    target_samples = wf.get('targetSampleCount')
    width = wf.get('width')
    
    print(f"[INFO] LOD Tier: {lod_tier}")
    print(f"[INFO] Target Sample Count: {target_samples}")
    print(f"[INFO] Waveform Width: {width}")
    
    client.assert_true(lod_tier is not None, "lodTier field should be present")
    client.assert_true(target_samples is not None, "targetSampleCount field should be present")
    client.assert_true(width is not None, "width field should be present")
    
    # Verify LOD tier is a valid value
    valid_tiers = ['Full', 'High', 'Medium', 'Preview']
    client.assert_true(lod_tier in valid_tiers, f"lodTier should be one of {valid_tiers}")
    
    # Verify sample count matches tier
    tier_sample_map = {
        'Full': 2048,
        'High': 1024,
        'Medium': 512,
        'Preview': 256
    }
    expected_samples = tier_sample_map.get(lod_tier)
    client.assert_equals(target_samples, expected_samples, 
                        f"targetSampleCount should be {expected_samples} for tier {lod_tier}")
    
    print("[SUCCESS] LOD tier is correctly reported in API")


def test_lod_tier_matches_width(client: OscilTestClient):
    """
    Test: LOD Tier Matches Width Thresholds
    
    Verifies that the LOD tier follows the defined width thresholds:
    - > 800px: Full (2048 samples)
    - 400-800px: High (1024 samples)
    - 200-400px: Medium (512 samples)
    - < 200px: Preview (256 samples)
    """
    state = client.get_waveform_state()
    
    if not state:
        print("[SKIP] Could not get waveform state")
        return
        
    panes = state.get('panes', [])
    if not panes:
        print("[SKIP] No panes in waveform state")
        return
    
    waveforms = panes[0].get('waveforms', [])
    if not waveforms:
        print("[SKIP] No waveforms in first pane")
        return
    
    wf = waveforms[0]
    lod_tier = wf.get('lodTier')
    width = wf.get('width', 0)
    
    print(f"[INFO] Width: {width}, LOD Tier: {lod_tier}")
    
    # Verify tier matches width thresholds
    if width >= 800:
        expected_tier = 'Full'
    elif width >= 400:
        expected_tier = 'High'
    elif width >= 200:
        expected_tier = 'Medium'
    else:
        expected_tier = 'Preview'
    
    client.assert_equals(lod_tier, expected_tier, 
                        f"Width {width} should result in tier {expected_tier}")
    
    print(f"[SUCCESS] Width {width} correctly maps to tier {lod_tier}")


def test_waveform_has_data_after_lod(client: OscilTestClient):
    """
    Test: Waveform Still Has Data After LOD Processing
    
    Verifies that LOD decimation doesn't break waveform data flow.
    """
    state = client.get_waveform_state()
    
    if not state:
        print("[SKIP] Could not get waveform state")
        return
        
    panes = state.get('panes', [])
    if not panes:
        print("[SKIP] No panes in waveform state")
        return
    
    waveforms = panes[0].get('waveforms', [])
    if not waveforms:
        print("[SKIP] No waveforms in first pane")
        return
    
    wf = waveforms[0]
    has_data = wf.get('hasWaveformData', False)
    peak_level = wf.get('peakLevel', 0.0)
    
    print(f"[INFO] Has waveform data: {has_data}")
    print(f"[INFO] Peak level: {peak_level}")
    
    client.assert_true(has_data, "Waveform should have data after LOD processing")
    client.assert_true(peak_level > 0.0, "Peak level should be > 0 (audio is playing)")
    
    print("[SUCCESS] Waveform has data after LOD processing")


def run_tests(client: OscilTestClient):
    """Run all LOD tests"""
    osc_id = setup(client)
    if not osc_id:
        print("[FATAL] Setup failed - could not create oscillator")
        return False
    
    try:
        test_lod_tier_reported_in_api(client)
        test_lod_tier_matches_width(client)
        test_waveform_has_data_after_lod(client)
        return True
    except Exception as e:
        print(f"[ERROR] Test failed with exception: {e}")
        return False


if __name__ == "__main__":
    client = OscilTestClient()
    
    if not client.is_harness_running():
        print("[FATAL] Test harness is not running. Start it first.")
        sys.exit(1)
    
    success = run_tests(client)
    sys.exit(0 if success else 1)













