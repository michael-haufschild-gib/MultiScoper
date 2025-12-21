#!/bin/bash

# Configuration
HARNESS_BIN="build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"
TEST_SCRIPT="tests/e2e/test_add_oscillator_journey.py"
LOG_FILE="test_harness.log"

# Kill any existing instances
pkill "Oscil Test Harness"

# Start the harness in background
echo "Starting Test Harness..."
"$HARNESS_BIN" > "$LOG_FILE" 2>&1 &
HARNESS_PID=$!

# Wait for harness to start (Python script also waits, but this is a safety net)
sleep 5

# Run the test
echo "Running Python Test..."
python3 "$TEST_SCRIPT"
EXIT_CODE=$?

# Cleanup
echo "Stopping Test Harness..."
kill $HARNESS_PID
wait $HARNESS_PID 2>/dev/null

# Exit with test result
exit $EXIT_CODE
