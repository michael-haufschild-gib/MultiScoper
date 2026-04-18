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
    # Fail loudly if the harness isn't reachable rather than spinning on
    # an unbounded provisioning loop below.
    c.wait_for_harness(max_retries=15, delay=1.0)

    # Ensure we have at least n tracks.
    while len(c.get_tracks()) < n_editors:
        created = c.add_track(f"probe{len(c.get_tracks())}")
        if created is None:
            raise RuntimeError(
                f"add_track failed while provisioning tracks for idle probe "
                f"(wanted {n_editors}, have {len(c.get_tracks())})"
            )

    # Silence every track's generator.
    for t in c.get_tracks():
        idx = int(t["index"])
        c.set_track_audio(idx, amplitude=0.0)
        c.reset_track_state(idx)  # remove oscillators/panes

    # Open exactly n_editors editors; close any past n. Don't assume the
    # track "index" field is a zero-based ordinal — pick the lowest-indexed
    # n_editors tracks explicitly so we can't silently benchmark the wrong
    # editor set when indices are non-contiguous.
    tracks = sorted(c.get_tracks(), key=lambda t: int(t["index"]))
    target_ids = {int(t["index"]) for t in tracks[:n_editors]}
    for t in tracks:
        idx = int(t["index"])
        if idx in target_ids:
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

    # Only run the exact-count wait if the harness actually reports
    # `editorVisible` — otherwise the wait is a no-op that would hide a
    # harness regression. When the field is present, a timeout here is a
    # real bug (wrong editors open / mismatched count), so let it propagate
    # instead of silently degrading to settle().
    refresh_tracks = c.get_tracks()
    if refresh_tracks and all("editorVisible" in t for t in refresh_tracks):
        c.wait_until(
            _exact_editor_count,
            timeout_s=5.0,
            desc=f"exactly {n_editors} editors visible before idle probe",
        )
    else:
        log.warning(
            "editorVisible missing from get_tracks(); falling back to settle() for n_editors=%d",
            n_editors,
        )

    settle(1.5, reason="post-silence tail / editor animations before idle probe")

    # Pin ResourceMonitor to the same harness URL that OscilTestClient is
    # talking to; otherwise a non-default base URL would silently sample a
    # different process than the one we just prepared.
    with ResourceMonitor(
        harness_url=c.base_url, sample_interval_s=0.5
    ) as mon:
        mon.sample_for(10.0)

    print(f"[{n_editors:2d} editors idle] {mon.report.summary()}")


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 16
    main(n)
