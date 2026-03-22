#!/usr/bin/env python3
"""
Oscil Scenario-Based E2E Test Runner

Reads YAML scenario definitions and executes them against the test harness.
Produces structured diagnostic reports with full state snapshots on failure.

Usage:
    python scenario_runner.py                    # Run all scenarios
    python scenario_runner.py --scenario core    # Run scenarios/core.yaml only
    python scenario_runner.py --json             # Output JSON report
    python scenario_runner.py --dry-run          # Parse and validate only

Designed for AI coding agent consumption: one command, complete diagnostic output.
"""

import sys
import os
import time
import json as json_module
import argparse
from pathlib import Path
from typing import Any

# Add parent for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from oscil_test_utils import OscilTestClient, HarnessConnectionError

try:
    import yaml
except ImportError:
    yaml = None

SCENARIOS_DIR = Path(__file__).parent / "scenarios"
URL = "http://localhost:8765"


# ── Assertion Evaluators ────────────────────────────────────────────────────

def _resolve_path(snapshot: dict, path: str) -> Any:
    """Resolve a dotted path like 'oscillators[0].captureBuffer.peak' in snapshot."""
    current = snapshot
    for part in path.replace("[", ".[").split("."):
        if not part:
            continue
        if part.startswith("[") and part.endswith("]"):
            idx = int(part[1:-1])
            if isinstance(current, list) and 0 <= idx < len(current):
                current = current[idx]
            else:
                return None
        elif isinstance(current, dict):
            current = current.get(part)
        else:
            return None
        if current is None:
            return None
    return current


