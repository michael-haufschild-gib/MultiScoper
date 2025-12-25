#!/bin/bash
# Comprehensive automated test for ghost image bugs
# Tests three scenarios:
# 1. Add oscillator to new pane (stale waveform data)
# 2. GPU -> CPU -> GPU toggle (render mode switch ghosting)
# 3. Pane swap via drag-and-drop simulation

set -e

API_BASE="http://127.0.0.1:8765"
LOG_FILE="/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"

log_marker() {
    echo "{\"hypothesisId\":\"TEST\",\"location\":\"test_script\",\"message\":\"$1\",\"timestamp\":$(date +%s000)}" >> "$LOG_FILE"
}

echo "=========================================="
echo "=== Ghost Image Bug - All Scenarios ==="
echo "=========================================="
echo ""

# Clear log file
> "$LOG_FILE"
echo "✓ Cleared log file"

# Wait for test harness
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

###########################################
# SCENARIO 1: Add Oscillator to New Pane
###########################################
echo ""
echo "=========================================="
echo "SCENARIO 1: Add Oscillator to New Pane"
echo "=========================================="

# Reset state
curl -s -X POST "$API_BASE/state/reset" | jq -c .
sleep 0.5

# Start transport and generate audio
curl -s -X POST "$API_BASE/transport/play" | jq -c .
curl -s -X POST "$API_BASE/track/1/burst" -H "Content-Type: application/json" \
    -d '{"waveform": "sine", "frequency": 440, "amplitude": 0.8, "durationMs": 10000}' | jq -c .
sleep 0.5

log_marker "=== SCENARIO 1: ADD OSC TO NEW PANE START ==="

# Create pane 1 and add oscillator
PANE1=$(curl -s -X POST "$API_BASE/state/pane/add" -H "Content-Type: application/json" -d '{"name": "Pane 1"}')
PANE1_ID=$(echo "$PANE1" | jq -r '.data.id')
echo "Created Pane 1: $PANE1_ID"
sleep 0.3

curl -s -X POST "$API_BASE/state/oscillator/add" -H "Content-Type: application/json" \
    -d "{\"name\": \"Osc 1\", \"paneId\": \"$PANE1_ID\", \"color\": \"#00FF00\"}" | jq -c .
sleep 1.0

log_marker "=== SCENARIO 1: OSC 1 ADDED, NOW ADDING PANE 2 ==="

# Create pane 2 and add oscillator
PANE2=$(curl -s -X POST "$API_BASE/state/pane/add" -H "Content-Type: application/json" -d '{"name": "Pane 2"}')
PANE2_ID=$(echo "$PANE2" | jq -r '.data.id')
echo "Created Pane 2: $PANE2_ID"
sleep 0.5

curl -s -X POST "$API_BASE/state/oscillator/add" -H "Content-Type: application/json" \
    -d "{\"name\": \"Osc 2\", \"paneId\": \"$PANE2_ID\", \"color\": \"#0088FF\"}" | jq -c .
sleep 1.5

log_marker "=== SCENARIO 1: COMPLETE ==="

# Check for stale waveforms
echo ""
echo "Scenario 1 Results:"
STALE_COUNT=$(grep "waveformEntry" "$LOG_FILE" 2>/dev/null | grep "boundsH\":740" | wc -l | tr -d ' ')
CORRECT_COUNT=$(grep "waveformEntry" "$LOG_FILE" 2>/dev/null | grep "boundsH\":352" | wc -l | tr -d ' ')
echo "  Stale waveform entries (H=740): $STALE_COUNT"
echo "  Correct waveform entries (H=352): $CORRECT_COUNT"

###########################################
# SCENARIO 2: GPU -> CPU -> GPU Toggle
###########################################
echo ""
echo "=========================================="
echo "SCENARIO 2: GPU -> CPU -> GPU Toggle"
echo "=========================================="

log_marker "=== SCENARIO 2: GPU TOGGLE START ==="

# Current state: 2 panes, 2 oscillators, GPU enabled
echo "Step 1: Disabling GPU rendering..."
curl -s -X POST "$API_BASE/state/gpu" -H "Content-Type: application/json" -d '{"enabled": false}' | jq -c .
sleep 1.0

log_marker "=== SCENARIO 2: GPU DISABLED, CPU RENDERING ==="

echo "Step 2: Wait for CPU rendering..."
sleep 1.5

log_marker "=== SCENARIO 2: RE-ENABLING GPU ==="

echo "Step 3: Re-enabling GPU rendering..."
curl -s -X POST "$API_BASE/state/gpu" -H "Content-Type: application/json" -d '{"enabled": true}' | jq -c .
sleep 1.5

log_marker "=== SCENARIO 2: GPU RE-ENABLED, COMPLETE ==="

echo "Scenario 2 Results:"
CLEARS_AFTER=$(grep "H3-FIX\|clearAllWaveforms" "$LOG_FILE" 2>/dev/null | wc -l | tr -d ' ')
echo "  Clear waveforms calls: $CLEARS_AFTER"

###########################################
# SCENARIO 3: Pane Swap (Drag and Drop)
###########################################
echo ""
echo "=========================================="
echo "SCENARIO 3: Pane Swap (Drag and Drop)"
echo "=========================================="

log_marker "=== SCENARIO 3: PANE SWAP START ==="

echo "Current pane order:"
curl -s "$API_BASE/state/panes" | jq '.data[] | {name, order}'

echo ""
echo "Swapping panes (move pane 0 to position 1)..."
curl -s -X POST "$API_BASE/state/pane/swap" -H "Content-Type: application/json" \
    -d '{"fromIndex": 0, "toIndex": 1}' | jq -c .
sleep 1.5

log_marker "=== SCENARIO 3: PANE SWAP EXECUTED ==="

echo "New pane order:"
curl -s "$API_BASE/state/panes" | jq '.data[] | {name, order}'
sleep 1.0

log_marker "=== SCENARIO 3: COMPLETE ==="

###########################################
# FINAL ANALYSIS
###########################################
echo ""
echo "=========================================="
echo "=== FINAL LOG ANALYSIS ==="
echo "=========================================="

echo ""
echo "Total log entries: $(wc -l < "$LOG_FILE")"

echo ""
echo "=== TEST Markers ==="
grep "TEST" "$LOG_FILE" 2>/dev/null | head -20

echo ""
echo "=== Sync Waveforms (checking for stateCount > activeCount) ==="
grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | grep -v "activeCount.*stateCount.\([0-9]\+\).*\1" | head -10 || echo "  (All sync counts match - GOOD)"

echo ""
echo "=== Clear Waveforms Calls ==="
grep "clearAllWaveforms\|H3-FIX" "$LOG_FILE" 2>/dev/null | head -10 || echo "  (No clear calls found)"

echo ""
echo "=== Scene FBO Background Colors ==="
grep "beginFrame" "$LOG_FILE" 2>/dev/null | head -5

echo ""
echo "=== Unique Waveform IDs Being Rendered ==="
grep "waveformEntry" "$LOG_FILE" 2>/dev/null | jq -r '.data.id' 2>/dev/null | sort -u | head -10 || echo "  (Could not parse)"

echo ""
echo "=========================================="
echo "=== TEST COMPLETE ==="
echo "=========================================="


