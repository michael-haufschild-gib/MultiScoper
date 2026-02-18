"""
Performance test for dialog animation frame rate impact.

This test profiles the frame rate when opening dialogs to identify
performance bottlenecks in the modal animation system.

Issue: Frame rate drops significantly when opening dialogs.
Hypothesis: The modal's timerCallback() calls resized() every frame,
causing expensive layout recalculations of all child components.
"""

import time
import pytest
from typing import Dict, Any, List

from .oscil_client import OscilClient, OscilClientError
from .performance_profiler import PerformanceProfiler, ProfilingResult


class DialogAnimationProfiler:
    """
    Profiles dialog animation performance to identify bottlenecks.
    """
    
    def __init__(self, client: OscilClient):
        self.client = client
        self.profiler = PerformanceProfiler(client)
    
    def measure_baseline_fps(self, duration: float = 2.0) -> float:
        """Measure FPS before opening dialog."""
        self.profiler.start()
        time.sleep(duration)
        result = self.profiler.stop()
        return result.avg_fps
    
    def measure_dialog_open_impact(self, 
                                    dialog_button_id: str,
                                    dialog_id: str,
                                    close_button_id: str) -> Dict[str, Any]:
        """
        Measure performance impact of opening and closing a dialog.
        
        Returns:
            Dict containing baseline FPS, dialog-open FPS, and analysis
        """
        results = {
            'baseline_fps': 0.0,
            'dialog_open_fps': 0.0,
            'dialog_closing_fps': 0.0,
            'fps_drop_percent': 0.0,
            'animation_duration_ms': 0,
            'component_stats': [],
            'hotspots': [],
            'analysis': {}
        }
        
        # 1. Measure baseline FPS (no dialog)
        print("Measuring baseline FPS...")
        self.profiler.start()
        time.sleep(2.0)
        baseline_result = self.profiler.stop()
        results['baseline_fps'] = baseline_result.avg_fps
        print(f"  Baseline FPS: {baseline_result.avg_fps:.1f}")
        
        # 2. Start profiling before opening dialog
        print("Starting profiling for dialog open...")
        self.profiler.start()
        time.sleep(0.1)  # Small delay to ensure profiling is active
        
        # 3. Open the dialog
        open_time = time.time()
        try:
            self.client.click(dialog_button_id)
        except OscilClientError as e:
            print(f"  Failed to click dialog button: {e}")
            self.profiler.stop()
            return results
        
        # 4. Wait for animation to complete (modal animation is ~500ms typically)
        time.sleep(0.8)
        
        # 5. Keep dialog open and measure FPS for a bit
        time.sleep(1.0)
        
        # 6. Stop profiling and get results
        dialog_result = self.profiler.stop()
        results['dialog_open_fps'] = dialog_result.avg_fps
        results['component_stats'] = [
            {'name': c.name, 'repaint_count': c.repaint_count, 
             'avg_repaint_time_ms': c.avg_repaint_time_ms,
             'repaints_per_second': c.repaints_per_second}
            for c in dialog_result.component_stats
        ]
        results['hotspots'] = [
            {'category': h.category, 'location': h.location, 
             'description': h.description, 'severity': h.severity,
             'impact_ms': h.impact_ms, 'recommendation': h.recommendation}
            for h in dialog_result.hotspots
        ]
        
        # Calculate FPS drop
        if results['baseline_fps'] > 0:
            fps_drop = results['baseline_fps'] - results['dialog_open_fps']
            results['fps_drop_percent'] = (fps_drop / results['baseline_fps']) * 100
        
        print(f"  Dialog open FPS: {results['dialog_open_fps']:.1f}")
        print(f"  FPS drop: {results['fps_drop_percent']:.1f}%")
        
        # 7. Now close the dialog and measure closing animation
        print("Closing dialog...")
        self.profiler.start()
        try:
            self.client.click(close_button_id)
        except OscilClientError as e:
            print(f"  Failed to click close button: {e}")
            # Try pressing Escape as fallback
            try:
                self.client._post("/ui/key", {"key": "escape"})
            except:
                pass
        
        time.sleep(1.0)
        closing_result = self.profiler.stop()
        results['dialog_closing_fps'] = closing_result.avg_fps
        
        # 8. Analysis
        results['analysis'] = self._analyze_results(results, dialog_result)
        
        return results
    
    def _analyze_results(self, results: Dict[str, Any], 
                         profile_result: ProfilingResult) -> Dict[str, Any]:
        """Analyze profiling results and identify issues."""
        analysis = {
            'severity': 'unknown',
            'issues': [],
            'recommendations': []
        }
        
        fps_drop = results['fps_drop_percent']
        
        # Classify severity
        if fps_drop < 10:
            analysis['severity'] = 'minor'
        elif fps_drop < 25:
            analysis['severity'] = 'moderate'
        elif fps_drop < 50:
            analysis['severity'] = 'significant'
        else:
            analysis['severity'] = 'severe'
        
        # Identify specific issues
        if fps_drop > 15:
            analysis['issues'].append(
                f"FPS dropped {fps_drop:.1f}% during dialog animation"
            )
        
        # Check for excessive component repaints
        high_repaint_components = [
            c for c in results['component_stats']
            if c.get('repaints_per_second', 0) > 60
        ]
        if high_repaint_components:
            for c in high_repaint_components:
                analysis['issues'].append(
                    f"Component '{c['name']}' repainting at {c['repaints_per_second']:.0f}/sec"
                )
        
        # Check GPU metrics if available
        if profile_result.gpu_metrics:
            if profile_result.gpu_metrics.p95_frame_time_ms > 20:
                analysis['issues'].append(
                    f"P95 frame time is {profile_result.gpu_metrics.p95_frame_time_ms:.1f}ms (should be <16.67ms)"
                )
        
        # Add recommendations based on findings
        if fps_drop > 15:
            analysis['recommendations'].append(
                "Remove resized() call from OscilModal::timerCallback() - "
                "transform-based scaling doesn't require layout updates"
            )
            analysis['recommendations'].append(
                "Consider using setBufferedToImage(true) for modal backdrop"
            )
        
        return analysis
    
    def get_frame_timeline_during_dialog(self, 
                                          dialog_button_id: str,
                                          frame_count: int = 120) -> List[Dict[str, Any]]:
        """
        Capture frame-by-frame timing during dialog animation.
        
        Returns list of frame timing data.
        """
        self.profiler.start()
        time.sleep(0.1)
        
        self.client.click(dialog_button_id)
        time.sleep(1.5)  # Capture full animation
        
        # Get timeline
        frames = self.profiler.get_frame_timeline(frame_count)
        self.profiler.stop()
        
        return [
            {
                'frame_number': f.frame_number,
                'total_frame_ms': f.total_frame_ms,
                'waveform_render_ms': f.waveform_render_ms,
                'effect_pipeline_ms': f.effect_pipeline_ms
            }
            for f in frames
        ]