def evaluate_assertion(assertion: dict, snapshot: dict) -> dict:
    """Evaluate a single assertion against the diagnostic snapshot.
    Returns {passed, check, expected, actual, detail}."""
    check = assertion.get("check", "")
    result = {"check": check, "passed": False, "expected": "", "actual": "", "detail": ""}

    oscs = snapshot.get("oscillators", [])
    panes = snapshot.get("panes", [])
    timing = snapshot.get("timing", {})
    transport = snapshot.get("transport", {})

    # ── Named assertions ──
    if check == "oscillator_count":
        expected = assertion.get("expected", 0)
        actual = len(oscs)
        result.update(passed=actual == expected, expected=expected, actual=actual)

    elif check == "pane_count":
        expected = assertion.get("expected", 0)
        actual = len(panes)
        result.update(passed=actual == expected, expected=expected, actual=actual)

    elif check == "waveform_has_data":
        idx = assertion.get("oscillator", 0)
        if idx < len(oscs):
            has = oscs[idx].get("captureBuffer", {}).get("hasData", False)
            result.update(passed=has, expected=True, actual=has,
                          detail=f"osc[{idx}] '{oscs[idx].get('name','')}'")
        else:
            result.update(detail=f"oscillator index {idx} out of range (have {len(oscs)})")

    elif check == "peak_above":
        idx = assertion.get("oscillator", 0)
        threshold = assertion.get("threshold", 0.1)
        if idx < len(oscs):
            peak = oscs[idx].get("captureBuffer", {}).get("peak", 0.0)
            result.update(passed=peak > threshold, expected=f"> {threshold}", actual=round(peak, 4),
                          detail=f"osc[{idx}] '{oscs[idx].get('name','')}'")
        else:
            result.update(detail=f"oscillator index {idx} out of range")

    elif check == "peak_below":
        idx = assertion.get("oscillator", 0)
        threshold = assertion.get("threshold", 0.05)
        if idx < len(oscs):
            peak = oscs[idx].get("captureBuffer", {}).get("peak", 0.0)
            result.update(passed=peak < threshold, expected=f"< {threshold}", actual=round(peak, 4),
                          detail=f"osc[{idx}] '{oscs[idx].get('name','')}'")
        else:
            result.update(detail=f"oscillator index {idx} out of range")

    elif check == "display_samples_positive":
        ds = timing.get("displaySamples", 0)
        result.update(passed=ds > 0, expected="> 0", actual=ds)

    elif check == "display_samples_above":
        threshold = assertion.get("threshold", 0)
        ds = timing.get("displaySamples", 0)
        result.update(passed=ds > threshold, expected=f"> {threshold}", actual=ds)

    elif check == "transport_playing":
        playing = transport.get("playing", False)
        result.update(passed=playing, expected=True, actual=playing)

    elif check == "transport_stopped":
        playing = transport.get("playing", False)
        result.update(passed=not playing, expected=False, actual=playing)

    elif check == "timing_mode":
        expected = assertion.get("expected", "TIME")
        actual = timing.get("mode", "")
        result.update(passed=actual == expected, expected=expected, actual=actual)

    elif check == "oscillator_visible":
        idx = assertion.get("oscillator", 0)
        if idx < len(oscs):
            vis = oscs[idx].get("visible", False)
            result.update(passed=vis, expected=True, actual=vis)
        else:
            result.update(detail=f"oscillator index {idx} out of range")

    elif check == "oscillator_hidden":
        idx = assertion.get("oscillator", 0)
        if idx < len(oscs):
            vis = oscs[idx].get("visible", True)
            result.update(passed=not vis, expected=False, actual=vis)
        else:
            result.update(detail=f"oscillator index {idx} out of range")

    elif check == "all_waveforms_have_data":
        all_have = all(o.get("captureBuffer", {}).get("hasData", False) for o in oscs)
        missing = [o["name"] for o in oscs if not o.get("captureBuffer", {}).get("hasData", False)]
        result.update(passed=all_have, expected="all have data",
                      actual=f"missing: {missing}" if missing else "all OK")

    elif check == "all_peaks_above":
        threshold = assertion.get("threshold", 0.1)
        peaks = [o.get("captureBuffer", {}).get("peak", 0) for o in oscs]
        all_ok = all(p > threshold for p in peaks)
        result.update(passed=all_ok, expected=f"all > {threshold}",
                      actual=[round(p, 4) for p in peaks])

    elif check == "snapshot_field":
        # Generic: check any field in the snapshot via dotted path
        path = assertion.get("path", "")
        op = assertion.get("op", "==")
        expected_val = assertion.get("value")
        actual_val = _resolve_path(snapshot, path)
        if op == "==":
            ok = actual_val == expected_val
        elif op == "!=":
            ok = actual_val != expected_val
        elif op == ">":
            ok = actual_val is not None and actual_val > expected_val
        elif op == "<":
            ok = actual_val is not None and actual_val < expected_val
        elif op == ">=":
            ok = actual_val is not None and actual_val >= expected_val
        elif op == "contains":
            ok = expected_val in (actual_val or "")
        else:
            ok = False
        result.update(passed=ok, expected=f"{path} {op} {expected_val}", actual=actual_val)

    else:
        result.update(detail=f"Unknown assertion type: {check}")

    return result


# ── Scenario Executor ───────────────────────────────────────────────────────

