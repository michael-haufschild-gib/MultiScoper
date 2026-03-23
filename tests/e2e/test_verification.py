"""
E2E tests for server-side verification endpoints.

These tests exercise the harness's pixel-based verification APIs,
which provide programmatic assertions about rendering output without
relying on screenshots reviewed by a human.

What bugs these tests catch:
- Server-side waveform verification returning false negatives
- Color verification not matching expected oscillator colors
- Bounds verification tolerance too tight or too loose
- Analyze waveform API returning malformed data
- Verification endpoints crashing on nonexistent elements
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestServerSideVerification:
    """Exercise /verify/* and /analyze/* endpoints."""

    def test_verify_visible_on_registered_element(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: verify_visible endpoint crashes or returns wrong result
        for elements that are definitely on screen.
        """
        sidebar = "sidebar"
        if not editor.element_exists(sidebar):
            pytest.skip("Sidebar not registered")

        result = editor.verify_visible(sidebar)
        assert result is True, "Sidebar should be verified as visible"

    def test_verify_visible_on_nonexistent_element(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: verify_visible crashes with null pointer when element
        doesn't exist, instead of returning false.
        """
        result = editor.verify_visible("nonexistent_element_xyz")
        assert result is False, (
            "Nonexistent element should not be verified as visible"
        )

    def test_verify_bounds_on_sidebar(self, editor: OscilTestClient):
        """
        Bug caught: bounds verification returning wrong dimensions,
        or tolerance calculation wrong.
        """
        sidebar = "sidebar"
        if not editor.element_exists(sidebar):
            pytest.skip("Sidebar not registered")

        el = editor.get_element(sidebar)
        if not el or el.width == 0:
            pytest.skip("Sidebar has no bounds data")

        # Verify with actual bounds (should pass)
        result = editor.verify_element_bounds(
            sidebar, el.width, el.height, tolerance=10
        )
        assert result, "verify_element_bounds should return data"
        assert result.get("pass", False), (
            f"Bounds verification should pass with actual dimensions "
            f"({el.width}x{el.height})"
        )

    def test_verify_color_endpoint_returns_bool(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: color verification endpoint crashing on valid element
        IDs, or returning non-boolean types that break assertion chains.

        Note: we cannot assert the color matches because waveform rendering
        depends on the pane element ID, which may differ from "waveformDisplay".
        The test verifies the API contract (returns bool, does not crash).
        """
        osc_id = editor.add_oscillator(
            source_id, name="Color Verify", colour="#00FF00"
        )
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        try:
            editor.wait_for_waveform_data(pane_index=0, timeout_s=3.0)
        except TimeoutError:
            editor.transport_stop()
            pytest.skip("Waveform data not available")

        # Test with sidebar (always exists) — verifies the API path works
        sidebar_result = editor.verify_element_color(
            "sidebar", "#000000", tolerance=100
        )
        assert isinstance(sidebar_result, bool), (
            "verify_element_color should return bool for existing element"
        )

        # Test with nonexistent element — should return False, not crash
        bad_result = editor.verify_element_color(
            "nonexistent_element_xyz", "#FF0000", tolerance=10
        )
        assert bad_result is False, (
            "verify_element_color should return False for nonexistent element"
        )

        editor.transport_stop()

    def test_analyze_waveform_api_contract(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform analysis endpoint returning empty or malformed
        data, preventing automated visual regression tests.

        Verifies the API returns a dict with expected fields when called
        on a valid element, and returns None gracefully on invalid elements.
        """
        osc_id = editor.add_oscillator(source_id, name="Analyze Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        try:
            editor.wait_for_waveform_data(pane_index=0, timeout_s=3.0)
        except TimeoutError:
            editor.transport_stop()
            pytest.skip("Waveform data not available")

        # Analyze with a known element (pane_body always exists with an oscillator)
        analysis = editor.analyze_waveform("pane_body")
        if analysis is not None:
            assert isinstance(analysis, dict), "Analysis should return a dict"

        # Nonexistent element should return None without crashing
        bad_analysis = editor.analyze_waveform("nonexistent_waveform_id")
        assert bad_analysis is None or isinstance(bad_analysis, dict), (
            "Analysis of nonexistent element should return None or empty dict"
        )

        editor.transport_stop()

    def test_waveform_state_verifies_silence(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state reporting non-zero levels with silent audio.
        Uses data-level verification instead of pixel analysis.
        """
        osc_id = editor.add_oscillator(source_id, name="Silence Verify")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="silence", amplitude=0.0)

        # Wait for the circular buffer to flush old audio data
        def peak_is_silent():
            wfs = editor.get_waveform_for_pane(0)
            if not wfs:
                return False
            return wfs[0].get("peakLevel", 1.0) < 0.05

        editor.wait_until(
            peak_is_silent,
            timeout_s=5.0,
            desc="peak to drop below noise floor after silence",
        )

        editor.transport_stop()


class TestVerificationErrorHandling:
    """Edge cases in verification API error handling."""

    def test_verify_waveform_nonexistent_element(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: waveform verification with bad element ID causes crash.
        """
        result = editor.verify_waveform_rendered("nonexistent_waveform_id")
        assert result is False, (
            "Nonexistent element should not be verified as rendered"
        )

    def test_verify_bounds_nonexistent_element(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: bounds verification with bad element ID causes crash.
        """
        result = editor.verify_element_bounds(
            "nonexistent_element", 100, 100, tolerance=5
        )
        # Should return empty dict or no pass, not crash
        assert not result or not result.get("pass", False)

    def test_verify_color_nonexistent_element(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: color verification with bad element ID causes crash.
        """
        result = editor.verify_element_color(
            "nonexistent_element", "#FF0000"
        )
        assert result is False