@pytest.fixture
def client():
    """Create OscilClient fixture."""
    c = OscilClient()
    if not c.ping():
        pytest.skip("Test harness not running")
    return c


@pytest.fixture
def ensure_editor_open(client):
    """Ensure editor is open before tests."""
    try:
        client._post("/track/0/showEditor")
        time.sleep(1.0)
    except:
        pass
    yield
    

class TestDialogAnimationPerformance:
    """Tests for dialog animation performance."""
    
    def test_add_oscillator_dialog_fps_impact(self, client, ensure_editor_open):
        """
        Test that opening the add oscillator dialog doesn't cause significant FPS drop.
        
        Expected: FPS should not drop more than 25% during dialog animation.
        """
        profiler = DialogAnimationProfiler(client)
        
        results = profiler.measure_dialog_open_impact(
            dialog_button_id="sidebar_addOscillator",
            dialog_id="addOscillatorDialog",
            close_button_id="addOscillatorDialog_cancelBtn"
        )
        
        # Print detailed results
        print("\n" + "="*60)
        print("DIALOG ANIMATION PERFORMANCE RESULTS")
        print("="*60)
        print(f"Baseline FPS: {results['baseline_fps']:.1f}")
        print(f"Dialog Open FPS: {results['dialog_open_fps']:.1f}")
        print(f"FPS Drop: {results['fps_drop_percent']:.1f}%")
        print(f"Severity: {results['analysis']['severity']}")
        
        if results['analysis']['issues']:
            print("\nIssues Identified:")
            for issue in results['analysis']['issues']:
                print(f"  - {issue}")
        
        if results['analysis']['recommendations']:
            print("\nRecommendations:")
            for rec in results['analysis']['recommendations']:
                print(f"  - {rec}")
        
        if results['hotspots']:
            print("\nPerformance Hotspots:")
            for hs in results['hotspots'][:5]:
                print(f"  [{hs['category']}] {hs['location']}: {hs['description']}")
        
        print("="*60)
        
        # Assert performance is acceptable
        # Allow up to 25% FPS drop during animation
        assert results['fps_drop_percent'] < 25, (
            f"FPS dropped too much ({results['fps_drop_percent']:.1f}%) during dialog animation. "
            f"Baseline: {results['baseline_fps']:.1f} FPS, Dialog: {results['dialog_open_fps']:.1f} FPS"
        )
    
    def test_multiple_dialog_opens(self, client, ensure_editor_open):
        """
        Test that repeated dialog opens don't cause progressive performance degradation.
        """
        profiler = PerformanceProfiler(client)
        fps_readings = []
        
        for i in range(3):
            print(f"\nIteration {i+1}/3:")
            
            # Measure FPS during dialog open
            profiler.start()
            try:
                client.click("sidebar_addOscillator")
                time.sleep(0.5)  # Animation time
            except OscilClientError:
                continue
            
            time.sleep(0.5)  # Measure with dialog open
            result = profiler.stop()
            fps_readings.append(result.avg_fps)
            print(f"  FPS: {result.avg_fps:.1f}")
            
            # Close dialog
            try:
                client.click("addOscillatorDialog_cancelBtn")
            except:
                try:
                    client._post("/ui/key", {"key": "escape"})
                except:
                    pass
            time.sleep(0.5)
        
        if len(fps_readings) >= 2:
            fps_degradation = fps_readings[0] - fps_readings[-1]
            print(f"\nFPS degradation over iterations: {fps_degradation:.1f}")
            
            # FPS shouldn't degrade by more than 10% across iterations
            if fps_readings[0] > 0:
                degradation_percent = (fps_degradation / fps_readings[0]) * 100
                assert degradation_percent < 10, (
                    f"Performance degraded {degradation_percent:.1f}% across dialog open/close cycles"
                )


def run_standalone_profile():
    """
    Run profiling outside of pytest for manual investigation.
    
    Usage: python -m tests.e2e.test_dialog_animation_performance
    """
    print("Dialog Animation Performance Profiler")
    print("=" * 50)
    
    client = OscilClient()
    
    # Check if harness is running
    if not client.ping():
        print("ERROR: Test harness is not running!")
        print("Start it with: ./build/dev/test_harness/...")
        return
    
    # Ensure editor is open
    print("Opening editor...")
    try:
        client._post("/track/0/showEditor")
        time.sleep(1.5)
    except Exception as e:
        print(f"Note: {e}")
    
    # Run profiling
    profiler = DialogAnimationProfiler(client)
    
    print("\n" + "-"*50)
    print("Profiling add oscillator dialog...")
    print("-"*50)
    
    results = profiler.measure_dialog_open_impact(
        dialog_button_id="sidebar_addOscillator",
        dialog_id="addOscillatorDialog",
        close_button_id="addOscillatorDialog_cancelBtn"
    )
    
    # Output detailed results
    import json
    print("\n\nFull Results JSON:")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    run_standalone_profile()

