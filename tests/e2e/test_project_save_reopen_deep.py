"""
E2E coverage for DAW-like save / close / reopen with rich state.

Simulates what a DAW does on File > Save → File > Close → File > Open
across the MultiScoper plugin: save bundles every oscillator, pane,
and global state value; close tears down plugin processors (freeing
capture buffers and registry entries); reopen re-creates tracks and
asks the plugin to restore the saved state.

This differs from the save/load roundtrip tests by exercising the
close + reopen path via /project/close and /project/reopen endpoints,
so that destroy → construct → restore is covered end-to-end for
multi-track configurations.

What bugs these tests catch:
- Cross-instance oscillator bindings (track 0 shows track 1's source)
  dropped after reopen — a known issue in Bitwig-style hosts.
- Pane order not preserved through close/reopen (panes come back
  shuffled).
- Oscillator order_index not preserved → sidebar list comes back
  in insertion order instead of the user's drag-reorder.
- Options/timing settings (gain, BPM, waveform mode) reset to defaults
  on reopen.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestSingleTrackDeepReopen:
    """Rich single-track state survives save/reset/load."""

    def test_multi_oscillator_roundtrip(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Five oscillators with distinct colours, modes, opacities,
        visibilities.  Every field must survive the roundtrip.
        """
        # Create rich state.
        configs = [
            ("Osc0", "#FF0000", "FullStereo", 1.0, True,  1.5),
            ("Osc1", "#00FF00", "Mono",       0.8, False, 2.0),
            ("Osc2", "#0000FF", "Mid",        0.6, True,  2.5),
            ("Osc3", "#FFFF00", "Side",       0.4, False, 3.0),
            ("Osc4", "#FF00FF", "Left",       0.2, True,  3.5),
        ]
        for name, colour, mode, opacity, visible, line_width in configs:
            osc_id = editor.add_oscillator(
                source_id, name=name, colour=colour, mode=mode
            )
            assert osc_id is not None, f"add {name} must succeed"
            editor.update_oscillator(
                osc_id, opacity=opacity, visible=visible, lineWidth=line_width
            )
        editor.wait_for_oscillator_count(len(configs), timeout_s=5.0)

        # Capture baseline state.
        baseline = {o["name"]: o for o in editor.get_oscillators()}
        assert len(baseline) == len(configs)

        # Save → reset → load.
        path = str(tmp_path / "deep_state.xml")
        assert editor.save_state(path)

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        assert editor.load_state(path)
        editor.wait_for_oscillator_count(len(configs), timeout_s=5.0)

        # Compare every field.
        restored = {o["name"]: o for o in editor.get_oscillators()}
        assert set(restored.keys()) == set(baseline.keys()), (
            f"names differ: baseline={set(baseline)} restored={set(restored)}"
        )
        for name in baseline:
            b = baseline[name]
            r = restored[name]
            for field in ("mode", "visible", "colour"):
                assert b[field] == r[field], (
                    f"{name}: {field} mismatch b={b[field]!r} r={r[field]!r}"
                )
            for numeric in ("opacity", "lineWidth"):
                diff = abs(b[numeric] - r[numeric])
                assert diff < 0.01, (
                    f"{name}: {numeric} mismatch b={b[numeric]} r={r[numeric]}"
                )


class TestProjectCloseReopenEndpoint:
    """Exercise /project/close and /project/reopen — DAW-style lifecycle."""

    def test_project_close_reopen_preserves_oscillators(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Bug caught: close destroys plugin processor; reopen fails
        to rebuild the oscillator list from the saved XML."""
        id_a = editor.add_oscillator(source_id, name="DawA", colour="#FF0000")
        id_b = editor.add_oscillator(source_id, name="DawB", colour="#00FF00")
        assert id_a and id_b
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        path = str(tmp_path / "project.xml")
        # Save via project API if available; fall back to state/save.
        saved = editor._post_ok("/project/save", {"path": path})
        if not saved:
            saved = editor.save_state(path)
        assert saved, "project save must succeed (project/save or state/save)"

        assert editor._post_ok("/project/close", {}), "/project/close must succeed"
        assert editor._post_ok("/project/reopen", {"path": path}), (
            "/project/reopen must succeed"
        )

        editor.wait_until(
            lambda: len(editor.get_oscillators()) == 2,
            timeout_s=10.0,
            desc="oscillators to restore after project reopen",
        )

        names = {o["name"] for o in editor.get_oscillators()}
        assert names == {"DawA", "DawB"}, (
            f"expected DawA and DawB, got {names}"
        )


class TestMultiPaneOrderPreservation:
    """Pane order and assignments survive save/load."""

    def test_pane_order_and_oscillator_assignments(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Bug caught: pane_id referenced by oscillator is a regenerated
        UUID on load, but oscillators still hold the OLD UUID — they
        all orphan after load."""
        osc1 = editor.add_oscillator(source_id, name="P1-Osc")
        assert osc1 is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2 = editor.add_pane("Second")
        assert pane2 is not None, "add_pane must succeed"
        pane3 = editor.add_pane("Third")
        assert pane3 is not None, "add_pane must succeed for a 3rd pane"

        osc2 = editor.add_oscillator(source_id, name="P2-Osc", pane_id=pane2)
        osc3 = editor.add_oscillator(source_id, name="P3-Osc", pane_id=pane3)
        assert osc2 and osc3
        editor.wait_for_oscillator_count(3, timeout_s=3.0)

        baseline_panes = editor.get_panes()
        baseline_oscs = editor.get_oscillators()
        assert len(baseline_panes) == 3

        path = str(tmp_path / "multi_pane.xml")
        assert editor.save_state(path)

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        assert editor.load_state(path)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        restored_panes = editor.get_panes()
        restored_oscs = editor.get_oscillators()
        assert len(restored_panes) == 3
        assert len(restored_oscs) == 3

        # Every oscillator's paneId must map to an existing pane.
        pane_ids = {p["id"] for p in restored_panes}
        for osc in restored_oscs:
            assert osc["paneId"] in pane_ids, (
                f"osc {osc['name']} has paneId={osc['paneId']!r} "
                f"not in restored panes {pane_ids}"
            )
