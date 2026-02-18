"""
Performance Profiler for Oscil E2E Testing

Provides comprehensive profiling capabilities including:
- GPU timing and frame rate analysis
- Thread-specific metrics
- Component repaint tracking
- Memory usage monitoring
- Automatic hotspot detection
- Performance baseline comparison
"""

import time
import json
import statistics
from dataclasses import dataclass, field
from typing import Optional, Dict, Any, List
from contextlib import contextmanager
from pathlib import Path

from .oscil_client import OscilClient, OscilClientError


@dataclass
class GpuMetrics:
    """GPU-specific performance metrics."""
    current_fps: float = 0.0
    avg_fps: float = 0.0
    min_fps: float = 0.0
    max_fps: float = 0.0
    avg_frame_time_ms: float = 0.0
    min_frame_time_ms: float = 0.0
    max_frame_time_ms: float = 0.0
    p50_frame_time_ms: float = 0.0
    p95_frame_time_ms: float = 0.0
    p99_frame_time_ms: float = 0.0
    
    # Per-pass timing
    avg_begin_frame_ms: float = 0.0
    avg_waveform_render_ms: float = 0.0
    avg_effect_pipeline_ms: float = 0.0
    avg_composite_ms: float = 0.0
    avg_blit_ms: float = 0.0
    
    # Operations
    total_fbo_binds: int = 0
    total_shader_switches: int = 0
    total_frames: int = 0
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'GpuMetrics':
        timing = data.get('timing', {})
        operations = data.get('operations', {})
        return cls(
            current_fps=data.get('currentFps', 0.0),
            avg_fps=data.get('avgFps', 0.0),
            min_fps=data.get('minFps', 0.0),
            max_fps=data.get('maxFps', 0.0),
            avg_frame_time_ms=data.get('avgFrameTimeMs', 0.0),
            min_frame_time_ms=data.get('minFrameTimeMs', 0.0),
            max_frame_time_ms=data.get('maxFrameTimeMs', 0.0),
            p50_frame_time_ms=data.get('p50FrameTimeMs', 0.0),
            p95_frame_time_ms=data.get('p95FrameTimeMs', 0.0),
            p99_frame_time_ms=data.get('p99FrameTimeMs', 0.0),
            avg_begin_frame_ms=timing.get('avgBeginFrameMs', 0.0),
            avg_waveform_render_ms=timing.get('avgWaveformRenderMs', 0.0),
            avg_effect_pipeline_ms=timing.get('avgEffectPipelineMs', 0.0),
            avg_composite_ms=timing.get('avgCompositeMs', 0.0),
            avg_blit_ms=timing.get('avgBlitMs', 0.0),
            total_fbo_binds=operations.get('totalFboBinds', 0),
            total_shader_switches=operations.get('totalShaderSwitches', 0),
            total_frames=data.get('totalFrames', 0)
        )


@dataclass
class ThreadMetrics:
    """Thread-specific performance metrics."""
    name: str = ""
    avg_execution_time_ms: float = 0.0
    max_execution_time_ms: float = 0.0
    min_execution_time_ms: float = 0.0
    invocation_count: int = 0
    load_percent: float = 0.0
    total_lock_wait_ms: float = 0.0
    lock_acquisition_count: int = 0
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ThreadMetrics':
        return cls(
            name=data.get('name', ''),
            avg_execution_time_ms=data.get('avgExecutionTimeMs', 0.0),
            max_execution_time_ms=data.get('maxExecutionTimeMs', 0.0),
            min_execution_time_ms=data.get('minExecutionTimeMs', 0.0),
            invocation_count=data.get('invocationCount', 0),
            load_percent=data.get('loadPercent', 0.0),
            total_lock_wait_ms=data.get('totalLockWaitMs', 0.0),
            lock_acquisition_count=data.get('lockAcquisitionCount', 0)
        )


@dataclass
class ComponentStats:
    """UI component repaint statistics."""
    name: str = ""
    repaint_count: int = 0
    total_repaint_time_ms: float = 0.0
    avg_repaint_time_ms: float = 0.0
    max_repaint_time_ms: float = 0.0
    repaints_per_second: float = 0.0
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ComponentStats':
        return cls(
            name=data.get('name', ''),
            repaint_count=data.get('repaintCount', 0),
            total_repaint_time_ms=data.get('totalRepaintTimeMs', 0.0),
            avg_repaint_time_ms=data.get('avgRepaintTimeMs', 0.0),
            max_repaint_time_ms=data.get('maxRepaintTimeMs', 0.0),
            repaints_per_second=data.get('repaintsPerSecond', 0.0)
        )


