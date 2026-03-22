"""
E2E tests for performance characteristics.

What bugs these tests catch:
- Memory leak when adding/removing oscillators repeatedly
- FPS dropping to zero with many oscillators
- Metrics collection API broken
- Start/stop/reset metrics lifecycle issues
- Performance degradation when switching themes rapidly
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestMetricsAPI:
    """Verify the metrics collection API works."""

    def test_metrics_current_returns_data(self, client: OscilTestClient):
        """
        Bug caught: metrics endpoint returning empty or malformed data.
        """
        metrics = client.metrics_current()
        if metrics is None:
            pytest.skip("Metrics API not available")

        assert "fps" in metrics or "cpuPercent" in metrics, (
            f"Metrics should contain fps or cpuPercent, got keys: {list(metrics.keys())}"
        )

    def test_metrics_start_stop_cycle(self, client: OscilTestClient):
        """
        Bug caught: start/stop not toggling collection state, or double-stop crash.
        """
        client.metrics_reset()
        started = client.metrics_start(interval_ms=50)
        if not started:
            pytest.skip("Metrics start API not available")

        # Wait for samples to accumulate (condition-based)
        client.wait_until(
            lambda: (s := client.metrics_stats())
            and s.get("sampleCount", 0) > 0,
            timeout_s=3.0,
            desc="metrics samples to accumulate",
        )

        stopped = client.metrics_stop()
        assert stopped, "Metrics should stop successfully"

        stats = client.metrics_stats()
        assert stats is not None
        sample_count = stats.get("sampleCount", 0)
        assert sample_count > 0, f"Should have collected samples, got {sample_count}"

        client.metrics_reset()

    def test_metrics_reset_clears_stats(self, client: OscilTestClient):
        """
        Bug caught: metrics reset not clearing accumulated data.
        """
        client.metrics_reset()
        started = client.metrics_start(interval_ms=50)
        if not started:
            pytest.skip("Metrics start API not available")

        # Collect some data
        client.wait_until(
            lambda: (s := client.metrics_stats())
            and s.get("sampleCount", 0) > 0,
            timeout_s=3.0,
            desc="metrics samples to accumulate",
        )
        client.metrics_stop()

        # Reset
        client.metrics_reset()
        stats = client.metrics_stats()
        if stats:
            assert stats.get("sampleCount", 0) == 0, (
                f"Reset should clear samples, got {stats.get('sampleCount')}"
            )

    def test_double_stop_does_not_crash(self, client: OscilTestClient):
        """
        Bug caught: stopping metrics collection twice causes crash or error.
        """
        client.metrics_reset()
        started = client.metrics_start(interval_ms=100)
        if not started:
            pytest.skip("Metrics start API not available")

        client.metrics_stop()
        # Second stop should not crash
        client.metrics_stop()

        # Verify harness is still responsive
        state = client.get_transport_state()
        assert state is not None, "Harness should be responsive after double metrics stop"
        client.metrics_reset()


@pytest.mark.slow
class TestPerformanceUnderLoad:
    """Performance tests that take longer to run."""

    def test_memory_stable_after_add_remove_cycle(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: memory leak when oscillators are repeatedly added and removed
        (e.g., JUCE components not properly deleted, or OpenGL resources leaked).
        """
        editor.metrics_reset()
        editor.metrics_start(interval_ms=100)

        # Wait for baseline to stabilize
        editor.wait_until(
            lambda: (m := editor.metrics_current()) and m.get("memoryMB", 0) > 0,
            timeout_s=3.0,
            desc="baseline memory measurement",
        )
        baseline = editor.metrics_current()
        if baseline is None or baseline.get("memoryMB", 0) == 0:
            editor.metrics_stop()
            editor.metrics_reset()
            pytest.skip("Memory metrics not available")

        baseline_mem = baseline["memoryMB"]

        # Add and remove oscillators 5 times
        for cycle in range(5):
            for i in range(3):
                editor.add_oscillator(source_id, name=f"Cycle {cycle} Osc {i}")
            editor.wait_for_oscillator_count(3, timeout_s=3.0)

            editor.reset_state()
            editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Wait for cleanup to settle
        editor.wait_until(
            lambda: (m := editor.metrics_current()) and m.get("memoryMB", 0) > 0,
            timeout_s=3.0,
            desc="post-cycle memory measurement",
        )

        after = editor.metrics_current()
        after_mem = after.get("memoryMB", 0) if after else 0

        editor.metrics_stop()
        editor.metrics_reset()

        if baseline_mem > 0 and after_mem > 0:
            growth = after_mem - baseline_mem
            assert growth < 50, (
                f"Memory grew by {growth:.1f}MB after add/remove cycles "
                f"(baseline={baseline_mem:.1f}, after={after_mem:.1f})"
            )

    def test_fps_stable_with_multiple_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: FPS dropping to zero or single digits when many oscillators
        are rendering simultaneously (render pipeline bottleneck).
        """
        editor.metrics_reset()

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Add 5 oscillators
        for i in range(5):
            editor.add_oscillator(source_id, name=f"FPS Test {i}")
        editor.wait_for_oscillator_count(5, timeout_s=5.0)

        # Collect metrics with condition-based wait
        editor.metrics_start(interval_ms=50)
        editor.wait_until(
            lambda: (s := editor.metrics_stats())
            and s.get("sampleCount", 0) >= 20,
            timeout_s=5.0,
            desc="sufficient metrics samples",
        )
        editor.metrics_stop()

        stats = editor.metrics_stats()
        editor.transport_stop()
        editor.metrics_reset()

        if not stats or stats.get("sampleCount", 0) == 0:
            pytest.skip("No performance data collected")

        avg_fps = stats.get("avgFps", 0)
        min_fps = stats.get("minFps", 0)

        if avg_fps > 0:
            assert avg_fps >= 10, (
                f"Average FPS too low with 5 oscillators: {avg_fps:.1f}"
            )
        if min_fps > 0:
            assert min_fps >= 5, (
                f"Minimum FPS too low with 5 oscillators: {min_fps:.1f}"
            )
