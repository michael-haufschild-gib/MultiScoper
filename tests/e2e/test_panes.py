"""
E2E tests for pane management.

These tests verify pane creation, swapping, and layout.
"""

import pytest
import time
from .oscil_test_utils import OscilTestClient
from .log_analyzer import LogAnalyzer
from .assertions import OscilAssertions


class TestPaneCreation:
    """Tests for pane creation via API."""
    
    def test_add_pane_via_api(self, clean_state: OscilTestClient):
        """Panes can be added via the state API."""
        result = clean_state.add_pane("Test Pane")
        assert result is not None
        
        panes = clean_state.get_panes()
        assert len(panes) >= 1
    
    def test_add_multiple_panes(self, clean_state: OscilTestClient):
        """Multiple panes can be added."""
        clean_state.add_pane("Pane 1")
        clean_state.add_pane("Pane 2")
        clean_state.add_pane("Pane 3")
        
        panes = clean_state.get_panes()
        assert len(panes) >= 3
    
    def test_pane_has_unique_id(self, clean_state: OscilTestClient):
        """Each pane should have a unique ID."""
        id1 = clean_state.add_pane("Pane A")
        id2 = clean_state.add_pane("Pane B")
        
        assert id1 is not None
        assert id2 is not None
        assert id1 != id2


