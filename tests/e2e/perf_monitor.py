"""
Performance / resource monitoring for Oscil test harness.

Measures the harness process from the outside (psutil) for OS-level truth
about RSS, CPU, and thread count, and combines that with the harness's
internal /metrics endpoint for FPS (a proxy for GPU throughput when a
visualization workload is active).

The point of this module is to catch the three failure modes the 16-instance
tests are meant to find:

  1. Memory leaks — RSS growing unbounded across repeated workload cycles.
  2. CPU hogs — sustained CPU% exceeding a threshold while idle or under
     a steady workload.
  3. GPU / render hogs — FPS collapsing or frame time ballooning under load.

GPU memory is not directly observable on macOS without Metal counters; we
use FPS under sustained visualization load as the practical proxy.
"""
from __future__ import annotations

import statistics
import time
from contextlib import contextmanager
from dataclasses import dataclass, field
from typing import Iterator, List, Optional

import psutil
import requests


DEFAULT_HARNESS_PORT = 8765
HARNESS_URL = f"http://localhost:{DEFAULT_HARNESS_PORT}"


HARNESS_PROCESS_NAME = "Oscil Test Harness"


def find_harness_pid(
    port: int = DEFAULT_HARNESS_PORT, harness_url: Optional[str] = None
) -> Optional[int]:
    """Find the PID of the running harness.

    Tries two strategies in order of reliability:

    1. Ask the harness itself via /health — the server reports its own PID.
       This is the most accurate route and works without elevated privileges.
    2. Fall back to scanning processes by name.  ``psutil.net_connections``
       requires root on macOS, so we avoid it entirely.

    ``harness_url`` overrides the default so callers running against a
    non-default port / host attach to the correct process. ``port`` is kept
    for backwards compatibility and used only when ``harness_url`` is None.

    The name-scan fallback cannot disambiguate between multiple harnesses on
    different ports/hosts, so it only runs for the default local harness.
    For non-default targets we return ``None`` on health failure — attaching
    to the wrong process would silently make the perf report meaningless.
    """
    url = harness_url if harness_url else f"http://localhost:{port}"
    try:
        resp = requests.get(f"{url}/health", timeout=2.0).json()
        pid = resp.get("data", {}).get("pid")
        if isinstance(pid, int) and pid > 0 and psutil.pid_exists(pid):
            return pid
    except (requests.RequestException, ValueError):
        pass

    # Only scan processes by name when the caller is targeting the default
    # local harness — any custom URL/port implies multiple harnesses could be
    # running and a name scan cannot distinguish them.
    if url.rstrip("/") != HARNESS_URL:
        return None

    for proc in psutil.process_iter(["pid", "name"]):
        if proc.info.get("name") == HARNESS_PROCESS_NAME:
            return proc.info["pid"]
    return None


@dataclass
class Sample:
    t: float
    rss_mb: float
    cpu_percent: float
    num_threads: int
    fps: float = 0.0
    harness_cpu: float = 0.0
    harness_mem_mb: float = 0.0
    osc_count: int = 0
    source_count: int = 0


@dataclass
class ResourceReport:
    samples: List[Sample] = field(default_factory=list)
    harness_url: str = HARNESS_URL

    # ---------------------------------------------------------------
    # Derived statistics
    # ---------------------------------------------------------------
    def duration_s(self) -> float:
        if len(self.samples) < 2:
            return 0.0
        return self.samples[-1].t - self.samples[0].t

    def rss_delta_mb(self) -> float:
        """Change in resident memory from first to last sample."""
        if len(self.samples) < 2:
            return 0.0
        return self.samples[-1].rss_mb - self.samples[0].rss_mb

    def rss_peak_mb(self) -> float:
        return max((s.rss_mb for s in self.samples), default=0.0)

    def rss_min_mb(self) -> float:
        return min((s.rss_mb for s in self.samples), default=0.0)

    def cpu_percent_avg(self) -> float:
        if not self.samples:
            return 0.0
        # Skip the first sample — psutil's cpu_percent() returns 0 the first
        # time it is called on a new Process handle.
        meaningful = self.samples[1:] or self.samples
        return statistics.mean(s.cpu_percent for s in meaningful)

    def cpu_percent_p95(self) -> float:
        vals = sorted(s.cpu_percent for s in self.samples[1:] or self.samples)
        if not vals:
            return 0.0
        idx = min(int(len(vals) * 0.95), len(vals) - 1)
        return vals[idx]

    def cpu_percent_peak(self) -> float:
        if len(self.samples) < 2:
            return 0.0
        return max(s.cpu_percent for s in self.samples[1:])

    def fps_avg(self) -> float:
        # Include zero-fps samples: `sample()` leaves fps == 0 when
        # /metrics/current fails, and a real render stall also produces
        # zero. Dropping those would let assert_fps_above() pass on a
        # handful of healthy samples while the rest of the run had no
        # usable frames.
        if not self.samples:
            return 0.0
        return statistics.mean(s.fps for s in self.samples)

    def fps_min(self) -> float:
        return min((s.fps for s in self.samples), default=0.0)

    def thread_peak(self) -> int:
        return max((s.num_threads for s in self.samples), default=0)

    def summary(self) -> str:
        return (
            f"duration={self.duration_s():.1f}s  "
            f"rss=[{self.rss_min_mb():.1f},{self.rss_peak_mb():.1f}]MB  "
            f"Δrss={self.rss_delta_mb():+.1f}MB  "
            f"cpu_avg={self.cpu_percent_avg():.1f}%  "
            f"cpu_p95={self.cpu_percent_p95():.1f}%  "
            f"cpu_peak={self.cpu_percent_peak():.1f}%  "
            f"fps_avg={self.fps_avg():.1f}  "
            f"fps_min={self.fps_min():.1f}  "
            f"threads_peak={self.thread_peak()}  "
            f"samples={len(self.samples)}"
        )


