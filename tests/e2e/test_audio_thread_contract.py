"""
E2E coverage for audio-thread prepareToPlay (Primitive 2.4).

The plugin's `deferRegistration` (PluginProcessor.cpp:134-137) branches
on `isThisTheMessageThread()`: on the message thread it runs registration
inline, off the message thread it reposts via callAsync.  The old harness
always called prepareToPlay from the message thread, so the non-message
branch was entirely untested.

Real DAWs that hit the callAsync branch:
  - Pro Tools AAX hosts the plugin's prepare on the audio thread.
  - Reaper's 'FX in own thread' mode does the same.

`POST /daw/audioThreadPrepare {"enabled": true}` arms the TestDAW so the
subsequent setSampleRate / setBufferSize call runs prepareToPlay from a
juce::ThreadPool worker.  The tests below then hammer 20 SR toggles and
confirm the source is still visible in the registry after every pass.

Bugs caught
-----------
  - Weak-ref lifetime bug in the async registration closure (source
    disappears because the processor was destroyed before callAsync
    fired).
  - Ordering: processBlock running before deferRegistration's async
    callback finishes would leave `sourceId_` invalid on the audio
    thread's first pass.
"""

from __future__ import annotations

import pytest

from oscil_test_utils import OscilTestClient


def _arm_audio_thread_prepare(client: OscilTestClient, enabled: bool) -> None:
    resp = client._post_json("/daw/audioThreadPrepare", {"enabled": enabled})
    assert resp is not None and resp.get("success"), (
        f"Failed to set audioThreadPrepare={enabled}: {resp}"
    )
    assert resp.get("data", {}).get("enabled") is enabled


class TestAudioThreadPrepareContract:
    def test_mode_toggle_round_trip(self, client: OscilTestClient):
        """Sanity: the flag can be turned on and off via HTTP."""
        _arm_audio_thread_prepare(client, True)
        _arm_audio_thread_prepare(client, False)

    def test_twenty_sample_rate_toggles_no_crash(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        With audio-thread-prepare armed, flip between 44.1k and 48k 20
        times and assert:
          - No harness crash (raises HarnessCrashedError on first failed
            post-toggle health check)
          - The source is still registered after each toggle
          - No deadlock: the sweep completes within a sane timeout
        """
        editor.add_oscillator(source_id, name="Audio Thread Prepare")

        _arm_audio_thread_prepare(editor, True)
        try:
            for i in range(20):
                rate = 48000.0 if i % 2 == 0 else 44100.0
                resp = editor._post_json("/daw/sampleRate", {"rate": rate})
                assert resp is not None and resp.get("success"), (
                    f"Toggle #{i} to {rate} failed: {resp}"
                )

                # Source registration is async; poll.  This is the key
                # assertion — deferRegistration must successfully re-post
                # the registration onto the message thread before we see
                # `/state/sources` come back populated.
                def sources_non_empty():
                    return bool(editor.get_sources())

                editor.wait_until(
                    sources_non_empty,
                    timeout_s=5.0,
                    desc=f"sources non-empty after toggle #{i} to {rate}",
                )
        finally:
            _arm_audio_thread_prepare(editor, False)

    def test_source_id_stable_under_audio_thread_prepare(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: if `deferRegistration` ever re-registers (instead of
        calling updateSource) on the audio-thread branch, the source id
        seen by tests would change mid-sweep.  That would drop any
        oscillator bound to the old id.
        """
        _arm_audio_thread_prepare(editor, True)
        try:
            for rate in (44100.0, 48000.0, 88200.0, 96000.0):
                editor._post_json("/daw/sampleRate", {"rate": rate})
                # Stay the course: our source_id must remain visible.
                editor.wait_until(
                    lambda: source_id in {s["id"] for s in editor.get_sources()},
                    timeout_s=5.0,
                    desc=f"original source {source_id} still registered at {rate}",
                )
        finally:
            _arm_audio_thread_prepare(editor, False)