def execute_setup(client: OscilTestClient, setup: dict):
    """Execute scenario setup steps."""
    if setup.get("reset", True):
        client.reset_state()
        client.wait_until(lambda: len(client.get_oscillators()) == 0, timeout_s=3.0, desc="reset")

    if setup.get("open_editor", True):
        client.open_editor()

    # Transport
    if setup.get("transport") == "playing":
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport play")

    if "bpm" in setup:
        client.set_bpm(setup["bpm"])

    # Audio generator
    audio = setup.get("audio", {})
    if audio:
        client.set_track_audio(
            0,
            waveform=audio.get("waveform", "sine"),
            frequency=audio.get("frequency", 440.0),
            amplitude=audio.get("amplitude", 0.8),
        )

    # Oscillators
    for i, osc_def in enumerate(setup.get("oscillators", [])):
        source = osc_def.get("source", "auto")
        if source == "auto":
            sources = client.get_sources()
            source = sources[0]["id"] if sources else ""
        osc_id = client.add_oscillator(
            source,
            name=osc_def.get("name", f"Osc {i+1}"),
            colour=osc_def.get("color", ""),
            mode=osc_def.get("mode", "FullStereo"),
        )
        if osc_id is None:
            raise RuntimeError(f"Failed to add oscillator {i}")

    osc_count = len(setup.get("oscillators", []))
    if osc_count > 0:
        client.wait_for_oscillator_count(osc_count, timeout_s=5.0)

    # Timing
    if "timing_mode" in setup:
        mode_seg = f"sidebar_timing_modeToggle_{setup['timing_mode'].lower()}"
        timing_id = "sidebar_timing"
        if client.element_exists(timing_id):
            client.click(timing_id)
        if client.element_exists(mode_seg):
            client.click(mode_seg)

    if "interval_ms" in setup:
        field = "sidebar_timing_intervalField"
        if client.element_exists(field):
            client.set_slider(field, setup["interval_ms"])

    # Wait condition
    wait_for = setup.get("wait_for", "")
    if wait_for == "waveform_data":
        try:
            client.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        except TimeoutError:
            pass  # Snapshot will show the state; assertion will catch the failure
    elif wait_for and wait_for.endswith("ms"):
        time.sleep(int(wait_for[:-2]) / 1000.0)


def get_snapshot(client: OscilTestClient) -> dict:
    """Fetch the diagnostic snapshot."""
    resp = client._get_json("/diagnostic/snapshot")
    if resp and resp.get("success"):
        return resp.get("data", {})
    return {}


def run_scenario(client: OscilTestClient, scenario: dict) -> dict:
    """Run a single scenario. Returns structured result."""
    name = scenario.get("name", "unnamed")
    result = {"name": name, "passed": False, "assertions": [], "snapshot": None, "error": None}

    try:
        execute_setup(client, scenario.get("setup", {}))
        snapshot = get_snapshot(client)
        result["snapshot"] = snapshot

        assertions = scenario.get("assertions", [])
        all_passed = True
        for assertion_def in assertions:
            a_result = evaluate_assertion(assertion_def, snapshot)
            result["assertions"].append(a_result)
            if not a_result["passed"]:
                all_passed = False

        result["passed"] = all_passed

    except Exception as e:
        result["error"] = str(e)
        # Try to get snapshot even on error
        try:
            result["snapshot"] = get_snapshot(client)
        except Exception:
            pass

    # Cleanup
    try:
        client.transport_stop()
        client.close_editor()
    except Exception:
        pass

    return result


# ── Report Formatting ───────────────────────────────────────────────────────

def format_snapshot_summary(snapshot: dict) -> str:
    """Format a concise snapshot summary for terminal output."""
    if not snapshot:
        return "    (no snapshot available)"
    lines = []
    oscs = snapshot.get("oscillators", [])
    for i, o in enumerate(oscs):
        cb = o.get("captureBuffer", {})
        lines.append(f"    osc[{i}] {o.get('name','?')}: src={o.get('sourceId','?')} "
                     f"visible={o.get('visible')} peak={cb.get('peak',0):.4f} "
                     f"available={cb.get('available',0)} hasData={cb.get('hasData')}")
    t = snapshot.get("transport", {})
    lines.append(f"    transport: playing={t.get('playing')} bpm={t.get('bpm')} pos={t.get('positionSamples')}")
    tm = snapshot.get("timing", {})
    lines.append(f"    timing: mode={tm.get('mode')} interval={tm.get('actualIntervalMs')}ms "
                 f"displaySamples={tm.get('displaySamples')}")
    ag = snapshot.get("audioGenerators", [{}])
    if ag:
        g = ag[0]
        lines.append(f"    audio: {g.get('waveform')} freq={g.get('frequency')} "
                     f"amp={g.get('amplitude')} generating={g.get('generating')}")
    m = snapshot.get("metrics", {})
    lines.append(f"    metrics: fps={m.get('fps',0):.1f} mem={m.get('memoryMB',0):.1f}MB")
    logs = snapshot.get("logs", [])
    if logs:
        lines.append(f"    logs (last {min(5, len(logs))}):")
        for entry in logs[-5:]:
            lines.append(f"      [{entry.get('t',0)/1000:.2f}s] {entry.get('msg','')}")
    return "\n".join(lines)


