"""
Performance Analyzer for Oscil E2E Testing

Provides post-hoc analysis of profiling data:
- Trend detection and analysis
- Hotspot categorization and prioritization
- Report generation (JSON, Markdown, HTML)
- Regression detection against baselines
"""

import json
import statistics
from dataclasses import dataclass, field
from typing import Optional, Dict, Any, List, Tuple
from pathlib import Path
from datetime import datetime
import html

from .performance_profiler import (
    ProfilingResult, 
    GpuMetrics, 
    ThreadMetrics, 
    ComponentStats,
    PerformanceHotspot,
    FrameData
)


@dataclass
class PerformanceTrend:
    """Represents a trend in performance metrics over time."""
    metric_name: str
    values: List[float] = field(default_factory=list)
    timestamps: List[int] = field(default_factory=list)
    trend_direction: str = "stable"  # improving, degrading, stable
    change_rate: float = 0.0  # per second
    volatility: float = 0.0  # standard deviation
    
    def analyze(self):
        """Analyze the trend from the collected values."""
        if len(self.values) < 2:
            return
        
        # Calculate volatility
        self.volatility = statistics.stdev(self.values) if len(self.values) > 1 else 0.0
        
        # Calculate change rate using linear regression approximation
        if len(self.timestamps) >= 2:
            time_range_sec = (self.timestamps[-1] - self.timestamps[0]) / 1000.0
            if time_range_sec > 0:
                value_change = self.values[-1] - self.values[0]
                self.change_rate = value_change / time_range_sec
        
        # Determine trend direction
        if abs(self.change_rate) < 0.01:
            self.trend_direction = "stable"
        elif self.change_rate > 0:
            self.trend_direction = "increasing"
        else:
            self.trend_direction = "decreasing"


@dataclass
class AnalysisReport:
    """Complete performance analysis report."""
    timestamp: int = 0
    duration_ms: int = 0
    
    # Summary
    overall_health: str = "healthy"  # healthy, warning, critical
    health_score: float = 100.0  # 0-100
    
    # Key metrics
    avg_fps: float = 0.0
    p95_frame_time_ms: float = 0.0
    peak_memory_mb: float = 0.0
    
    # Categorized hotspots
    gpu_hotspots: List[PerformanceHotspot] = field(default_factory=list)
    thread_hotspots: List[PerformanceHotspot] = field(default_factory=list)
    ui_hotspots: List[PerformanceHotspot] = field(default_factory=list)
    memory_hotspots: List[PerformanceHotspot] = field(default_factory=list)
    
    # Top offenders
    top_components: List[ComponentStats] = field(default_factory=list)
    
    # Trends
    fps_trend: Optional[PerformanceTrend] = None
    memory_trend: Optional[PerformanceTrend] = None
    frame_time_trend: Optional[PerformanceTrend] = None
    
    # Recommendations
    recommendations: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'timestamp': self.timestamp,
            'durationMs': self.duration_ms,
            'overallHealth': self.overall_health,
            'healthScore': self.health_score,
            'avgFps': self.avg_fps,
            'p95FrameTimeMs': self.p95_frame_time_ms,
            'peakMemoryMB': self.peak_memory_mb,
            'hotspotCounts': {
                'gpu': len(self.gpu_hotspots),
                'thread': len(self.thread_hotspots),
                'ui': len(self.ui_hotspots),
                'memory': len(self.memory_hotspots),
                'total': len(self.gpu_hotspots) + len(self.thread_hotspots) + 
                        len(self.ui_hotspots) + len(self.memory_hotspots)
            },
            'topComponentsCount': len(self.top_components),
            'recommendationCount': len(self.recommendations)
        }


