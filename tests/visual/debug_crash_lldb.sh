#!/bin/bash
HARNESS_APP="build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"

# Start LLDB in batch mode, run the app, and print backtrace on crash
lldb -b -o "process launch" -o "continue" -o "thread backtrace all" -o "quit" -- "$HARNESS_APP" > lldb_crash.log 2>&1 &
LLDB_PID=$!

echo "LLDB started with PID $LLDB_PID"

# Wait for harness to initialize
sleep 10

# Run the trigger script
echo "Running trigger script..."
python3 tests/visual/debug_harness.py

# Give it a moment to crash and log
sleep 5

# Kill LLDB if it's still running (it should exit after 'quit' if app exits/crashes, but 'continue' might hang if no crash)
kill $LLDB_PID 2>/dev/null

echo "LLDB Log:"
cat lldb_crash.log
