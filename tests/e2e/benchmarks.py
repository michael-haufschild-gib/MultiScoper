"""
Performance Benchmarks for Oscil E2E Testing

Provides standardized benchmark scenarios for:
- Baseline performance measurement
- Regression detection
- Scalability testing
- GPU vs CPU comparison
"""

import time
import json
from dataclasses import dataclass, field
from typing import Optional, Dict, Any, List, Callable
from pathlib import Path
from datetime import datetime

from .oscil_client import OscilClient, OscilClientError
from .performance_profiler import PerformanceProfiler, ProfilingResult, PerformanceBaseline
from .performance_analyzer import PerformanceAnalyzer, AnalysisReport


@dataclass
class BenchmarkConfig:
    """Configuration for a benchmark scenario."""
    name: str
    description: str = ""
    oscillator_count: int = 1
    effects_enabled: bool = False
    gpu_rendering: bool = True
    warmup_seconds: float = 2.0
    duration_seconds: float = 5.0
    audio_enabled: bool = True


@dataclass
class BenchmarkResult:
    """Result of a single benchmark run."""
    config: BenchmarkConfig
    profiling_result: ProfilingResult
    analysis_report: Optional[AnalysisReport] = None
    passed: bool = True
    failure_reason: str = ""
    
    # Derived metrics for easy comparison
    avg_fps: float = 0.0
    p95_frame_time_ms: float = 0.0
    peak_memory_mb: float = 0.0
    
    def __post_init__(self):
        if self.profiling_result:
            self.avg_fps = self.profiling_result.avg_fps
            self.p95_frame_time_ms = self.profiling_result.p95_frame_time_ms
            self.peak_memory_mb = self.profiling_result.peak_memory_mb
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            'benchmarkName': self.config.name,
            'oscillatorCount': self.config.oscillator_count,
            'effectsEnabled': self.config.effects_enabled,
            'gpuRendering': self.config.gpu_rendering,
            'durationSeconds': self.config.duration_seconds,
            'passed': self.passed,
            'failureReason': self.failure_reason,
            'avgFps': self.avg_fps,
            'p95FrameTimeMs': self.p95_frame_time_ms,
            'peakMemoryMB': self.peak_memory_mb
        }


@dataclass
class BenchmarkSuiteResult:
    """Result of running a complete benchmark suite."""
    suite_name: str
    timestamp: int = 0
    total_duration_ms: int = 0
    results: List[BenchmarkResult] = field(default_factory=list)
    
    @property
    def passed_count(self) -> int:
        return sum(1 for r in self.results if r.passed)
    
    @property
    def failed_count(self) -> int:
        return sum(1 for r in self.results if not r.passed)
    
    @property
    def all_passed(self) -> bool:
        return all(r.passed for r in self.results)
    
    def get_summary(self) -> str:
        lines = [
            f"=== Benchmark Suite: {self.suite_name} ===",
            f"Total benchmarks: {len(self.results)}",
            f"Passed: {self.passed_count}",
            f"Failed: {self.failed_count}",
            f"Duration: {self.total_duration_ms / 1000:.1f}s",
            ""
        ]
        
        for result in self.results:
            status = "✓" if result.passed else "✗"
            lines.append(
                f"  {status} {result.config.name}: {result.avg_fps:.1f} FPS, "
                f"P95={result.p95_frame_time_ms:.1f}ms"
            )
            if not result.passed:
                lines.append(f"      Reason: {result.failure_reason}")
        
        return "\n".join(lines)
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            'suiteName': self.suite_name,
            'timestamp': self.timestamp,
            'totalDurationMs': self.total_duration_ms,
            'passedCount': self.passed_count,
            'failedCount': self.failed_count,
            'allPassed': self.all_passed,
            'results': [r.to_dict() for r in self.results]
        }


# ============================================================================
# Standard Benchmark Configurations
# ============================================================================

BENCHMARK_BASELINE = BenchmarkConfig(
    name="baseline",
    description="Single oscillator, no effects, GPU rendering",
    oscillator_count=1,
    effects_enabled=False,
    gpu_rendering=True
)

