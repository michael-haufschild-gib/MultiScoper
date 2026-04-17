"""
Probe: silence all audio, open N editors, measure CPU for 10 seconds.
Used to compare pre/post the continuous-repaint gating change.
"""
from __future__ import annotations

import logging
import sys

from oscil_test_utils import OscilTestClient, settle
from perf_monitor import ResourceMonitor

log = logging.getLogger(__name__)


def main(n_editors: int) -> None:
    c = OscilTestClient()

    # Ensure we have at least n tracks.
    while len(c.get_tracks()) < n_editors:
        c.add_track(f"probe{len(c.get_tracks())}")

    # Silence every track's generator.
    for t in c.get_tracks():
        idx = int(t["index"])
        c.set_track_audio(idx, amplitude=0.0)
        c.reset_track_state(idx)  # remove oscillators/panes

    # Open exactly n_editors editors; close any past n. Swallowing the
    # close-editor error would let us measure more editors than the user
    # asked for; re-validate the open-editor count before sampling.
    tracks = c.get_tracks()
    for t in tracks:
        idx = int(t["index"])
        if idx < n_editors:
            c.open_editor(track_id=idx)
        else:
            try:
                c.close_editor(track_id=idx)
            except Exception as exc:
                msg = str(exc).lower()
                if "not found" not in msg and "already" not in msg:
                    raise

    c.transport_stop()

    def _exact_editor_count() -> bool:
        open_editors = sum(
            1 for t in c.get_tracks() if bool(t.get("editorVisible", False))
        )
        return open_editors == n_editors

    # Fallback if the harness doesn't expose editorVisible in get_tracks:
    # we still rely on settle() to damp animation tails. wait_until is
    # infrastructure-friendly and avoids hard-coded sleeps.
    try:
        c.wait_until(
            _exact_editor_count,
            timeout_s=5.0,
            desc=f"exactly {n_editors} editors visible before idle probe",
        )
    except Exception as exc:
        # Field may not exist; fall back to settle() for animation damping.
        # Log so test flakiness is diagnosable instead of silently degrading.
        log.warning(
            "editor-count wait failed for n_editors=%d (%s); falling back to settle()",
            n_editors,
            exc,
        )

    settle(1.5, reason="post-silence tail / editor animations before idle probe")

    with ResourceMonitor(sample_interval_s=0.5) as mon:
        mon.sample_for(10.0)

    print(f"[{n_editors:2d} editors idle] {mon.report.summary()}")


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 16
    main(n)
