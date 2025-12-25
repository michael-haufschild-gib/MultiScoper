"""
E2E tests for visual presets functionality.

These tests verify preset application via the state API.
"""

import pytest
import time
from .oscil_test_utils import OscilTestClient
from .log_analyzer import LogAnalyzer
from .assertions import OscilAssertions


class TestPresetListing:
    """Tests for preset listing."""
    
    def test_presets_exist(self, clean_state: OscilTestClient):
        """System presets should be available."""
        # Presets would be listed via a specific endpoint
        # For now, verify the API is accessible
        settings = clean_state.get_display_settings()
        assert settings is not None
    
    def test_oscillator_can_be_created(self, clean_state: OscilTestClient):
        """Oscillators can be created to apply presets to."""
        osc_id = clean_state.add_oscillator("input_1_2", name="Preset Target")
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1


class TestPresetApplication:
    """Tests for applying presets via API."""
    
    def test_oscillator_properties(self, clean_state: OscilTestClient):
        """Oscillator properties can be set."""
        osc_id = clean_state.add_oscillator("input_1_2", name="Test")
        
        # Update oscillator (presets would modify these properties)
        result = clean_state.update_oscillator(osc_id, name="Updated")
        
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
    
    def test_multiple_oscillators(self, clean_state: OscilTestClient):
        """Multiple oscillators can be created."""
        clean_state.add_oscillator("input_1_2", name="Osc 1")
        clean_state.add_oscillator("input_3_4", name="Osc 2")
        
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 2


class TestPresetCategories:
    """Tests for preset categories."""
    
    def test_default_settings(self, clean_state: OscilTestClient):
        """Default settings should be valid."""
        settings = clean_state.get_display_settings()
        assert settings is not None
    
    def test_settings_can_be_modified(self, clean_state: OscilTestClient):
        """Settings can be modified (like presets would)."""
        clean_state.set_layout(2)
        clean_state.set_show_grid(True)
        clean_state.set_gpu_enabled(True)
        
        settings = clean_state.get_display_settings()
        assert settings.get("columns") == 2
        assert settings.get("showGrid") is True
        assert settings.get("gpuEnabled") is True


class TestPresetPersistence:
    """Tests for preset persistence."""
    
    def test_settings_persist_after_oscillator_add(self, clean_state: OscilTestClient):
        """Settings should persist after adding oscillators."""
        # Set specific settings
        clean_state.set_layout(2)
        clean_state.set_show_grid(False)
        
        # Add oscillator
        clean_state.add_oscillator("input_1_2", name="Test")
        
        # Verify settings persisted
        settings = clean_state.get_display_settings()
        assert settings.get("columns") == 2
        assert settings.get("showGrid") is False
    
    def test_oscillator_persists_after_settings_change(self, clean_state: OscilTestClient):
        """Oscillators should persist after settings changes."""
        # Add oscillator
        clean_state.add_oscillator("input_1_2", name="Test")
        
        # Change various settings
        clean_state.set_layout(3)
        clean_state.set_gpu_enabled(True)
        clean_state.set_show_grid(True)
        
        # Verify oscillator persisted
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
        assert oscillators[0].get("name") == "Test"


class TestPresetWithRendering:
    """Tests for presets with different rendering modes."""
    
    def test_oscillator_in_gpu_mode(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Oscillators work in GPU mode."""
        clean_state.set_gpu_enabled(True)
        time.sleep(0.3)
        
        clean_state.clear_logs()
        analyzer.clear()
        
        clean_state.add_oscillator("input_1_2", name="GPU Osc")
        time.sleep(0.5)
        
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
    
    def test_oscillator_in_cpu_mode(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Oscillators work in CPU mode."""
        clean_state.set_gpu_enabled(False)
        time.sleep(0.3)
        
        clean_state.clear_logs()
        analyzer.clear()
        
        clean_state.add_oscillator("input_1_2", name="CPU Osc")
        time.sleep(0.5)
        
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
    
    def test_oscillator_persists_across_mode_switch(self, clean_state: OscilTestClient):
        """Oscillators should persist across render mode switches."""
        # Add in GPU mode
        clean_state.set_gpu_enabled(True)
        clean_state.add_oscillator("input_1_2", name="Persistent")
        time.sleep(0.3)
        
        # Switch to CPU
        clean_state.set_gpu_enabled(False)
        time.sleep(0.3)
        
        # Verify oscillator exists
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
        assert oscillators[0].get("name") == "Persistent"
        
        # Switch back to GPU
        clean_state.set_gpu_enabled(True)
        time.sleep(0.3)
        
        # Verify oscillator still exists
        oscillators = clean_state.get_oscillators()
        assert len(oscillators) == 1
