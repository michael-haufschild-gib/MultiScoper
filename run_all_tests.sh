#!/bin/bash

# Configuration
HARNESS_BIN="build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"
TEST_SCRIPT="tests/e2e/run_all_e2e_tests.py"
LOG_FILE="test_harness_all.log"

# Kill any existing instances
pkill "Oscil Test Harness"

# Start the harness in background
echo "Starting Test Harness..."
"$HARNESS_BIN" > "$LOG_FILE" 2>&1 &
HARNESS_PID=$!

# Wait for harness to start
sleep 5

# Run the test
echo "Running All E2E Tests..."
python3 "$TEST_SCRIPT"
EXIT_CODE=$?

# Cleanup
echo "Stopping Test Harness..."
kill $HARNESS_PID
wait $HARNESS_PID 2>/dev/null

# Exit with test result
exit $EXIT_CODE
