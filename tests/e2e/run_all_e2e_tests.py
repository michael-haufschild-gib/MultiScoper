#!/usr/bin/env python3
"""
Oscil E2E Test Suite Runner

Runs all E2E tests in sequence and provides a comprehensive summary.

Usage:
    python run_all_e2e_tests.py [--quick] [--test NAME]

Options:
    --quick     Run only quick sanity tests
    --test NAME Run only the specified test module
"""

import sys
import os
import subprocess
import time
import argparse
from datetime import datetime

# Add current directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Test modules to run (in order)
TEST_MODULES = [
    # Original tests
    ("test_add_oscillator_journey", "Add Oscillator User Journey"),
    ("test_edit_oscillator_journey", "Edit Oscillator User Journey"),
    ("test_delete_oscillator_journey", "Delete Oscillator User Journey"),
    ("test_sidebar_interactions", "Sidebar Interactions"),
    ("test_timing_section", "Timing Section Controls"),
    ("test_theme_switching", "Theme Switching"),
    ("test_transport_and_timing", "Transport and Timing Controls"),
    # New tests - Options Section
    ("test_options_section", "Options Section Controls"),
    # New tests - Dialogs
    ("test_oscillator_config_dialog", "Oscillator Config Dialog"),
    ("test_preset_browser_dialog", "Preset Browser Dialog"),
    ("test_preset_editor_dialog", "Preset Editor Dialog"),
    # New tests - Pane Interactions
    ("test_pane_interactions", "Pane Interactions"),
    # New tests - End-to-End Verification
    ("test_execution_paths", "Execution Path Verification"),
    # New tests - Keyboard
    ("test_keyboard_shortcuts", "Keyboard Shortcuts"),
]

# Quick sanity tests (subset of above)
QUICK_TESTS = [
    ("test_add_oscillator_journey", "Add Oscillator User Journey"),
    ("test_sidebar_interactions", "Sidebar Interactions"),
    ("test_options_section", "Options Section Controls"),
]


def check_harness_running() -> bool:
    """Check if the test harness is running"""
    import requests
    try:
        response = requests.get("http://localhost:8765/health", timeout=2.0)
        return response.status_code == 200
    except:
        return False


def run_test_module(module_name: str, description: str) -> tuple:
    """Run a single test module and return (passed, failed, duration)"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    module_path = os.path.join(script_dir, f"{module_name}.py")

    if not os.path.exists(module_path):
        print(f"  [SKIP] Module not found: {module_path}")
        return (0, 0, 0.0)

    start_time = time.time()

    try:
        result = subprocess.run(
            [sys.executable, module_path],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per module
        )

        duration = time.time() - start_time

        # Parse results from output
        output = result.stdout + result.stderr
        passed = output.count("[PASS]")
        failed = output.count("[FAIL]")

        # Print output
        print(output)

        if result.returncode != 0:
            print(f"  [FAIL] Module exited with code {result.returncode}")
            if failed == 0:
                failed = 1

        return (passed, failed, duration)

    except subprocess.TimeoutExpired:
        print(f"  [FAIL] Module timed out after 5 minutes")
        return (0, 1, 300.0)
    except Exception as e:
        print(f"  [FAIL] Error running module: {e}")
        return (0, 1, time.time() - start_time)


def main():
    parser = argparse.ArgumentParser(description="Run Oscil E2E Test Suite")
    parser.add_argument("--quick", action="store_true", help="Run only quick sanity tests")
    parser.add_argument("--test", type=str, help="Run only specified test module")
    args = parser.parse_args()

    print("=" * 70)
    print("OSCIL E2E TEST SUITE")
    print(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    # Check harness
    print("\n[1] Checking test harness connection...")
    if not check_harness_running():
        print("[ERROR] Test harness is not running!")
        print("Please start the test harness first:")
        print("  ./build/OscilTestHarness_artefacts/OscilTestHarness.app/Contents/MacOS/OscilTestHarness")
        sys.exit(1)
    print("[OK] Test harness is running")

    # Determine which tests to run
    if args.test:
        # Run specific test
        test_modules = [(args.test, args.test.replace("_", " ").title())]
    elif args.quick:
        test_modules = QUICK_TESTS
    else:
        test_modules = TEST_MODULES

    print(f"\n[2] Running {len(test_modules)} test module(s)...")

    # Track results
    total_passed = 0
    total_failed = 0
    total_duration = 0.0
    module_results = []

    for i, (module_name, description) in enumerate(test_modules):
        print(f"\n{'='*70}")
        print(f"[{i+1}/{len(test_modules)}] {description}")
        print(f"{'='*70}")

        passed, failed, duration = run_test_module(module_name, description)

        total_passed += passed
        total_failed += failed
        total_duration += duration

        module_results.append({
            "name": description,
            "passed": passed,
            "failed": failed,
            "duration": duration,
            "status": "PASS" if failed == 0 else "FAIL"
        })

    # Print summary
    print("\n" + "=" * 70)
    print("TEST SUITE SUMMARY")
    print("=" * 70)

    for result in module_results:
        status_icon = "[PASS]" if result["status"] == "PASS" else "[FAIL]"
        print(f"  {status_icon} {result['name']}: {result['passed']} passed, {result['failed']} failed ({result['duration']:.1f}s)")

    print("-" * 70)
    print(f"TOTAL: {total_passed} passed, {total_failed} failed")
    print(f"Duration: {total_duration:.1f} seconds")
    print(f"Finished: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    if total_failed > 0:
        print("\n[RESULT] SOME TESTS FAILED")
        sys.exit(1)
    else:
        print("\n[RESULT] ALL TESTS PASSED")
        sys.exit(0)


if __name__ == "__main__":
    main()
