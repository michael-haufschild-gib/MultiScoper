#!/bin/bash
# Automated test script for ghost image bug when adding oscillators to new panes
# Reproduces: Start with no oscillator -> add one -> add another to new pane -> ghost images appear

set -e

API_BASE="http://127.0.0.1:8765"
LOG_FILE="/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"

echo "=== Ghost Image Bug: Add Oscillator to New Pane ==="
echo ""

# Clear log file
> "$LOG_FILE"
echo "✓ Cleared log file"

# Wait for test harness to be ready
echo "Checking test harness health..."
for i in {1..10}; do
    if curl -s "$API_BASE/health" > /dev/null 2>&1; then
        echo "✓ Test harness is running"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "✗ Test harness not responding. Please start it first."
        exit 1
    fi
    sleep 0.5
done

# Step 1: Reset state to clean slate (no oscillators)
echo ""
echo "Step 1: Resetting state (removing all oscillators)..."
curl -s -X POST "$API_BASE/state/reset" | jq -c .
sleep 0.5

# Step 2: Start transport
echo ""
echo "Step 2: Starting transport..."
curl -s -X POST "$API_BASE/transport/play" | jq -c .
sleep 0.3

# Step 3: Generate audio on track 1
echo ""
echo "Step 3: Generating audio..."
curl -s -X POST "$API_BASE/track/1/burst" \
    -H "Content-Type: application/json" \
    -d '{"waveform": "sine", "frequency": 440, "amplitude": 0.8, "durationMs": 5000}' | jq -c .
sleep 1.0

# Log marker before adding first oscillator
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== ADD OSCILLATOR 1 START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 4: Get initial pane state
echo ""
echo "Step 4: Initial pane state:"
PANES=$(curl -s "$API_BASE/state/panes")
echo "$PANES" | jq -c .
PANE1_ID=$(echo "$PANES" | jq -r '.data[0].id // empty')

if [ -z "$PANE1_ID" ]; then
    echo "No pane found, creating default pane..."
    PANE_RESULT=$(curl -s -X POST "$API_BASE/state/pane/add" \
        -H "Content-Type: application/json" \
        -d '{"name": "Pane 1"}')
    echo "$PANE_RESULT" | jq -c .
    PANE1_ID=$(echo "$PANE_RESULT" | jq -r '.data.id // empty')
    sleep 0.5
fi

echo "Pane 1 ID: $PANE1_ID"

# Step 5: Add first oscillator to Pane 1
echo ""
echo "Step 5: Adding first oscillator to Pane 1..."
curl -s -X POST "$API_BASE/state/oscillator/add" \
    -H "Content-Type: application/json" \
    -d "{\"name\": \"Osc 1\", \"paneId\": \"$PANE1_ID\", \"color\": \"#00FF00\"}" | jq -c .
sleep 1.0

echo '{"hypothesisId":"TEST","location":"test_script","message":"=== ADD OSCILLATOR 1 END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 6: Verify oscillator was added
echo ""
echo "Step 6: Current oscillators:"
curl -s "$API_BASE/state/oscillators" | jq -c .

# Step 7: Create a NEW pane (Pane 2)
echo ""
echo "Step 7: Creating Pane 2..."
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== CREATE PANE 2 START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

PANE2_RESULT=$(curl -s -X POST "$API_BASE/state/pane/add" \
    -H "Content-Type: application/json" \
    -d '{"name": "Pane 2"}')
echo "$PANE2_RESULT" | jq -c .
PANE2_ID=$(echo "$PANE2_RESULT" | jq -r '.data.id // empty')
echo "Pane 2 ID: $PANE2_ID"
sleep 1.0

echo '{"hypothesisId":"TEST","location":"test_script","message":"=== CREATE PANE 2 END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 8: Add second oscillator to Pane 2
echo ""
echo "Step 8: Adding second oscillator to Pane 2..."
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== ADD OSCILLATOR 2 START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

curl -s -X POST "$API_BASE/state/oscillator/add" \
    -H "Content-Type: application/json" \
    -d "{\"name\": \"Osc 2\", \"paneId\": \"$PANE2_ID\", \"color\": \"#0088FF\"}" | jq -c .
sleep 1.5

echo '{"hypothesisId":"TEST","location":"test_script","message":"=== ADD OSCILLATOR 2 END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 9: Get final state
echo ""
echo "Step 9: Final state:"
echo "Panes:"
curl -s "$API_BASE/state/panes" | jq .
echo "Oscillators:"
curl -s "$API_BASE/state/oscillators" | jq .

# Step 10: Wait for more frames to render
echo ""
echo "Step 10: Waiting for rendering to stabilize..."
sleep 2.0

echo ""
echo "=== Test Complete ==="
echo ""
echo "Log file analysis:"
echo "=================="
if [ -f "$LOG_FILE" ]; then
    echo "Total log entries: $(wc -l < "$LOG_FILE")"
    echo ""
    echo "=== Key Events (TEST markers) ==="
    grep "TEST" "$LOG_FILE" 2>/dev/null || echo "(No TEST markers)"
    echo ""
    echo "=== Waveform Updates (H7,H10) ==="
    grep "H7\|H10" "$LOG_FILE" 2>/dev/null | head -20 || echo "(No H7/H10 entries)"
    echo ""
    echo "=== Waveform Entries in Read Map ==="
    grep "waveformEntry" "$LOG_FILE" 2>/dev/null | head -30 || echo "(No waveformEntry logs)"
    echo ""
    echo "=== Sync Waveforms (H9) ==="
    grep "H9" "$LOG_FILE" 2>/dev/null | head -10 || echo "(No H9 entries)"
    echo ""
    echo "=== Scene FBO Clears (H5) ==="
    grep "beginFrame" "$LOG_FILE" 2>/dev/null | head -10 || echo "(No beginFrame logs)"
else
    echo "(No log file found)"
fi















