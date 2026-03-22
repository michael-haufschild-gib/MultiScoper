#!/usr/bin/env python3
"""
Oscil E2E Test Suite Runner

Wrapper that invokes pytest with the correct virtualenv and working directory.

Usage:
    python run_all_e2e_tests.py              # Run all tests
    python run_all_e2e_tests.py --quick      # Run only fast tests (skip slow marker)
    python run_all_e2e_tests.py -k sidebar   # Run tests matching 'sidebar'
    python run_all_e2e_tests.py --collect     # List tests without running
"""

import os
import sys
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
VENV_PYTHON = os.path.join(SCRIPT_DIR, ".venv", "bin", "python")


def ensure_venv():
    """Create venv and install dependencies if missing."""
    if not os.path.exists(VENV_PYTHON):
        print("[SETUP] Creating virtual environment...")
        subprocess.check_call([sys.executable, "-m", "venv", os.path.join(SCRIPT_DIR, ".venv")])
        subprocess.check_call([VENV_PYTHON, "-m", "pip", "install", "-q", "pytest", "requests"])
    return VENV_PYTHON


def main():
    python = ensure_venv()

    # Build pytest command
    cmd = [python, "-m", "pytest"]

    # Pass through any CLI arguments
    extra_args = sys.argv[1:]

    if "--quick" in extra_args:
        extra_args.remove("--quick")
        extra_args.extend(["-m", "not slow"])

    if "--collect" in extra_args:
        extra_args.remove("--collect")
        extra_args.append("--collect-only")

    cmd.extend(extra_args)

    # Run from the e2e directory so pytest.ini is picked up
    result = subprocess.run(cmd, cwd=SCRIPT_DIR)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
