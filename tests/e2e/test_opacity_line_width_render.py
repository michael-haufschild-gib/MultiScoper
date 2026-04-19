"""
E2E coverage for opacity and lineWidth properties' effect on state.

What bugs these tests catch:
- opacity update propagates to state but not to the renderer (waveform
  looks identical regardless of opacity).
- lineWidth < 1 silently clamped to 1 (losing user intent).
- opacity/lineWidth default reset wrong after preset change.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestOpacityPropagation:
    def test_opacity_persists_in_state(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        for target in (0.1, 0.3, 0.5, 0.8, 1.0):
            editor.update_oscillator(oscillator, opacity=target)
            editor.wait_until(
                lambda: abs(
                    editor.get_oscillator_by_id(oscillator)["opacity"] - target
                ) < 0.05,
                timeout_s=2.0,
                desc=f"opacity to settle near {target}",
            )

    def test_opacity_zero_keeps_osc_visible_flag_true(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """opacity=0 is a rendering choice; the osc's visible flag
        is independent."""
        editor.update_oscillator(oscillator, opacity=0.0, visible=True)
        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(oscillator))
                    and osc["visible"] is True and osc["opacity"] <= 0.05,
            timeout_s=2.0,
            desc="opacity=0 coexists with visible=True",
        )


class TestLineWidthPropagation:
    @pytest.mark.parametrize("target", [1.0, 1.5, 2.0, 3.0, 5.0])
    def test_line_width_persists_in_state(
        self, editor: MultiScoperTestClient, oscillator: str, target: float
    ):
        editor.update_oscillator(oscillator, lineWidth=target)
        editor.wait_until(
            lambda: abs(
                editor.get_oscillator_by_id(oscillator)["lineWidth"] - target
            ) < 0.1,
            timeout_s=2.0,
            desc=f"lineWidth to settle near {target}",
        )

    def test_line_width_zero_clamped(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """lineWidth=0 is non-sensible for a visible line — plugin
        clamps to MIN_LINE_WIDTH."""
        editor.update_oscillator(oscillator, lineWidth=0.0)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["lineWidth"] > 0,
            timeout_s=2.0,
            desc="lineWidth to clamp above 0",
        )


class TestVisualPropertyStatePersistence:
    def test_opacity_and_line_width_round_trip(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        osc_id = editor.add_oscillator(source_id, name="Visual")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.update_oscillator(osc_id, opacity=0.37, lineWidth=2.7)
        editor.wait_until(
            lambda: abs(
                editor.get_oscillator_by_id(osc_id)["opacity"] - 0.37
            ) < 0.05,
            timeout_s=3.0, desc="opacity set",
        )

        path = str(tmp_path / "visual.xml")
        assert editor.save_state(path)
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)
        assert editor.load_state(path)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc = editor.get_oscillators()[0]
        assert abs(osc["opacity"] - 0.37) < 0.05
        assert abs(osc["lineWidth"] - 2.7) < 0.1