BENCHMARK_5_OSCILLATORS = BenchmarkConfig(
    name="5_oscillators",
    description="5 oscillators, no effects, GPU rendering",
    oscillator_count=5,
    effects_enabled=False,
    gpu_rendering=True
)

BENCHMARK_10_OSCILLATORS = BenchmarkConfig(
    name="10_oscillators",
    description="10 oscillators, no effects, GPU rendering",
    oscillator_count=10,
    effects_enabled=False,
    gpu_rendering=True
)

BENCHMARK_WITH_EFFECTS = BenchmarkConfig(
    name="with_effects",
    description="Single oscillator with all effects enabled",
    oscillator_count=1,
    effects_enabled=True,
    gpu_rendering=True
)

BENCHMARK_HEAVY_LOAD = BenchmarkConfig(
    name="heavy_load",
    description="10 oscillators with effects",
    oscillator_count=10,
    effects_enabled=True,
    gpu_rendering=True
)

BENCHMARK_CPU_RENDERING = BenchmarkConfig(
    name="cpu_rendering",
    description="Single oscillator, CPU rendering only",
    oscillator_count=1,
    effects_enabled=False,
    gpu_rendering=False
)

# Standard suite configurations
QUICK_SUITE = [BENCHMARK_BASELINE, BENCHMARK_5_OSCILLATORS]
STANDARD_SUITE = [BENCHMARK_BASELINE, BENCHMARK_5_OSCILLATORS, BENCHMARK_10_OSCILLATORS, BENCHMARK_WITH_EFFECTS]
FULL_SUITE = [BENCHMARK_BASELINE, BENCHMARK_5_OSCILLATORS, BENCHMARK_10_OSCILLATORS, 
              BENCHMARK_WITH_EFFECTS, BENCHMARK_HEAVY_LOAD, BENCHMARK_CPU_RENDERING]


