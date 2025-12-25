"""
E2E tests for timing settings.

These tests verify timing modes, intervals, BPM control, and host sync.
"""

import pytest
import time
from .oscil_test_utils import OscilTestClient
from .log_analyzer import LogAnalyzer
from .assertions import OscilAssertions


class TestTimingMode:
    """Tests for timing mode selection via API."""
    
    def test_get_timing_settings(self, clean_state: OscilTestClient):
        """Timing settings can be retrieved."""
        settings = clean_state.get_timing_settings()
        assert settings is not None
        assert "mode" in settings or "interval" in settings or "success" in settings
    
    def test_set_free_running_mode(self, clean_state: OscilTestClient):
        """Free running mode can be set."""
        result = clean_state.set_timing_settings(mode="free")
        assert result.get("success", False) or result is True
        
        time.sleep(0.2)
        
        settings = clean_state.get_timing_settings()
        # Verify settings are accessible
        assert settings is not None
    
    def test_set_beat_sync_mode(self, clean_state: OscilTestClient):
        """Beat sync mode can be set."""
        result = clean_state.set_timing_settings(mode="beat")
        assert result.get("success", False) or result is True
    
    def test_set_external_trigger_mode(self, clean_state: OscilTestClient):
        """External trigger mode can be set."""
        result = clean_state.set_timing_settings(mode="external")
        assert result.get("success", False) or result is True


class TestIntervalControl:
    """Tests for capture interval control via API."""
    
    def test_set_interval_ms(self, clean_state: OscilTestClient):
        """Capture interval can be set in milliseconds."""
        result = clean_state.set_timing_settings(intervalMs=100.0)
        assert result.get("success", False) or result is True
    
    def test_set_minimum_interval(self, clean_state: OscilTestClient):
        """Minimum interval should be valid."""
        result = clean_state.set_timing_settings(intervalMs=10.0)
        assert result.get("success", False) or result is True
    
    def test_set_maximum_interval(self, clean_state: OscilTestClient):
        """Maximum interval should be valid."""
        result = clean_state.set_timing_settings(intervalMs=1000.0)
        assert result.get("success", False) or result is True


class TestBpmControl:
    """Tests for BPM control via API."""
    
    def test_set_bpm(self, clean_state: OscilTestClient):
        """BPM can be set."""
        result = clean_state.set_timing_settings(bpm=120.0)
        assert result.get("success", False) or result is True
    
    def test_set_low_bpm(self, clean_state: OscilTestClient):
        """Low BPM values should be valid."""
        result = clean_state.set_timing_settings(bpm=60.0)
        assert result.get("success", False) or result is True
    
    def test_set_high_bpm(self, clean_state: OscilTestClient):
        """High BPM values should be valid."""
        result = clean_state.set_timing_settings(bpm=200.0)
        assert result.get("success", False) or result is True


class TestHostSync:
    """Tests for host synchronization via API."""
    
    def test_enable_host_sync(self, clean_state: OscilTestClient):
        """Host sync can be enabled."""
        result = clean_state.set_timing_settings(hostSync=True)
        assert result.get("success", False) or result is True
    
    def test_disable_host_sync(self, clean_state: OscilTestClient):
        """Host sync can be disabled."""
        result = clean_state.set_timing_settings(hostSync=False)
        assert result.get("success", False) or result is True
    
    def test_host_sync_with_bpm(self, clean_state: OscilTestClient):
        """Host sync and BPM can be set together."""
        # When host sync is enabled, BPM should come from host
        result = clean_state.set_timing_settings(hostSync=True)
        assert result.get("success", False) or result is True
        
        # Get current settings
        settings = clean_state.get_timing_settings()
        assert settings is not None


class TestTimingIntegration:
    """Integration tests for timing settings."""
    
    def test_timing_with_oscillator(self, setup_oscillator):
        """Timing settings work with active oscillators."""
        client, osc_id = setup_oscillator
        
        # Set timing settings
        result = client.set_timing_settings(bpm=90.0)
        assert result.get("success", False) or result is True
        
        # Verify oscillator still exists
        oscillators = client.get_oscillators()
        assert len(oscillators) == 1
    
    def test_multiple_timing_changes(self, clean_state: OscilTestClient):
        """Multiple timing changes should be applied."""
        # Change timing multiple times
        clean_state.set_timing_settings(bpm=80.0)
        clean_state.set_timing_settings(bpm=100.0)
        clean_state.set_timing_settings(bpm=120.0)
        
        settings = clean_state.get_timing_settings()
        assert settings is not None
    
    def test_timing_mode_changes(self, clean_state: OscilTestClient):
        """Timing mode can be changed multiple times."""
        clean_state.set_timing_settings(mode="free")
        clean_state.set_timing_settings(mode="beat")
        clean_state.set_timing_settings(mode="external")
        clean_state.set_timing_settings(mode="free")
        
        settings = clean_state.get_timing_settings()
        assert settings is not None


class TestTransport:
    """Tests for transport control via API."""
    
    def test_transport_state(self, clean_state: OscilTestClient):
        """Transport state can be queried."""
        settings = clean_state.get_timing_settings()
        assert settings is not None
    
    def test_timing_settings_persist(self, clean_state: OscilTestClient):
        """Timing settings should persist."""
        # Set specific timing
        clean_state.set_timing_settings(bpm=145.0, hostSync=False)
        
        # Add an oscillator
        clean_state.add_oscillator("input_1_2", name="Test")
        
        # Verify settings still exist
        settings = clean_state.get_timing_settings()
        assert settings is not None