@dataclass
class PerformanceHotspot:
    """Performance issue identified during profiling."""
    category: str = ""  # GPU, Thread, UI, Memory
    location: str = ""
    description: str = ""
    severity: float = 0.0  # 0.0-1.0
    impact_ms: float = 0.0
    recommendation: str = ""
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PerformanceHotspot':
        return cls(
            category=data.get('category', ''),
            location=data.get('location', ''),
            description=data.get('description', ''),
            severity=data.get('severity', 0.0),
            impact_ms=data.get('impactMs', 0.0),
            recommendation=data.get('recommendation', '')
        )


@dataclass
class FrameData:
    """Single frame timing data."""
    frame_number: int = 0
    timestamp: int = 0
    total_frame_ms: float = 0.0
    begin_frame_ms: float = 0.0
    waveform_render_ms: float = 0.0
    effect_pipeline_ms: float = 0.0
    composite_ms: float = 0.0
    blit_to_screen_ms: float = 0.0
    waveform_count: int = 0
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'FrameData':
        return cls(
            frame_number=data.get('frameNumber', 0),
            timestamp=data.get('timestamp', 0),
            total_frame_ms=data.get('totalFrameMs', 0.0),
            begin_frame_ms=data.get('beginFrameMs', 0.0),
            waveform_render_ms=data.get('waveformRenderMs', 0.0),
            effect_pipeline_ms=data.get('effectPipelineMs', 0.0),
            composite_ms=data.get('compositeMs', 0.0),
            blit_to_screen_ms=data.get('blitToScreenMs', 0.0),
            waveform_count=data.get('waveformCount', 0)
        )


@dataclass
class ProfilingResult:
    """Complete profiling session result."""
    start_timestamp: int = 0
    end_timestamp: int = 0
    duration_ms: int = 0
    
    # Summary stats
    avg_fps: float = 0.0
    min_fps: float = 0.0
    max_fps: float = 0.0
    p50_frame_time_ms: float = 0.0
    p95_frame_time_ms: float = 0.0
    p99_frame_time_ms: float = 0.0
    avg_cpu: float = 0.0
    max_memory_mb: float = 0.0
    peak_memory_mb: float = 0.0
    memory_growth_mb_per_sec: float = 0.0
    
    # Detailed metrics
    gpu_metrics: Optional[GpuMetrics] = None
    audio_thread: Optional[ThreadMetrics] = None
    ui_thread: Optional[ThreadMetrics] = None
    opengl_thread: Optional[ThreadMetrics] = None
    component_stats: List[ComponentStats] = field(default_factory=list)
    hotspots: List[PerformanceHotspot] = field(default_factory=list)
    frame_timeline: List[FrameData] = field(default_factory=list)
    
    def is_healthy(self) -> bool:
        """Check if performance is acceptable (60+ FPS, no critical hotspots)."""
        if self.avg_fps < 55:
            return False
        if self.p95_frame_time_ms > 20:
            return False
        critical_hotspots = [h for h in self.hotspots if h.severity > 0.7]
        if critical_hotspots:
            return False
        return True
    
    def get_summary(self) -> str:
        """Get a human-readable summary of the profiling results."""
        lines = [
            f"=== Profiling Summary ({self.duration_ms/1000:.1f}s) ===",
            f"FPS: {self.avg_fps:.1f} avg, {self.min_fps:.1f} min, {self.max_fps:.1f} max",
            f"Frame Time: P50={self.p50_frame_time_ms:.2f}ms, P95={self.p95_frame_time_ms:.2f}ms, P99={self.p99_frame_time_ms:.2f}ms",
            f"Memory: {self.max_memory_mb:.1f} MB peak, {self.memory_growth_mb_per_sec:.3f} MB/s growth",
            f"CPU: {self.avg_cpu:.1f}% average",
        ]
        
        if self.hotspots:
            lines.append(f"\nHotspots ({len(self.hotspots)}):")
            for hs in self.hotspots[:5]:  # Top 5
                lines.append(f"  [{hs.category}] {hs.location}: {hs.description} (severity: {hs.severity:.2f})")
        
        return "\n".join(lines)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        return {
            'startTimestamp': self.start_timestamp,
            'endTimestamp': self.end_timestamp,
            'durationMs': self.duration_ms,
            'avgFps': self.avg_fps,
            'minFps': self.min_fps,
            'maxFps': self.max_fps,
            'p50FrameTimeMs': self.p50_frame_time_ms,
            'p95FrameTimeMs': self.p95_frame_time_ms,
            'p99FrameTimeMs': self.p99_frame_time_ms,
            'avgCpu': self.avg_cpu,
            'maxMemoryMB': self.max_memory_mb,
            'peakMemoryMB': self.peak_memory_mb,
            'memoryGrowthMBPerSec': self.memory_growth_mb_per_sec,
            'hotspotCount': len(self.hotspots),
            'isHealthy': self.is_healthy()
        }


