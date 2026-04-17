"""
Probe: silence all audio, open N editors, measure CPU for 10 seconds.
Used to compare pre/post the continuous-repaint gating change.
"""
from __future__ import annotations

import sys
import time

from oscil_test_utils import OscilTestClient
from perf_monitor import ResourceMonitor


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

    # Open exactly n_editors editors; close any past n.
    tracks = c.get_tracks()
    for t in tracks:
        idx = int(t["index"])
        if idx < n_editors:
            c.open_editor(track_id=idx)
        else:
            try:
                c.close_editor(track_id=idx)
            except Exception:
                pass

    c.transport_stop()

    # Let everything settle — the post-silence tail is 30 frames (~0.5s);
    # give it a generous margin.
    time.sleep(2.0)

    with ResourceMonitor(sample_interval_s=0.5) as mon:
        mon.sample_for(10.0)

    print(f"[{n_editors:2d} editors idle] {mon.report.summary()}")


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 16
    main(n)