class PerformanceAnalyzer:
    """
    Analyzes profiling results and generates reports.
    
    Usage:
        analyzer = PerformanceAnalyzer()
        report = analyzer.analyze(profiling_result)
        print(analyzer.to_markdown(report))
    """
    
    # Thresholds for health assessment
    FPS_HEALTHY = 55.0
    FPS_WARNING = 45.0
    FPS_CRITICAL = 30.0
    
    P95_HEALTHY = 20.0  # ms
    P95_WARNING = 33.33  # ms
    P95_CRITICAL = 50.0  # ms
    
    MEMORY_GROWTH_WARNING = 0.5  # MB/sec
    MEMORY_GROWTH_CRITICAL = 2.0  # MB/sec
    
    def __init__(self):
        pass
    
    def analyze(self, result: ProfilingResult) -> AnalysisReport:
        """
        Perform comprehensive analysis of profiling results.
        
        Args:
            result: ProfilingResult from a profiling session
            
        Returns:
            AnalysisReport with categorized findings
        """
        report = AnalysisReport(
            timestamp=result.end_timestamp,
            duration_ms=result.duration_ms,
            avg_fps=result.avg_fps,
            p95_frame_time_ms=result.p95_frame_time_ms,
            peak_memory_mb=result.peak_memory_mb
        )
        
        # Categorize hotspots
        for hs in result.hotspots:
            if hs.category == "GPU":
                report.gpu_hotspots.append(hs)
            elif hs.category == "Thread" or hs.category == "Lock":
                report.thread_hotspots.append(hs)
            elif hs.category == "UI":
                report.ui_hotspots.append(hs)
            elif hs.category == "Memory":
                report.memory_hotspots.append(hs)
        
        # Get top components by repaint time
        if result.component_stats:
            sorted_components = sorted(
                result.component_stats, 
                key=lambda c: c.total_repaint_time_ms,
                reverse=True
            )
            report.top_components = sorted_components[:10]
        
        # Analyze trends from frame timeline
        if result.frame_timeline:
            report.fps_trend = self._analyze_fps_trend(result.frame_timeline)
            report.frame_time_trend = self._analyze_frame_time_trend(result.frame_timeline)
        
        # Analyze memory trend
        if result.memory_growth_mb_per_sec != 0:
            trend = PerformanceTrend(metric_name="memory")
            trend.change_rate = result.memory_growth_mb_per_sec
            trend.trend_direction = "increasing" if result.memory_growth_mb_per_sec > 0.01 else "stable"
            report.memory_trend = trend
        
        # Calculate health score and status
        report.health_score, report.overall_health = self._calculate_health(result, report)
        
        # Generate recommendations
        report.recommendations = self._generate_recommendations(result, report)
        
        return report
    
    def _analyze_fps_trend(self, frames: List[FrameData]) -> PerformanceTrend:
        """Analyze FPS trend from frame data."""
        trend = PerformanceTrend(metric_name="fps")
        
        for frame in frames:
            if frame.total_frame_ms > 0:
                fps = 1000.0 / frame.total_frame_ms
                trend.values.append(fps)
                trend.timestamps.append(frame.timestamp)
        
        trend.analyze()
        return trend
    
    def _analyze_frame_time_trend(self, frames: List[FrameData]) -> PerformanceTrend:
        """Analyze frame time trend from frame data."""
        trend = PerformanceTrend(metric_name="frameTime")
        
        for frame in frames:
            trend.values.append(frame.total_frame_ms)
            trend.timestamps.append(frame.timestamp)
        
        trend.analyze()
        return trend
    
    def _calculate_health(self, result: ProfilingResult, report: AnalysisReport) -> Tuple[float, str]:
        """Calculate overall health score and status."""
        score = 100.0
        
        # FPS scoring (40% weight)
        if result.avg_fps >= self.FPS_HEALTHY:
            fps_score = 40.0
        elif result.avg_fps >= self.FPS_WARNING:
            fps_score = 40.0 * (result.avg_fps - self.FPS_WARNING) / (self.FPS_HEALTHY - self.FPS_WARNING)
        elif result.avg_fps >= self.FPS_CRITICAL:
            fps_score = 20.0 * (result.avg_fps - self.FPS_CRITICAL) / (self.FPS_WARNING - self.FPS_CRITICAL)
        else:
            fps_score = 0.0
        score = fps_score
        
        # P95 frame time scoring (30% weight)
        if result.p95_frame_time_ms <= self.P95_HEALTHY:
            p95_score = 30.0
        elif result.p95_frame_time_ms <= self.P95_WARNING:
            p95_score = 30.0 * (self.P95_WARNING - result.p95_frame_time_ms) / (self.P95_WARNING - self.P95_HEALTHY)
        elif result.p95_frame_time_ms <= self.P95_CRITICAL:
            p95_score = 15.0 * (self.P95_CRITICAL - result.p95_frame_time_ms) / (self.P95_CRITICAL - self.P95_WARNING)
        else:
            p95_score = 0.0
        score += p95_score
        
        # Hotspot scoring (20% weight)
        critical_hotspots = sum(1 for hs in result.hotspots if hs.severity > 0.7)
        warning_hotspots = sum(1 for hs in result.hotspots if 0.4 < hs.severity <= 0.7)
        hotspot_penalty = critical_hotspots * 5.0 + warning_hotspots * 2.0
        hotspot_score = max(0.0, 20.0 - hotspot_penalty)
        score += hotspot_score
        
        # Memory stability scoring (10% weight)
        if result.memory_growth_mb_per_sec <= 0.1:
            memory_score = 10.0
        elif result.memory_growth_mb_per_sec <= self.MEMORY_GROWTH_WARNING:
            memory_score = 5.0
        else:
            memory_score = 0.0
        score += memory_score
        
        # Determine status
        if score >= 80:
            status = "healthy"
        elif score >= 50:
            status = "warning"
        else:
            status = "critical"
        
        return score, status
    
    def _generate_recommendations(self, result: ProfilingResult, report: AnalysisReport) -> List[str]:
        """Generate actionable recommendations based on analysis."""
        recommendations = []
        
        # FPS recommendations
        if result.avg_fps < self.FPS_HEALTHY:
            if result.gpu_metrics and result.gpu_metrics.avg_effect_pipeline_ms > 5.0:
                recommendations.append(
                    "Effect pipeline is consuming significant frame time. "
                    "Consider reducing effect complexity or disabling non-essential effects."
                )
            
            if result.gpu_metrics and result.gpu_metrics.avg_waveform_render_ms > 8.0:
                recommendations.append(
                    "Waveform rendering is slow. Consider reducing waveform count "
                    "or using simpler shaders."
                )
        
        # Frame stuttering recommendations
        if result.p95_frame_time_ms > self.P95_WARNING:
            recommendations.append(
                "Frame stuttering detected (high P95 frame time). "
                "Check for GC pressure, UI thread blocking, or shader compilation hitches."
            )
        
        # Memory recommendations
        if result.memory_growth_mb_per_sec > self.MEMORY_GROWTH_WARNING:
            recommendations.append(
                f"Memory is growing at {result.memory_growth_mb_per_sec:.2f} MB/sec. "
                "This may indicate a memory leak. Check for accumulating buffers or retained objects."
            )
        
        # Thread recommendations
        if report.thread_hotspots:
            for hs in report.thread_hotspots[:2]:
                recommendations.append(hs.recommendation)
        
        # UI recommendations
        if report.top_components:
            top = report.top_components[0]
            if top.repaints_per_second > 120:
                recommendations.append(
                    f"Component '{top.name}' is repainting {top.repaints_per_second:.0f}/sec. "
                    "Consider coalescing updates or using shouldComponentUpdate logic."
                )
        
        return recommendations
    
    def to_json(self, report: AnalysisReport) -> str:
        """Convert report to JSON string."""
        data = report.to_dict()
        
        # Add detailed hotspots
        data['hotspots'] = {
            'gpu': [{'location': h.location, 'severity': h.severity, 'recommendation': h.recommendation} 
                   for h in report.gpu_hotspots],
            'thread': [{'location': h.location, 'severity': h.severity, 'recommendation': h.recommendation}
                      for h in report.thread_hotspots],
            'ui': [{'location': h.location, 'severity': h.severity, 'recommendation': h.recommendation}
                  for h in report.ui_hotspots],
            'memory': [{'location': h.location, 'severity': h.severity, 'recommendation': h.recommendation}
                      for h in report.memory_hotspots]
        }
        
        # Add recommendations
        data['recommendations'] = report.recommendations
        
        return json.dumps(data, indent=2)
    
    def to_markdown(self, report: AnalysisReport) -> str:
        """Convert report to Markdown format."""
        lines = [
            "# Performance Analysis Report",
            "",
            f"**Generated:** {datetime.fromtimestamp(report.timestamp / 1000).isoformat()}",
            f"**Duration:** {report.duration_ms / 1000:.1f} seconds",
            f"**Health Status:** {report.overall_health.upper()} (Score: {report.health_score:.0f}/100)",
            "",
            "## Summary",
            "",
            f"| Metric | Value |",
            f"|--------|-------|",
            f"| Average FPS | {report.avg_fps:.1f} |",
            f"| P95 Frame Time | {report.p95_frame_time_ms:.2f} ms |",
            f"| Peak Memory | {report.peak_memory_mb:.1f} MB |",
            ""
        ]
        
        # Hotspots section
        all_hotspots = (report.gpu_hotspots + report.thread_hotspots + 
                       report.ui_hotspots + report.memory_hotspots)
        if all_hotspots:
            lines.extend([
                "## Performance Hotspots",
                "",
                f"Found {len(all_hotspots)} hotspot(s):",
                ""
            ])
            
            # Sort by severity
            sorted_hotspots = sorted(all_hotspots, key=lambda h: h.severity, reverse=True)
            
            for hs in sorted_hotspots[:10]:
                severity_indicator = "🔴" if hs.severity > 0.7 else ("🟡" if hs.severity > 0.4 else "🟢")
                lines.append(f"### {severity_indicator} [{hs.category}] {hs.location}")
                lines.append("")
                lines.append(f"**Severity:** {hs.severity:.2f}")
                lines.append(f"**Description:** {hs.description}")
                if hs.impact_ms > 0:
                    lines.append(f"**Impact:** {hs.impact_ms:.2f} ms")
                lines.append(f"**Recommendation:** {hs.recommendation}")
                lines.append("")
        
        # Top components
        if report.top_components:
            lines.extend([
                "## Top Components by Repaint Time",
                "",
                "| Component | Repaints | Total Time | Avg Time | Repaints/sec |",
                "|-----------|----------|------------|----------|--------------|"
            ])
            
            for comp in report.top_components[:5]:
                lines.append(
                    f"| {comp.name} | {comp.repaint_count} | "
                    f"{comp.total_repaint_time_ms:.1f} ms | {comp.avg_repaint_time_ms:.2f} ms | "
                    f"{comp.repaints_per_second:.1f} |"
                )
            lines.append("")
        
        # Recommendations
        if report.recommendations:
            lines.extend([
                "## Recommendations",
                ""
            ])
            for i, rec in enumerate(report.recommendations, 1):
                lines.append(f"{i}. {rec}")
            lines.append("")
        
        return "\n".join(lines)
    
    def to_html(self, report: AnalysisReport) -> str:
        """Convert report to HTML format."""
        health_color = {
            'healthy': '#22c55e',
            'warning': '#f59e0b', 
            'critical': '#ef4444'
        }.get(report.overall_health, '#6b7280')
        
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>Oscil Performance Report</title>
    <style>
        body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 40px; background: #f8fafc; }}
        .container {{ max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        h1 {{ color: #1e293b; border-bottom: 2px solid #e2e8f0; padding-bottom: 10px; }}
        h2 {{ color: #334155; margin-top: 30px; }}
        .health-badge {{ display: inline-block; padding: 5px 15px; border-radius: 20px; color: white; font-weight: bold; background: {health_color}; }}
        .metric-grid {{ display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; margin: 20px 0; }}
        .metric-card {{ background: #f1f5f9; padding: 20px; border-radius: 8px; text-align: center; }}
        .metric-value {{ font-size: 24px; font-weight: bold; color: #0f172a; }}
        .metric-label {{ font-size: 12px; color: #64748b; text-transform: uppercase; }}
        table {{ width: 100%; border-collapse: collapse; margin: 20px 0; }}
        th, td {{ padding: 12px; text-align: left; border-bottom: 1px solid #e2e8f0; }}
        th {{ background: #f8fafc; font-weight: 600; }}
        .hotspot {{ background: #fef2f2; padding: 15px; border-radius: 8px; margin: 10px 0; border-left: 4px solid #ef4444; }}
        .hotspot.warning {{ background: #fffbeb; border-left-color: #f59e0b; }}
        .hotspot.info {{ background: #f0fdf4; border-left-color: #22c55e; }}
        .recommendation {{ background: #eff6ff; padding: 10px 15px; border-radius: 6px; margin: 8px 0; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Performance Analysis Report</h1>
        <p>
            <span class="health-badge">{report.overall_health.upper()}</span>
            Score: {report.health_score:.0f}/100
        </p>
        
        <div class="metric-grid">
            <div class="metric-card">
                <div class="metric-value">{report.avg_fps:.1f}</div>
                <div class="metric-label">Average FPS</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">{report.p95_frame_time_ms:.1f}ms</div>
                <div class="metric-label">P95 Frame Time</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">{report.peak_memory_mb:.0f}MB</div>
                <div class="metric-label">Peak Memory</div>
            </div>
        </div>
"""
        
        # Hotspots
        all_hotspots = (report.gpu_hotspots + report.thread_hotspots + 
                       report.ui_hotspots + report.memory_hotspots)
        if all_hotspots:
            html_content += "<h2>Performance Hotspots</h2>\n"
            sorted_hotspots = sorted(all_hotspots, key=lambda h: h.severity, reverse=True)
            
            for hs in sorted_hotspots[:10]:
                css_class = "hotspot"
                if hs.severity <= 0.4:
                    css_class += " info"
                elif hs.severity <= 0.7:
                    css_class += " warning"
                
                html_content += f"""
        <div class="{css_class}">
            <strong>[{html.escape(hs.category)}] {html.escape(hs.location)}</strong><br>
            <small>Severity: {hs.severity:.2f}</small><br>
            {html.escape(hs.description)}<br>
            <em>{html.escape(hs.recommendation)}</em>
        </div>
"""
        
        # Recommendations
        if report.recommendations:
            html_content += "<h2>Recommendations</h2>\n"
            for rec in report.recommendations:
                html_content += f'<div class="recommendation">{html.escape(rec)}</div>\n'
        
        html_content += """
    </div>
</body>
</html>
"""
        return html_content
    
    def save_report(self, report: AnalysisReport, output_dir: Path, name: str = "performance_report"):
        """
        Save report in multiple formats.
        
        Args:
            report: Analysis report
            output_dir: Directory to save reports
            name: Base name for report files
        """
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Save JSON
        with open(output_dir / f"{name}.json", 'w') as f:
            f.write(self.to_json(report))
        
        # Save Markdown
        with open(output_dir / f"{name}.md", 'w') as f:
            f.write(self.to_markdown(report))
        
        # Save HTML
        with open(output_dir / f"{name}.html", 'w') as f:
            f.write(self.to_html(report))