def print_report(results: list, json_output: bool = False):
    """Print the diagnostic report."""
    if json_output:
        print(json_module.dumps(results, indent=2, default=str))
        return

    passed = sum(1 for r in results if r["passed"])
    failed = len(results) - passed

    print(f"\n{'='*60}")
    print(f" OSCIL E2E DIAGNOSTIC REPORT")
    print(f" Scenarios: {passed}/{len(results)} passed", end="")
    if failed:
        print(f", {failed} FAILED")
    else:
        print()
    print(f"{'='*60}\n")

    for r in results:
        status = "PASS" if r["passed"] else "FAIL"
        print(f"{'✓' if r['passed'] else '✗'} [{status}] {r['name']}")

        if r.get("error"):
            print(f"  ERROR: {r['error']}")

        if not r["passed"]:
            for a in r.get("assertions", []):
                mark = "✓" if a["passed"] else "✗"
                line = f"  {mark} {a['check']}"
                if a.get("detail"):
                    line += f" ({a['detail']})"
                line += f" — expected: {a['expected']}, actual: {a['actual']}"
                print(line)

            print(f"\n  Snapshot at failure:")
            print(format_snapshot_summary(r.get("snapshot", {})))
            print()

    if failed:
        sys.exit(1)


# ── Main ────────────────────────────────────────────────────────────────────

def load_scenarios(scenario_filter: str = "") -> list:
    """Load scenario definitions from YAML files."""
    if yaml is None:
        # Fallback: try to find JSON scenarios
        scenarios = []
        for f in sorted(SCENARIOS_DIR.glob("*.json")):
            with open(f) as fh:
                data = json_module.load(fh)
                if isinstance(data, list):
                    scenarios.extend(data)
        if not scenarios:
            print("ERROR: PyYAML not installed and no JSON scenarios found.", file=sys.stderr)
            print("Install: pip install pyyaml", file=sys.stderr)
            sys.exit(1)
        return scenarios

    scenarios = []
    files = sorted(SCENARIOS_DIR.glob("*.yaml")) + sorted(SCENARIOS_DIR.glob("*.yml"))
    for f in files:
        if scenario_filter and scenario_filter not in f.stem:
            continue
        with open(f) as fh:
            data = yaml.safe_load(fh)
            if isinstance(data, list):
                scenarios.extend(data)
    return scenarios


def main():
    parser = argparse.ArgumentParser(description="Oscil E2E Scenario Runner")
    parser.add_argument("--scenario", default="", help="Filter by scenario file name")
    parser.add_argument("--json", action="store_true", help="Output JSON report")
    parser.add_argument("--dry-run", action="store_true", help="Parse scenarios only")
    parser.add_argument("--url", default=URL, help="Harness URL")
    args = parser.parse_args()

    scenarios = load_scenarios(args.scenario)
    if not scenarios:
        print("No scenarios found in", SCENARIOS_DIR, file=sys.stderr)
        sys.exit(1)

    if args.dry_run:
        print(f"Parsed {len(scenarios)} scenarios:")
        for s in scenarios:
            n_checks = len(s.get("assertions", []))
            print(f"  - {s.get('name', 'unnamed')} ({n_checks} assertions)")
        return

    client = OscilTestClient(args.url)
    try:
        client.wait_for_harness(max_retries=5, delay=1.0)
    except HarnessConnectionError:
        print(f"ERROR: Test harness not running at {args.url}", file=sys.stderr)
        print("Start it with:", file=sys.stderr)
        print('  "./build/dev/test_harness/OscilTestHarness_artefacts/Debug/'
              'Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &', file=sys.stderr)
        sys.exit(1)

    results = []
    for scenario in scenarios:
        results.append(run_scenario(client, scenario))

    print_report(results, json_output=args.json)


if __name__ == "__main__":
    main()