class PerformanceProfiler:
    """
    Performance profiler for Oscil E2E testing.
    
    Usage:
        profiler = PerformanceProfiler()
        
        with profiler.session(duration=5.0) as result:
            # result contains ProfilingResult after session completes
            print(result.get_summary())
        
        # Or manual control:
        profiler.start()
        time.sleep(5)
        result = profiler.stop()
    """
    
    def __init__(self, client: Optional[OscilClient] = None):
        """
        Initialize the profiler.
        
        Args:
            client: OscilClient instance (creates one if not provided)
        """
        self.client = client or OscilClient()
        self._is_profiling = False
        self._start_time = 0
    
    def start(self) -> bool:
        """Start a profiling session."""
        try:
            response = self.client._post("/profiling/start")
            self._is_profiling = response.get('data', {}).get('profiling', False)
            self._start_time = time.time()
            return self._is_profiling
        except OscilClientError as e:
            print(f"Failed to start profiling: {e}")
            return False
    
    def stop(self) -> ProfilingResult:
        """Stop profiling and return results."""
        try:
            response = self.client._post("/profiling/stop")
            self._is_profiling = False
            
            data = response.get('data', {})
            summary = data.get('summary', {})
            
            result = ProfilingResult(
                start_timestamp=int(self._start_time * 1000),
                end_timestamp=int(time.time() * 1000),
                duration_ms=data.get('durationMs', 0),
                avg_fps=summary.get('avgFps', 0.0),
                min_fps=summary.get('minFps', 0.0),
                max_fps=summary.get('maxFps', 0.0),
                p50_frame_time_ms=summary.get('p50FrameTimeMs', 0.0),
                p95_frame_time_ms=summary.get('p95FrameTimeMs', 0.0),
                p99_frame_time_ms=summary.get('p99FrameTimeMs', 0.0),
                avg_cpu=summary.get('avgCpu', 0.0),
                max_memory_mb=summary.get('maxMemoryMB', 0.0),
                peak_memory_mb=summary.get('peakMemoryMB', 0.0),
                memory_growth_mb_per_sec=summary.get('memoryGrowthMBPerSec', 0.0)
            )
            
            # Fetch detailed metrics
            result.gpu_metrics = self.get_gpu_metrics()
            result.audio_thread, result.ui_thread, result.opengl_thread = self._get_thread_metrics()
            result.component_stats = self.get_component_stats()
            result.hotspots = self.get_hotspots()
            result.frame_timeline = self.get_frame_timeline()
            
            return result
            
        except OscilClientError as e:
            print(f"Failed to stop profiling: {e}")
            return ProfilingResult()
    
    def get_snapshot(self) -> Dict[str, Any]:
        """Get current profiling snapshot without stopping."""
        try:
            response = self.client._get("/profiling/snapshot")
            return response.get('data', {})
        except OscilClientError:
            return {}
    
    def get_gpu_metrics(self) -> GpuMetrics:
        """Get GPU-specific metrics."""
        try:
            response = self.client._get("/profiling/gpu")
            return GpuMetrics.from_dict(response.get('data', {}))
        except OscilClientError:
            return GpuMetrics()
    
    def _get_thread_metrics(self) -> tuple:
        """Get thread metrics for all threads."""
        try:
            response = self.client._get("/profiling/threads")
            data = response.get('data', {})
            return (
                ThreadMetrics.from_dict(data.get('audioThread', {})),
                ThreadMetrics.from_dict(data.get('uiThread', {})),
                ThreadMetrics.from_dict(data.get('openglThread', {}))
            )
        except OscilClientError:
            return (ThreadMetrics(), ThreadMetrics(), ThreadMetrics())
    
    def get_component_stats(self) -> List[ComponentStats]:
        """Get component repaint statistics."""
        try:
            response = self.client._get("/profiling/components")
            components = response.get('data', {}).get('components', [])
            return [ComponentStats.from_dict(c) for c in components]
        except OscilClientError:
            return []
    
    def get_hotspots(self) -> List[PerformanceHotspot]:
        """Get identified performance hotspots."""
        try:
            response = self.client._get("/profiling/hotspots")
            hotspots = response.get('data', {}).get('hotspots', [])
            return [PerformanceHotspot.from_dict(h) for h in hotspots]
        except OscilClientError:
            return []
    
    def get_frame_timeline(self, frame_count: int = 60) -> List[FrameData]:
        """Get frame timeline data."""
        try:
            response = self.client._get(f"/profiling/timeline", params={'frames': frame_count})
            frames = response.get('data', {}).get('frames', [])
            return [FrameData.from_dict(f) for f in frames]
        except OscilClientError:
            return []
    
    @contextmanager
    def session(self, duration: float = 5.0, warmup: float = 1.0):
        """
        Context manager for a profiling session.
        
        Args:
            duration: How long to profile in seconds
            warmup: Warmup period before profiling starts
            
        Yields:
            ProfilingResult after session completes
        """
        # Warmup period
        if warmup > 0:
            time.sleep(warmup)
        
        # Start profiling
        self.start()
        
        # Wait for duration
        time.sleep(duration)
        
        # Stop and yield result
        result = self.stop()
        yield result
    
    def is_profiling(self) -> bool:
        """Check if profiling is active."""
        return self._is_profiling


