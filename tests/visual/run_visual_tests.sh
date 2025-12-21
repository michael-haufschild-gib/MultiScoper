#!/bin/bash

# Path to harness executable
HARNESS_APP="build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"
LOG_FILE="tests/visual/harness.log"

if [ ! -f "$HARNESS_APP" ]; then
    echo "Error: Harness not found at $HARNESS_APP"
    exit 1
fi

echo "Starting Harness..."
"$HARNESS_APP" > "$LOG_FILE" 2>&1 &
HARNESS_PID=$!

echo "Harness PID: $HARNESS_PID"

# Wait for harness to start
sleep 5

echo "Running Tests..."
python3 tests/visual/test_visual_rendering.py
TEST_EXIT_CODE=$?

echo "Stopping Harness..."
kill $HARNESS_PID

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "Tests Passed!"
else
    echo "Tests Failed!"
    echo "Harness Log:"
    cat "$LOG_FILE"
fi

exit $TEST_EXIT_CODE