class ResourceMonitor:
    """Samples the harness process + harness /metrics in a short blocking loop.

    Usage::

        with ResourceMonitor() as mon:
            do_workload()
        report = mon.report  # ResourceReport
    """

    def __init__(self, harness_url: str = HARNESS_URL, sample_interval_s: float = 0.25):
        self.harness_url = harness_url.rstrip("/")
        self.sample_interval_s = sample_interval_s
        self.report = ResourceReport(harness_url=self.harness_url)
        self._proc: Optional[psutil.Process] = None
        self._running = False

    def __enter__(self) -> "ResourceMonitor":
        pid = find_harness_pid(harness_url=self.harness_url)
        if pid is None:
            raise RuntimeError(
                f"Could not find harness PID via {self.harness_url} — is the harness running?"
            )
        self._proc = psutil.Process(pid)
        # Prime cpu_percent for delta sampling.
        self._proc.cpu_percent(interval=None)
        # Reset harness-internal metrics so the FPS window is fresh.
        try:
            requests.post(f"{self.harness_url}/metrics/reset", timeout=2.0)
            requests.post(
                f"{self.harness_url}/metrics/start",
                json={"intervalMs": 100},
                timeout=2.0,
            )
        except requests.RequestException:
            pass
        self._running = True
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        try:
            requests.post(f"{self.harness_url}/metrics/stop", timeout=2.0)
        except requests.RequestException:
            pass
        self._running = False

    def sample(self) -> Sample:
        """Take one sample. Call between workload steps."""
        if self._proc is None:
            raise RuntimeError("ResourceMonitor not started — use as context manager")
        try:
            rss_mb = self._proc.memory_info().rss / (1024 * 1024)
            cpu = self._proc.cpu_percent(interval=None)
            threads = self._proc.num_threads()
        except psutil.NoSuchProcess:
            raise RuntimeError(
                "Harness process disappeared during monitoring — likely a crash."
            ) from None
        fps = 0.0
        harness_cpu = 0.0
        harness_mem_mb = 0.0
        osc_count = 0
        source_count = 0
        try:
            resp = requests.get(f"{self.harness_url}/metrics/current", timeout=2.0)
            if resp.status_code == 200:
                m = resp.json().get("data", {})
                fps = float(m.get("fps") or 0.0)
                harness_cpu = float(m.get("cpuPercent") or 0.0)
                harness_mem_mb = float(m.get("memoryMB") or 0.0)
                osc_count = int(m.get("oscillatorCount") or 0)
                source_count = int(m.get("sourceCount") or 0)
        except requests.RequestException:
            pass
        s = Sample(
            t=time.time(),
            rss_mb=rss_mb,
            cpu_percent=cpu,
            num_threads=threads,
            fps=fps,
            harness_cpu=harness_cpu,
            harness_mem_mb=harness_mem_mb,
            osc_count=osc_count,
            source_count=source_count,
        )
        self.report.samples.append(s)
        return s

    def sample_for(self, seconds: float) -> None:
        """Block and sample for a fixed duration."""
        t_end = time.time() + seconds
        while time.time() < t_end:
            self.sample()
            time.sleep(self.sample_interval_s)


@contextmanager
def monitor_resources(
    harness_url: str = HARNESS_URL,
    sample_interval_s: float = 0.25,
) -> Iterator[ResourceMonitor]:
    """Convenience wrapper: with monitor_resources() as mon: ..."""
    with ResourceMonitor(
        harness_url=harness_url, sample_interval_s=sample_interval_s
    ) as mon:
        yield mon


# ---------------------------------------------------------------------------
# Assertions
# ---------------------------------------------------------------------------
def assert_no_memory_leak(report: ResourceReport, max_growth_mb: float) -> None:
    """Fail if RSS grew more than max_growth_mb from first to last sample."""
    delta = report.rss_delta_mb()
    assert delta <= max_growth_mb, (
        f"Memory leak suspected: RSS grew {delta:.1f} MB over "
        f"{report.duration_s():.1f}s (threshold {max_growth_mb:.1f} MB). "
        f"Summary: {report.summary()}"
    )


def assert_cpu_below(report: ResourceReport, max_avg_percent: float) -> None:
    """Fail if average CPU% across the run exceeded max_avg_percent."""
    avg = report.cpu_percent_avg()
    assert avg <= max_avg_percent, (
        f"CPU hog: avg CPU {avg:.1f}% exceeds threshold {max_avg_percent:.1f}%. "
        f"Summary: {report.summary()}"
    )


def assert_fps_above(report: ResourceReport, min_fps: float) -> None:
    """Fail if the average FPS reported by the harness fell below min_fps."""
    fps = report.fps_avg()
    assert fps >= min_fps, (
        f"GPU/render hog: avg FPS {fps:.1f} below threshold {min_fps:.1f}. "
        f"Summary: {report.summary()}"
    )