class ProfileSession:
    """
    RAII-style profiling session.
    
    Usage:
        with ProfileSession(client) as session:
            # Do operations
            pass
        
        print(session.result.get_summary())
    """
    
    def __init__(self, client: Optional[OscilClient] = None, duration: float = 5.0):
        self.profiler = PerformanceProfiler(client)
        self.duration = duration
        self.result: Optional[ProfilingResult] = None
    
    def __enter__(self) -> 'ProfileSession':
        self.profiler.start()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        time.sleep(self.duration)
        self.result = self.profiler.stop()
        return False


class PerformanceBaseline:
    """
    Manages performance baselines for regression detection.
    """
    
    def __init__(self, baseline_dir: Optional[Path] = None):
        self.baseline_dir = baseline_dir or Path(__file__).parent / "baselines" / "performance"
        self.baseline_dir.mkdir(parents=True, exist_ok=True)
    
    def save_baseline(self, name: str, result: ProfilingResult):
        """Save a profiling result as a baseline."""
        baseline_file = self.baseline_dir / f"{name}.json"
        with open(baseline_file, 'w') as f:
            json.dump(result.to_dict(), f, indent=2)
    
    def load_baseline(self, name: str) -> Optional[Dict[str, Any]]:
        """Load a baseline by name."""
        baseline_file = self.baseline_dir / f"{name}.json"
        if baseline_file.exists():
            with open(baseline_file, 'r') as f:
                return json.load(f)
        return None
    
    def compare_to_baseline(self, name: str, result: ProfilingResult, tolerance: float = 0.1) -> Dict[str, Any]:
        """
        Compare a profiling result to a baseline.
        
        Args:
            name: Baseline name
            result: Current profiling result
            tolerance: Acceptable deviation (0.1 = 10%)
            
        Returns:
            Comparison results with regressions/improvements
        """
        baseline = self.load_baseline(name)
        if not baseline:
            return {'error': f"Baseline '{name}' not found"}
        
        comparisons = {}
        current = result.to_dict()
        
        for key in ['avgFps', 'p95FrameTimeMs', 'maxMemoryMB']:
            if key in baseline and key in current:
                baseline_val = baseline[key]
                current_val = current[key]
                
                if baseline_val == 0:
                    continue
                
                change = (current_val - baseline_val) / baseline_val
                
                # For FPS, higher is better; for frame time/memory, lower is better
                if key == 'avgFps':
                    is_regression = change < -tolerance
                    is_improvement = change > tolerance
                else:
                    is_regression = change > tolerance
                    is_improvement = change < -tolerance
                
                comparisons[key] = {
                    'baseline': baseline_val,
                    'current': current_val,
                    'change': change,
                    'changePercent': change * 100,
                    'isRegression': is_regression,
                    'isImprovement': is_improvement
                }
        
        has_regressions = any(c.get('isRegression', False) for c in comparisons.values())
        has_improvements = any(c.get('isImprovement', False) for c in comparisons.values())
        
        return {
            'baselineName': name,
            'comparisons': comparisons,
            'hasRegressions': has_regressions,
            'hasImprovements': has_improvements,
            'overallStatus': 'regression' if has_regressions else ('improvement' if has_improvements else 'stable')
        }


def quick_profile(duration: float = 5.0, warmup: float = 1.0) -> ProfilingResult:
    """
    Quick profiling helper function.
    
    Args:
        duration: Profiling duration in seconds
        warmup: Warmup period in seconds
        
    Returns:
        ProfilingResult
    """
    profiler = PerformanceProfiler()
    
    if warmup > 0:
        time.sleep(warmup)
    
    profiler.start()
    time.sleep(duration)
    return profiler.stop()

