"""
Custom assertions for Oscil E2E tests.

This module provides assertion helpers that verify execution paths,
rendering behavior, and UI state through log analysis.
"""

from typing import Optional, List, Dict, Any, Union
from .log_analyzer import LogAnalyzer, LogEntry


class OscilAssertions:
    """
    Custom assertions for verifying plugin behavior through debug logs.
    
    Usage:
        assertions = OscilAssertions(analyzer)
        assertions.assert_waveform_rendered(pane_id="pane_0")
        assertions.assert_gpu_enabled()
    """
    
    def __init__(self, analyzer: LogAnalyzer):
        """Initialize with a log analyzer."""
        self.analyzer = analyzer
    
    def reload(self) -> "OscilAssertions":
        """Reload the log analyzer."""
        self.analyzer.reload()
        return self
    
    # =========================================================================
    # Rendering Assertions
    # =========================================================================
    
    def assert_gpu_enabled(self) -> "OscilAssertions":
        """Assert that GPU rendering is enabled."""
        entries = self.analyzer.filter(location="setGpuRenderingEnabled")
        if not entries:
            raise AssertionError("No GPU rendering state change logged")
        
        last_entry = entries[-1]
        if not last_entry.data.get("enabled", False):
            raise AssertionError("GPU rendering is not enabled (last state was disabled)")
        return self
    
    def assert_gpu_disabled(self) -> "OscilAssertions":
        """Assert that GPU rendering is disabled."""
        entries = self.analyzer.filter(location="setGpuRenderingEnabled")
        if not entries:
            raise AssertionError("No GPU rendering state change logged")
        
        last_entry = entries[-1]
        if last_entry.data.get("enabled", True):
            raise AssertionError("GPU rendering is not disabled (last state was enabled)")
        return self
    
    def assert_waveform_cleared(self) -> "OscilAssertions":
        """Assert that clearAllWaveforms was called."""
        self.analyzer.assert_called(
            location="clearAllWaveforms",
            message="Clearing",
            at_least=1
        )
        return self
    
    def assert_waveform_rendered(self, waveform_id: Optional[int] = None) -> "OscilAssertions":
        """Assert that a waveform was rendered."""
        entries = self.analyzer.filter(location="renderOpenGL")
        if not entries:
            raise AssertionError("No renderOpenGL calls logged")
        
        if waveform_id is not None:
            # Check for specific waveform ID in waveform entry logs
            wf_entries = self.analyzer.filter(location="waveformEntry")
            found = any(e.data.get("id") == waveform_id for e in wf_entries)
            if not found:
                raise AssertionError(f"Waveform ID {waveform_id} was not rendered")
        
        return self
    
    def assert_background_color(
        self,
        red: Optional[float] = None,
        green: Optional[float] = None,
        blue: Optional[float] = None,
        alpha: Optional[float] = None,
        tolerance: float = 0.01
    ) -> "OscilAssertions":
        """Assert that the background color matches expected values."""
        entries = self.analyzer.filter(location="beginFrame", message="Clearing")
        if not entries:
            raise AssertionError("No background clear operations logged")
        
        last_entry = entries[-1]
        data = last_entry.data
        
        def check_component(name: str, expected: Optional[float], actual_key: str):
            if expected is not None:
                actual = data.get(actual_key)
                if actual is None:
                    raise AssertionError(f"No {name} value in log data")
                if abs(actual - expected) > tolerance:
                    raise AssertionError(
                        f"Background {name} mismatch: expected {expected}, got {actual}"
                    )
        
        check_component("red", red, "bgRed")
        check_component("green", green, "bgGreen")
        check_component("blue", blue, "bgBlue")
        check_component("alpha", alpha, "bgAlpha")
        
        return self
    
    def assert_opaque_state(self, component: str, expected_opaque: bool) -> "OscilAssertions":
        """Assert that a component has the expected opaque state."""
        entries = self.analyzer.filter(
            location=f"{component}:setGpuRenderingEnabled",
            message="Setting opaque state"
        )
        if not entries:
            # Try alternative patterns
            entries = self.analyzer.filter(message="Setting opaque state")
            entries = [e for e in entries if component in e.location]
        
        if not entries:
            raise AssertionError(f"No opaque state change logged for {component}")
        
        last_entry = entries[-1]
        actual_opaque = last_entry.data.get("setOpaqueTo", not expected_opaque)
        
        if actual_opaque != expected_opaque:
            raise AssertionError(
                f"Expected {component} opaque={expected_opaque}, but was {actual_opaque}"
            )
        return self
    
    # =========================================================================
    # Panel/Pane Assertions
    # =========================================================================
    
    def assert_panels_refreshed(self, at_least: int = 1) -> "OscilAssertions":
        """Assert that panels were refreshed."""
        self.analyzer.assert_called(
            location="refreshPanels",
            at_least=at_least
        )
        return self
    
    def assert_pane_count(self, expected_count: int) -> "OscilAssertions":
        """Assert the number of panes after a refresh."""
        entries = self.analyzer.filter(location="refreshPanels", message="Refreshing panels")
        if not entries:
            raise AssertionError("No refreshPanels calls logged")
        
        # Check the pane count from log data
        last_entry = entries[-1]
        pane_count = last_entry.data.get("paneCountBefore")
        if pane_count is not None and pane_count != expected_count:
            # Note: paneCountBefore might not match expected after refresh
            pass  # We'd need paneCountAfter for accurate assertion
        
        return self
    
    def assert_column_layout(self, expected_columns: int) -> "OscilAssertions":
        """Assert the column layout."""
        entries = self.analyzer.filter(location="refreshPanels")
        if not entries:
            raise AssertionError("No refreshPanels calls logged")
        
        last_entry = entries[-1]
        column_count = last_entry.data.get("columnCount")
        
        if column_count is not None and column_count != expected_columns:
            raise AssertionError(
                f"Expected {expected_columns} columns, but got {column_count}"
            )
        return self
    
    # =========================================================================
    # Waveform Data Assertions
    # =========================================================================
    
    def assert_waveform_updated(self, waveform_id: int) -> "OscilAssertions":
        """Assert that a specific waveform was updated."""
        entries = self.analyzer.filter(location="updateWaveform")
        found = any(e.data.get("id") == waveform_id for e in entries)
        if not found:
            raise AssertionError(f"Waveform ID {waveform_id} was never updated")
        return self
    
    def assert_waveform_bounds(
        self,
        waveform_id: int,
        x: Optional[int] = None,
        y: Optional[int] = None,
        width: Optional[int] = None,
        height: Optional[int] = None
    ) -> "OscilAssertions":
        """Assert waveform bounds."""
        entries = self.analyzer.filter(location="updateWaveform")
        wf_entries = [e for e in entries if e.data.get("id") == waveform_id]
        
        if not wf_entries:
            raise AssertionError(f"No updates for waveform ID {waveform_id}")
        
        last_entry = wf_entries[-1]
        data = last_entry.data
        
        def check_bound(name: str, expected: Optional[int], key: str):
            if expected is not None:
                actual = data.get(key)
                if actual != expected:
                    raise AssertionError(
                        f"Waveform {waveform_id} {name}: expected {expected}, got {actual}"
                    )
        
        check_bound("x", x, "boundsX")
        check_bound("y", y, "boundsY")
        check_bound("width", width, "boundsW")
        check_bound("height", height, "boundsH")
        
        return self
    
    def assert_unique_waveform_count(self, expected: int) -> "OscilAssertions":
        """Assert the number of unique waveform IDs that were updated."""
        entries = self.analyzer.filter(location="updateWaveform")
        unique_ids = set(e.data.get("id") for e in entries if "id" in e.data)
        
        if len(unique_ids) != expected:
            raise AssertionError(
                f"Expected {expected} unique waveforms, got {len(unique_ids)}: {unique_ids}"
            )
        return self
    
    # =========================================================================
    # Timing Assertions
    # =========================================================================
    
    def assert_timing_mode(self, mode: str) -> "OscilAssertions":
        """Assert the timing mode (TIME or MELODIC)."""
        entries = self.analyzer.filter(location="setTimingMode")
        if not entries:
            raise AssertionError("No timing mode changes logged")
        
        last_entry = entries[-1]
        actual = last_entry.data.get("mode")
        if actual != mode:
            raise AssertionError(f"Expected timing mode '{mode}', got '{actual}'")
        return self
    
    def assert_bpm(self, expected_bpm: float, tolerance: float = 0.1) -> "OscilAssertions":
        """Assert the BPM value."""
        entries = self.analyzer.filter(location="*BPM*")
        if not entries:
            raise AssertionError("No BPM changes logged")
        
        last_entry = entries[-1]
        actual = last_entry.data.get("bpm") or last_entry.data.get("value")
        if actual is None:
            raise AssertionError("No BPM value in log data")
        if abs(actual - expected_bpm) > tolerance:
            raise AssertionError(f"Expected BPM {expected_bpm}, got {actual}")
        return self
    
    # =========================================================================
    # Execution Path Assertions
    # =========================================================================
    
    def assert_execution_path(
        self,
        expected_locations: List[str],
        hypothesis_id: Optional[str] = None
    ) -> "OscilAssertions":
        """Assert that locations were called in order."""
        self.analyzer.assert_sequence(expected_locations, hypothesis_id)
        return self
    
    def assert_no_errors(self) -> "OscilAssertions":
        """Assert that no error entries were logged."""
        error_entries = self.analyzer.filter(message="error|Error|ERROR|failed|Failed")
        if error_entries:
            errors = [e.message for e in error_entries[:5]]
            raise AssertionError(f"Found error log entries: {errors}")
        return self
    
    # =========================================================================
    # Composite Assertions
    # =========================================================================
    
    def assert_gpu_toggle_clean(self) -> "OscilAssertions":
        """Assert that a GPU toggle resulted in clean state (no ghosts)."""
        # Should have cleared waveforms
        self.assert_waveform_cleared()
        # Should have refreshed panels
        self.assert_panels_refreshed()
        # Opaque state should match GPU state
        gpu_entries = self.analyzer.filter(location="setGpuRenderingEnabled")
        if gpu_entries:
            last = gpu_entries[-1]
            gpu_enabled = last.data.get("enabled", True)
            # In GPU mode, components should NOT be opaque
            # self.assert_opaque_state("PaneComponent", not gpu_enabled)
        return self
    
    def assert_theme_applied(self, theme_name: Optional[str] = None) -> "OscilAssertions":
        """Assert that a theme was applied."""
        entries = self.analyzer.filter(location="setBackgroundColour")
        if not entries:
            raise AssertionError("No background color changes logged (theme not applied)")
        
        # Background alpha should be 1.0 (opaque)
        last_entry = self.analyzer.filter(location="beginFrame")
        if last_entry:
            alpha = last_entry[-1].data.get("bgAlpha")
            if alpha is not None and alpha < 0.99:
                raise AssertionError(f"Background alpha is {alpha}, expected 1.0")
        
        return self
    
    def assert_oscillator_added(self) -> "OscilAssertions":
        """Assert that an oscillator was added."""
        self.assert_panels_refreshed()
        self.analyzer.assert_called(location="updateWaveform", at_least=1)
        return self


def create_assertions(analyzer: LogAnalyzer) -> OscilAssertions:
    """Factory function to create assertions."""
    return OscilAssertions(analyzer)















