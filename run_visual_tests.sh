#!/bin/bash
set -e

# Build the harness
echo "Building Test Harness..."
cmake --build --preset dev --target OscilTestHarness

# Ensure virtualenv
if [ ! -d ".venv" ]; then
    echo "Creating virtualenv..."
    python3 -m venv .venv
    .venv/bin/pip install requests Pillow numpy
fi

# Kill any existing harness
pkill -f "Oscil Test Harness" || true

# Start harness in background using 'open' to ensure window creation on macOS
echo "Starting Harness..."
open "./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app"

# Run tests
echo "Running Visual Verification Tests..."
.venv/bin/python tests/e2e/visual_verification_test.py

# Cleanup
echo "Cleaning up..."
pkill -f "Oscil Test Harness" || true
rm -f /tmp/debug_*.png
echo "Done."
