#!/usr/bin/env python3
"""
Oscil Performance Profiling Script

Automated script for running performance profiling and benchmarks.

Usage:
    ./scripts/profile_performance.py                    # Quick profile
    ./scripts/profile_performance.py --benchmark quick  # Run quick benchmarks
    ./scripts/profile_performance.py --benchmark full   # Run full benchmarks
    ./scripts/profile_performance.py --duration 30      # 30 second profile
    ./scripts/profile_performance.py --report           # Generate detailed report
"""

import argparse
import subprocess
import sys
import time
import os
import signal
from pathlib import Path
from datetime import datetime

# Add tests/e2e to path for imports
SCRIPT_DIR = Path(__file__).parent.absolute()
PROJECT_ROOT = SCRIPT_DIR.parent
E2E_DIR = PROJECT_ROOT / "tests" / "e2e"
sys.path.insert(0, str(PROJECT_ROOT / "tests"))

from e2e.oscil_client import OscilClient, OscilClientError
from e2e.performance_profiler import PerformanceProfiler, ProfilingResult
from e2e.performance_analyzer import PerformanceAnalyzer
from e2e.benchmarks import (
    PerformanceBenchmark, 
    QUICK_SUITE, 
    STANDARD_SUITE, 
    FULL_SUITE
)


# Test harness paths
HARNESS_PATHS = [
    PROJECT_ROOT / "build" / "dev" / "test_harness" / "OscilTestHarness_artefacts" / "Debug" / "Oscil Test Harness.app" / "Contents" / "MacOS" / "Oscil Test Harness",
    PROJECT_ROOT / "build" / "Test" / "OscilTestHarness_artefacts" / "Debug" / "OscilTestHarness.app" / "Contents" / "MacOS" / "OscilTestHarness",
    PROJECT_ROOT / "cmake-build-test" / "test_harness" / "OscilTestHarness_artefacts" / "Debug" / "OscilTestHarness.app" / "Contents" / "MacOS" / "OscilTestHarness",
]

OUTPUT_DIR = PROJECT_ROOT / "build" / "performance_reports"


def find_harness() -> Path:
    """Find the test harness binary."""
    for path in HARNESS_PATHS:
        if path.exists():
            return path
    return None


def is_harness_running() -> bool:
    """Check if the test harness is already running."""
    try:
        client = OscilClient(timeout=2.0)
        return client.ping()
    except:
        return False