class PerformanceBenchmark:
    """
    Runs standardized performance benchmarks.
    
    Usage:
        benchmark = PerformanceBenchmark()
        result = benchmark.run(BENCHMARK_BASELINE)
        
        # Or run a suite:
        suite_result = benchmark.run_suite(STANDARD_SUITE, "standard")
    """
    
    # Performance thresholds for pass/fail
    MIN_FPS = 55.0
    MAX_P95_FRAME_TIME_MS = 25.0
    MAX_MEMORY_MB = 500.0
    
    def __init__(self, client: Optional[OscilClient] = None):
        self.client = client or OscilClient()
        self.profiler = PerformanceProfiler(self.client)
        self.analyzer = PerformanceAnalyzer()
        self.baseline_manager = PerformanceBaseline()
    
    def run(self, config: BenchmarkConfig) -> BenchmarkResult:
        """
        Run a single benchmark.
        
        Args:
            config: Benchmark configuration
            
        Returns:
            BenchmarkResult with profiling data
        """
        try:
            # Setup
            self._setup_benchmark(config)
            
            # Warmup
            time.sleep(config.warmup_seconds)
            
            # Profile
            self.profiler.start()
            time.sleep(config.duration_seconds)
            profiling_result = self.profiler.stop()
            
            # Analyze
            analysis_report = self.analyzer.analyze(profiling_result)
            
            # Create result
            result = BenchmarkResult(
                config=config,
                profiling_result=profiling_result,
                analysis_report=analysis_report
            )
            
            # Check pass/fail criteria
            result.passed, result.failure_reason = self._check_criteria(result)
            
            # Cleanup
            self._cleanup_benchmark()
            
            return result
            
        except Exception as e:
            return BenchmarkResult(
                config=config,
                profiling_result=ProfilingResult(),
                passed=False,
                failure_reason=f"Benchmark failed with exception: {str(e)}"
            )
    
    def run_suite(self, configs: List[BenchmarkConfig], suite_name: str = "custom") -> BenchmarkSuiteResult:
        """
        Run a suite of benchmarks.
        
        Args:
            configs: List of benchmark configurations
            suite_name: Name for the suite
            
        Returns:
            BenchmarkSuiteResult with all results
        """
        start_time = time.time()
        
        suite_result = BenchmarkSuiteResult(
            suite_name=suite_name,
            timestamp=int(start_time * 1000)
        )
        
        for config in configs:
            print(f"Running benchmark: {config.name}...")
            result = self.run(config)
            suite_result.results.append(result)
            
            # Brief pause between benchmarks
            time.sleep(1.0)
        
        end_time = time.time()
        suite_result.total_duration_ms = int((end_time - start_time) * 1000)
        
        return suite_result
    
    def run_quick_suite(self) -> BenchmarkSuiteResult:
        """Run the quick benchmark suite."""
        return self.run_suite(QUICK_SUITE, "quick")
    
    def run_standard_suite(self) -> BenchmarkSuiteResult:
        """Run the standard benchmark suite."""
        return self.run_suite(STANDARD_SUITE, "standard")
    
    def run_full_suite(self) -> BenchmarkSuiteResult:
        """Run the full benchmark suite."""
        return self.run_suite(FULL_SUITE, "full")
    
    def compare_gpu_vs_cpu(self) -> Dict[str, Any]:
        """
        Compare GPU vs CPU rendering performance.
        
        Returns:
            Comparison results
        """
        gpu_config = BenchmarkConfig(
            name="gpu_comparison",
            oscillator_count=5,
            effects_enabled=False,
            gpu_rendering=True,
            duration_seconds=3.0
        )
        
        cpu_config = BenchmarkConfig(
            name="cpu_comparison",
            oscillator_count=5,
            effects_enabled=False,
            gpu_rendering=False,
            duration_seconds=3.0
        )
        
        gpu_result = self.run(gpu_config)
        cpu_result = self.run(cpu_config)
        
        fps_improvement = ((gpu_result.avg_fps - cpu_result.avg_fps) / cpu_result.avg_fps * 100) if cpu_result.avg_fps > 0 else 0
        
        return {
            'gpu': {
                'avgFps': gpu_result.avg_fps,
                'p95FrameTimeMs': gpu_result.p95_frame_time_ms,
                'peakMemoryMB': gpu_result.peak_memory_mb
            },
            'cpu': {
                'avgFps': cpu_result.avg_fps,
                'p95FrameTimeMs': cpu_result.p95_frame_time_ms,
                'peakMemoryMB': cpu_result.peak_memory_mb
            },
            'fpsImprovementPercent': fps_improvement,
            'recommendation': 'GPU' if gpu_result.avg_fps > cpu_result.avg_fps else 'CPU'
        }
    
    def scalability_test(self, max_oscillators: int = 20, step: int = 5) -> List[Dict[str, Any]]:
        """
        Test how performance scales with oscillator count.
        
        Args:
            max_oscillators: Maximum number of oscillators to test
            step: Step size for oscillator count
            
        Returns:
            List of results for each oscillator count
        """
        results = []
        
        for count in range(1, max_oscillators + 1, step):
            config = BenchmarkConfig(
                name=f"scale_{count}_osc",
                oscillator_count=count,
                effects_enabled=False,
                gpu_rendering=True,
                duration_seconds=3.0
            )
            
            result = self.run(config)
            results.append({
                'oscillatorCount': count,
                'avgFps': result.avg_fps,
                'p95FrameTimeMs': result.p95_frame_time_ms,
                'peakMemoryMB': result.peak_memory_mb,
                'passed': result.passed
            })
            
            # Stop if performance becomes unacceptable
            if result.avg_fps < 30:
                break
        
        return results
    
    def _setup_benchmark(self, config: BenchmarkConfig):
        """Set up the benchmark environment."""
        # Reset state
        self.client.reset()
        time.sleep(0.5)
        
        # Open editor
        self.client.show_editor()
        time.sleep(0.5)
        
        # Set GPU mode
        self.client.set_gpu_enabled(config.gpu_rendering)
        time.sleep(0.3)
        
        # Add oscillators
        for i in range(config.oscillator_count):
            self.client.add_oscillator(f"Bench Osc {i+1}", "input_1_2", "pane_0")
            time.sleep(0.1)
        
        # Enable effects if needed
        if config.effects_enabled:
            try:
                self.client._post("/stress/effects", {"enableAll": True, "quality": "high"})
            except OscilClientError:
                pass  # Endpoint may not be available
        
        # Start audio if needed
        if config.audio_enabled:
            try:
                self.client._post("/stress/audio", {
                    "durationMs": int((config.warmup_seconds + config.duration_seconds + 2) * 1000),
                    "waveform": "sine",
                    "frequency": 440.0,
                    "amplitude": 0.5
                })
            except OscilClientError:
                pass
    
    def _cleanup_benchmark(self):
        """Clean up after benchmark."""
        try:
            self.client.reset()
        except OscilClientError:
            pass
    
    def _check_criteria(self, result: BenchmarkResult) -> tuple:
        """Check if benchmark result meets pass criteria."""
        reasons = []
        
        if result.avg_fps < self.MIN_FPS:
            reasons.append(f"FPS too low: {result.avg_fps:.1f} < {self.MIN_FPS}")
        
        if result.p95_frame_time_ms > self.MAX_P95_FRAME_TIME_MS:
            reasons.append(f"P95 frame time too high: {result.p95_frame_time_ms:.1f}ms > {self.MAX_P95_FRAME_TIME_MS}ms")
        
        if result.peak_memory_mb > self.MAX_MEMORY_MB:
            reasons.append(f"Memory too high: {result.peak_memory_mb:.1f}MB > {self.MAX_MEMORY_MB}MB")
        
        if reasons:
            return False, "; ".join(reasons)
        
        return True, ""
    
    def save_results(self, suite_result: BenchmarkSuiteResult, output_dir: Path):
        """Save benchmark results to files."""
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Save JSON
        json_file = output_dir / f"benchmark_{suite_result.suite_name}_{timestamp}.json"
        with open(json_file, 'w') as f:
            json.dump(suite_result.to_dict(), f, indent=2)
        
        # Save summary
        summary_file = output_dir / f"benchmark_{suite_result.suite_name}_{timestamp}.txt"
        with open(summary_file, 'w') as f:
            f.write(suite_result.get_summary())
        
        print(f"Results saved to {output_dir}")
    
    def save_as_baseline(self, result: BenchmarkResult, name: Optional[str] = None):
        """Save a benchmark result as a baseline for regression detection."""
        baseline_name = name or result.config.name
        self.baseline_manager.save_baseline(baseline_name, result.profiling_result)
        print(f"Saved baseline: {baseline_name}")
    
    def compare_to_baseline(self, result: BenchmarkResult, baseline_name: Optional[str] = None) -> Dict[str, Any]:
        """Compare a benchmark result to a saved baseline."""
        name = baseline_name or result.config.name
        return self.baseline_manager.compare_to_baseline(name, result.profiling_result)


def run_benchmarks_cli():
    """
    CLI entry point for running benchmarks.
    
    Usage:
        python -m tests.e2e.benchmarks
    """
    import argparse
    
    parser = argparse.ArgumentParser(description="Run Oscil performance benchmarks")
    parser.add_argument('--suite', choices=['quick', 'standard', 'full'], default='quick',
                       help='Benchmark suite to run')
    parser.add_argument('--output', type=str, default='./benchmark_results',
                       help='Output directory for results')
    parser.add_argument('--save-baseline', action='store_true',
                       help='Save results as baselines')
    
    args = parser.parse_args()
    
    benchmark = PerformanceBenchmark()
    
    print(f"Running {args.suite} benchmark suite...")
    
    if args.suite == 'quick':
        result = benchmark.run_quick_suite()
    elif args.suite == 'standard':
        result = benchmark.run_standard_suite()
    else:
        result = benchmark.run_full_suite()
    
    print("\n" + result.get_summary())
    
    # Save results
    benchmark.save_results(result, Path(args.output))
    
    # Save baselines if requested
    if args.save_baseline:
        for r in result.results:
            benchmark.save_as_baseline(r)
    
    return 0 if result.all_passed else 1


if __name__ == "__main__":
    exit(run_benchmarks_cli())