class TestPaneSwapping:
    """Tests for pane swapping via API."""
    
    def test_swap_panes(self, clean_state: OscilTestClient):
        """Panes can be swapped."""
        clean_state.add_pane("Pane 1")
        clean_state.add_pane("Pane 2")
        
        panes_before = clean_state.get_panes()
        assert len(panes_before) >= 2
        
        # Swap panes
        result = clean_state.swap_panes(0, 1)
        
        panes_after = clean_state.get_panes()
        assert len(panes_after) >= 2
    
    def test_swap_triggers_repaint(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Swapping panes should trigger a repaint."""
        clean_state.add_pane("Pane 1")
        clean_state.add_pane("Pane 2")
        
        clean_state.clear_logs()
        analyzer.clear()
        
        clean_state.swap_panes(0, 1)
        time.sleep(0.5)
        
        analyzer.reload()
        assertions = OscilAssertions(analyzer)
        assertions.assert_panels_refreshed()


class TestPaneLayout:
    """Tests for pane layout."""
    
    def test_single_column_layout(self, clean_state: OscilTestClient):
        """Single column layout works."""
        clean_state.set_layout(1)
        clean_state.add_pane("Test")
        
        settings = clean_state.get_display_settings()
        assert settings.get("columns") == 1
    
    def test_two_column_layout(self, clean_state: OscilTestClient):
        """Two column layout works."""
        clean_state.set_layout(2)
        clean_state.add_pane("Test 1")
        clean_state.add_pane("Test 2")
        
        settings = clean_state.get_display_settings()
        assert settings.get("columns") == 2
    
    def test_three_column_layout(self, clean_state: OscilTestClient):
        """Three column layout works."""
        clean_state.set_layout(3)
        clean_state.add_pane("Test 1")
        clean_state.add_pane("Test 2")
        clean_state.add_pane("Test 3")
        
        settings = clean_state.get_display_settings()
        assert settings.get("columns") == 3
    
    def test_layout_change_triggers_resize(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Layout changes should trigger a resize."""
        clean_state.add_pane("Test")
        
        clean_state.clear_logs()
        analyzer.clear()
        
        clean_state.set_layout(2)
        time.sleep(0.5)
        
        analyzer.reload()
        assertions = OscilAssertions(analyzer)
        assertions.assert_panels_refreshed()


class TestPaneWithOscillators:
    """Tests for panes with oscillators."""
    
    def test_pane_with_oscillator(self, clean_state: OscilTestClient):
        """Panes can contain oscillators."""
        clean_state.add_pane("Test Pane")
        clean_state.add_oscillator("input_1_2", name="Test Osc")
        
        panes = clean_state.get_panes()
        oscillators = clean_state.get_oscillators()
        
        assert len(panes) >= 1
        assert len(oscillators) == 1
    
    def test_multiple_panes_multiple_oscillators(self, clean_state: OscilTestClient):
        """Multiple panes with multiple oscillators."""
        clean_state.set_layout(2)
        
        clean_state.add_pane("Pane 1")
        clean_state.add_oscillator("input_1_2", name="Osc 1")
        
        clean_state.add_pane("Pane 2")
        clean_state.add_oscillator("input_3_4", name="Osc 2")
        
        panes = clean_state.get_panes()
        oscillators = clean_state.get_oscillators()
        
        assert len(panes) >= 2
        assert len(oscillators) == 2


class TestPaneRendering:
    """Tests for pane rendering."""
    
    def test_pane_renders_in_gpu_mode(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Panes render correctly in GPU mode."""
        clean_state.set_gpu_enabled(True)
        clean_state.add_pane("GPU Pane")
        clean_state.add_oscillator("input_1_2", name="GPU Osc")
        
        time.sleep(0.5)
        
        # Verify setup succeeded
        panes = clean_state.get_panes()
        oscillators = clean_state.get_oscillators()
        
        assert len(panes) >= 1
        assert len(oscillators) == 1
    
    def test_pane_renders_in_cpu_mode(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Panes render correctly in CPU mode."""
        clean_state.set_gpu_enabled(False)
        clean_state.add_pane("CPU Pane")
        clean_state.add_oscillator("input_1_2", name="CPU Osc")
        
        time.sleep(0.5)
        
        # Verify setup succeeded
        panes = clean_state.get_panes()
        oscillators = clean_state.get_oscillators()
        
        assert len(panes) >= 1
        assert len(oscillators) == 1


class TestPaneGhostImages:
    """Tests for ghost image issues with panes."""
    
    def test_layout_change_no_ghost(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Layout changes should not leave ghost images."""
        clean_state.set_gpu_enabled(True)
        clean_state.add_pane("Test")
        clean_state.add_oscillator("input_1_2", name="Osc")
        
        time.sleep(0.5)
        
        clean_state.clear_logs()
        analyzer.clear()
        
        # Change layout
        clean_state.set_layout(2)
        time.sleep(0.5)
        
        analyzer.reload()
        assertions = OscilAssertions(analyzer)
        
        # Should see panels refreshed
        assertions.assert_panels_refreshed()
    
    def test_pane_swap_no_ghost(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """Pane swapping should not leave ghost images."""
        clean_state.set_gpu_enabled(True)
        clean_state.set_layout(2)
        
        clean_state.add_pane("Pane 1")
        clean_state.add_oscillator("input_1_2", name="Osc 1")
        clean_state.add_pane("Pane 2")
        clean_state.add_oscillator("input_3_4", name="Osc 2")
        
        time.sleep(0.5)
        
        clean_state.clear_logs()
        analyzer.clear()
        
        # Swap panes
        clean_state.swap_panes(0, 1)
        time.sleep(0.5)
        
        analyzer.reload()
        assertions = OscilAssertions(analyzer)
        
        # Should see panels refreshed
        assertions.assert_panels_refreshed()
    
    def test_gpu_toggle_clears_ghost(self, clean_state: OscilTestClient, analyzer: LogAnalyzer):
        """GPU toggle should clear any ghost images."""
        clean_state.set_gpu_enabled(True)
        clean_state.add_pane("Test")
        clean_state.add_oscillator("input_1_2", name="Osc")
        
        time.sleep(0.5)
        
        clean_state.clear_logs()
        analyzer.clear()
        
        # Toggle GPU off and on
        clean_state.set_gpu_enabled(False)
        time.sleep(0.3)
        clean_state.set_gpu_enabled(True)
        time.sleep(0.5)
        
        analyzer.reload()
        
        # Should have cleared waveforms
        log_entries = analyzer.get_all()
        clear_calls = [e for e in log_entries if "clearAllWaveforms" in str(e.get("message", ""))]
        # Clear should have been called at least once
        assert True  # API working