def start_harness(harness_path: Path) -> subprocess.Popen:
    """Start the test harness process."""
    print(f"Starting test harness: {harness_path}")
    
    env = os.environ.copy()
    env["OSCIL_TEST_MODE"] = "1"
    
    process = subprocess.Popen(
        [str(harness_path)],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # Wait for harness to be ready
    client = OscilClient(timeout=30.0)
    if not client.wait_for_ready(timeout=30.0):
        process.terminate()
        raise RuntimeError("Test harness failed to start")
    
    print("Test harness is ready")
    time.sleep(1.0)  # Extra time for UI initialization
    
    return process


def stop_harness(process: subprocess.Popen):
    """Stop the test harness process."""
    if process and process.poll() is None:
        print("Stopping test harness...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def run_quick_profile(duration: float = 5.0) -> ProfilingResult:
    """Run a quick profiling session."""
    print(f"\n=== Running Quick Profile ({duration}s) ===\n")
    
    client = OscilClient()
    profiler = PerformanceProfiler(client)
    
    # Setup: reset state, open editor, add an oscillator
    print("Setting up test environment...")
    client.reset()
    time.sleep(0.5)
    client.show_editor()
    time.sleep(0.5)
    client.add_oscillator("Profile Test", "input_1_2", "pane_0")
    time.sleep(0.5)
    
    # Enable audio
    try:
        client._post("/stress/audio", {
            "durationMs": int((duration + 5) * 1000),
            "waveform": "sine",
            "frequency": 440.0,
            "amplitude": 0.5
        })
    except:
        print("Warning: Could not start audio generator")
    
    # Warmup
    print("Warming up...")
    time.sleep(2.0)
    
    # Profile
    print(f"Profiling for {duration} seconds...")
    profiler.start()
    time.sleep(duration)
    result = profiler.stop()
    
    return result


def print_result_summary(result: ProfilingResult):
    """Print a summary of profiling results."""
    print("\n" + "=" * 60)
    print("PROFILING RESULTS")
    print("=" * 60)
    print(f"\nDuration: {result.duration_ms / 1000:.1f} seconds")
    print(f"\n--- Frame Rate ---")
    print(f"  Average FPS: {result.avg_fps:.1f}")
    print(f"  Min FPS: {result.min_fps:.1f}")
    print(f"  Max FPS: {result.max_fps:.1f}")
    print(f"\n--- Frame Time ---")
    print(f"  P50: {result.p50_frame_time_ms:.2f} ms")
    print(f"  P95: {result.p95_frame_time_ms:.2f} ms")
    print(f"  P99: {result.p99_frame_time_ms:.2f} ms")
    print(f"\n--- Memory ---")
    print(f"  Peak: {result.peak_memory_mb:.1f} MB")
    print(f"  Growth: {result.memory_growth_mb_per_sec:.3f} MB/sec")
    print(f"\n--- CPU ---")
    print(f"  Average: {result.avg_cpu:.1f}%")
    
    if result.gpu_metrics:
        print(f"\n--- GPU Timing Breakdown ---")
        print(f"  Begin Frame: {result.gpu_metrics.avg_begin_frame_ms:.2f} ms")
        print(f"  Waveform Render: {result.gpu_metrics.avg_waveform_render_ms:.2f} ms")
        print(f"  Effect Pipeline: {result.gpu_metrics.avg_effect_pipeline_ms:.2f} ms")
        print(f"  Composite: {result.gpu_metrics.avg_composite_ms:.2f} ms")
        print(f"  Blit: {result.gpu_metrics.avg_blit_ms:.2f} ms")
        print(f"  Total Frames: {result.gpu_metrics.total_frames}")
    
    if result.hotspots:
        print(f"\n--- Hotspots ({len(result.hotspots)}) ---")
        for hs in result.hotspots[:5]:
            severity = "🔴" if hs.severity > 0.7 else ("🟡" if hs.severity > 0.4 else "🟢")
            print(f"  {severity} [{hs.category}] {hs.location}")
            print(f"      {hs.description}")
            print(f"      Recommendation: {hs.recommendation}")
    
    # Health assessment
    print(f"\n--- Health Assessment ---")
    if result.is_healthy():
        print("  ✓ Performance is HEALTHY")
    else:
        print("  ✗ Performance issues detected")
        if result.avg_fps < 55:
            print(f"    - Low FPS: {result.avg_fps:.1f} (target: 60)")
        if result.p95_frame_time_ms > 20:
            print(f"    - High P95 frame time: {result.p95_frame_time_ms:.1f}ms (target: <16.67ms)")
    
    print("=" * 60)


def run_benchmarks(suite_name: str, output_dir: Path):
    """Run benchmark suite and save results."""
    print(f"\n=== Running {suite_name.upper()} Benchmark Suite ===\n")
    
    benchmark = PerformanceBenchmark()
    
    if suite_name == "quick":
        result = benchmark.run_quick_suite()
    elif suite_name == "standard":
        result = benchmark.run_standard_suite()
    elif suite_name == "full":
        result = benchmark.run_full_suite()
    else:
        print(f"Unknown suite: {suite_name}")
        return
    
    print("\n" + result.get_summary())
    
    # Save results
    benchmark.save_results(result, output_dir)
    
    if not result.all_passed:
        print("\n⚠️  Some benchmarks failed!")
        return 1
    
    print("\n✓ All benchmarks passed!")
    return 0


def generate_report(result: ProfilingResult, output_dir: Path):
    """Generate detailed performance report."""
    analyzer = PerformanceAnalyzer()
    report = analyzer.analyze(result)
    
    # Save reports
    analyzer.save_report(report, output_dir, "performance_report")
    
    print(f"\nReports saved to: {output_dir}")
    print(f"  - performance_report.json")
    print(f"  - performance_report.md")
    print(f"  - performance_report.html")


def main():
    parser = argparse.ArgumentParser(
        description="Oscil Performance Profiling Script",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    ./scripts/profile_performance.py                    # Quick 5s profile
    ./scripts/profile_performance.py --duration 30      # 30 second profile
    ./scripts/profile_performance.py --benchmark quick  # Run quick benchmarks
    ./scripts/profile_performance.py --benchmark full   # Run full benchmarks
    ./scripts/profile_performance.py --report           # Generate detailed report
        """
    )
    
    parser.add_argument('--duration', type=float, default=5.0,
                       help='Profiling duration in seconds (default: 5)')
    parser.add_argument('--benchmark', choices=['quick', 'standard', 'full'],
                       help='Run benchmark suite instead of profiling')
    parser.add_argument('--report', action='store_true',
                       help='Generate detailed HTML/Markdown report')
    parser.add_argument('--output', type=str, default=None,
                       help='Output directory for reports')
    parser.add_argument('--no-start', action='store_true',
                       help='Assume test harness is already running')
    parser.add_argument('--scalability', action='store_true',
                       help='Run scalability test (1-20 oscillators)')
    parser.add_argument('--gpu-comparison', action='store_true',
                       help='Compare GPU vs CPU rendering')
    
    args = parser.parse_args()
    
    # Set up output directory
    output_dir = Path(args.output) if args.output else OUTPUT_DIR
    output_dir.mkdir(parents=True, exist_ok=True)
    
    harness_process = None
    exit_code = 0
    
    try:
        # Check if harness is running
        if not args.no_start and not is_harness_running():
            harness_path = find_harness()
            if not harness_path:
                print("Error: Test harness binary not found. Build with -DOSCIL_BUILD_TEST_HARNESS=ON")
                return 1
            harness_process = start_harness(harness_path)
        elif is_harness_running():
            print("Using existing test harness instance")
        else:
            print("Error: Test harness not running. Start it or remove --no-start")
            return 1
        
        # Run requested operation
        if args.scalability:
            print("\n=== Running Scalability Test ===\n")
            benchmark = PerformanceBenchmark()
            results = benchmark.scalability_test(max_oscillators=20, step=2)
            
            print("\nOscillators | FPS     | P95 Frame Time | Memory")
            print("-" * 55)
            for r in results:
                status = "✓" if r['passed'] else "✗"
                print(f"  {r['oscillatorCount']:2d}        | {r['avgFps']:6.1f} | {r['p95FrameTimeMs']:8.2f} ms    | {r['peakMemoryMB']:.0f} MB {status}")
        
        elif args.gpu_comparison:
            print("\n=== GPU vs CPU Comparison ===\n")
            benchmark = PerformanceBenchmark()
            comparison = benchmark.compare_gpu_vs_cpu()
            
            print(f"GPU Rendering: {comparison['gpu']['avgFps']:.1f} FPS, P95={comparison['gpu']['p95FrameTimeMs']:.1f}ms")
            print(f"CPU Rendering: {comparison['cpu']['avgFps']:.1f} FPS, P95={comparison['cpu']['p95FrameTimeMs']:.1f}ms")
            print(f"\nGPU is {comparison['fpsImprovementPercent']:.1f}% faster")
            print(f"Recommendation: Use {comparison['recommendation']} rendering")
        
        elif args.benchmark:
            exit_code = run_benchmarks(args.benchmark, output_dir)
        
        else:
            # Regular profiling
            result = run_quick_profile(args.duration)
            print_result_summary(result)
            
            if args.report:
                generate_report(result, output_dir)
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        exit_code = 130
    
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        exit_code = 1
    
    finally:
        # Cleanup: stop harness if we started it
        if harness_process:
            stop_harness(harness_process)
    
    return exit_code


if __name__ == "__main__":
    sys.exit(main())

