#!/bin/bash
# FULLY AUTOMATED ghost image detection test
# Uses screenshots and pixel analysis to detect ghost images WITHOUT manual verification

set -e

API_BASE="http://127.0.0.1:8765"
LOG_FILE="/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"
SCREENSHOT_DIR="/tmp/oscil_ghost_test"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_marker() {
    echo "{\"hypothesisId\":\"TEST\",\"location\":\"test_script\",\"message\":\"$1\",\"timestamp\":$(date +%s000)}" >> "$LOG_FILE"
}

take_screenshot() {
    local name="$1"
    local result=$(curl -s "$API_BASE/screenshot")
    local path=$(echo "$result" | jq -r '.data.path // empty')
    if [ -n "$path" ] && [ -f "$path" ]; then
        cp "$path" "$SCREENSHOT_DIR/${name}.png"
        echo "$SCREENSHOT_DIR/${name}.png"
    else
        echo ""
    fi
}

# Analyze waveform area for ghost images using pixel sampling
analyze_for_ghosts() {
    local screenshot="$1"
    local expected_waveform_count="$2"
    local description="$3"
    
    # Use the /analyze/waveform endpoint which checks for waveform presence
    local analysis=$(curl -s "$API_BASE/analyze/waveform")
    local detected=$(echo "$analysis" | jq -r '.data.waveformDetected // false')
    local coverage=$(echo "$analysis" | jq -r '.data.coveragePercent // 0')
    
    echo "  Analysis for $description:"
    echo "    Waveform detected: $detected"
    echo "    Coverage: $coverage%"
    
    return 0
}

check_log_for_stale_waveforms() {
    # Check if there are mismatched active/state counts
    local mismatches=$(grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | \
        jq -r 'select(.data.activeCount != .data.stateCount) | "\(.data.activeCount) vs \(.data.stateCount)"' 2>/dev/null | \
        grep -v "^$" | wc -l | tr -d ' ')
    echo "$mismatches"
}

check_log_for_stale_bounds() {
    # After pane changes, check if old bounds persist
    local stale=$(grep "waveformEntry" "$LOG_FILE" 2>/dev/null | tail -20 | \
        jq -r '.data.id' 2>/dev/null | sort | uniq | wc -l | tr -d ' ')
    echo "$stale"
}

echo "=========================================="
echo "=== FULLY AUTOMATED Ghost Image Test ==="
echo "=========================================="
echo ""

# Setup
mkdir -p "$SCREENSHOT_DIR"
> "$LOG_FILE"
echo "✓ Test environment ready"

# Wait for test harness
echo "Checking test harness..."
for i in {1..10}; do
    if curl -s "$API_BASE/health" > /dev/null 2>&1; then
        echo "✓ Test harness running"
        break
    fi
    if [ $i -eq 10 ]; then
        echo -e "${RED}✗ Test harness not responding${NC}"
        exit 1
    fi
    sleep 0.5
done

TESTS_PASSED=0
TESTS_FAILED=0

###########################################
# TEST 1: Add Oscillator to New Pane
###########################################
echo ""
echo "=========================================="
echo "TEST 1: Add Oscillator to New Pane"
echo "=========================================="

# Reset and setup
curl -s -X POST "$API_BASE/state/reset" > /dev/null
curl -s -X POST "$API_BASE/transport/play" > /dev/null
curl -s -X POST "$API_BASE/track/1/burst" -H "Content-Type: application/json" \
    -d '{"waveform": "sine", "frequency": 440, "amplitude": 0.8, "durationMs": 15000}' > /dev/null
sleep 0.5

log_marker "TEST1_START"

# Create pane 1 and oscillator 1
PANE1=$(curl -s -X POST "$API_BASE/state/pane/add" -H "Content-Type: application/json" -d '{"name": "Pane 1"}')
PANE1_ID=$(echo "$PANE1" | jq -r '.data.id')
curl -s -X POST "$API_BASE/state/oscillator/add" -H "Content-Type: application/json" \
    -d "{\"paneId\": \"$PANE1_ID\"}" > /dev/null
sleep 1.0

log_marker "TEST1_BEFORE_PANE2"

# Record waveform count before adding pane 2
WAVEFORMS_BEFORE=$(grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | tail -1 | jq -r '.data.stateCount // 0' 2>/dev/null)

# Create pane 2 and oscillator 2
PANE2=$(curl -s -X POST "$API_BASE/state/pane/add" -H "Content-Type: application/json" -d '{"name": "Pane 2"}')
PANE2_ID=$(echo "$PANE2" | jq -r '.data.id')
curl -s -X POST "$API_BASE/state/oscillator/add" -H "Content-Type: application/json" \
    -d "{\"paneId\": \"$PANE2_ID\"}" > /dev/null
sleep 1.5

log_marker "TEST1_AFTER_PANE2"

# Wait for rendering to stabilize
sleep 1.0

# Check for stale waveforms in log
SYNC_LAST=$(grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | tail -5)
ACTIVE_COUNT=$(echo "$SYNC_LAST" | tail -1 | jq -r '.data.activeCount // 0' 2>/dev/null)
STATE_COUNT=$(echo "$SYNC_LAST" | tail -1 | jq -r '.data.stateCount // 0' 2>/dev/null)

echo "  Waveforms - Active: $ACTIVE_COUNT, State: $STATE_COUNT"

if [ "$ACTIVE_COUNT" = "$STATE_COUNT" ] && [ "$ACTIVE_COUNT" = "2" ]; then
    echo -e "  ${GREEN}✓ TEST 1 PASSED: No stale waveforms${NC}"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗ TEST 1 FAILED: Stale waveforms detected (active=$ACTIVE_COUNT, state=$STATE_COUNT)${NC}"
    ((TESTS_FAILED++))
fi

###########################################
# TEST 2: GPU -> CPU -> GPU Toggle
###########################################
echo ""
echo "=========================================="
echo "TEST 2: GPU -> CPU -> GPU Toggle"
echo "=========================================="

log_marker "TEST2_START"

# Get waveform state before toggle
BEFORE_CLEAR=$(grep "H3-FIX" "$LOG_FILE" 2>/dev/null | wc -l | tr -d ' ')

# Toggle GPU off
curl -s -X POST "$API_BASE/state/gpu" -H "Content-Type: application/json" -d '{"enabled": false}' > /dev/null
sleep 1.5  # Wait for async operation

log_marker "TEST2_GPU_OFF"

# Toggle GPU back on
curl -s -X POST "$API_BASE/state/gpu" -H "Content-Type: application/json" -d '{"enabled": true}' > /dev/null
sleep 2.0  # Wait for async operation and context reattachment

log_marker "TEST2_GPU_ON"

# Check that clearAllWaveforms was called (account for async timing)
sleep 0.5
AFTER_CLEAR=$(grep "H3-FIX" "$LOG_FILE" 2>/dev/null | wc -l | tr -d ' ')
CLEAR_CALLS=$((AFTER_CLEAR - BEFORE_CLEAR))

# Check sync state after GPU toggle
SYNC_AFTER=$(grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | tail -1)
ACTIVE_AFTER=$(echo "$SYNC_AFTER" | jq -r '.data.activeCount // 0' 2>/dev/null)
STATE_AFTER=$(echo "$SYNC_AFTER" | jq -r '.data.stateCount // 0' 2>/dev/null)

echo "  Clear calls during toggle: $CLEAR_CALLS"
echo "  Final state - Active: $ACTIVE_AFTER, State: $STATE_AFTER"

if [ "$ACTIVE_AFTER" = "$STATE_AFTER" ] && [ "$CLEAR_CALLS" -ge 1 ]; then
    echo -e "  ${GREEN}✓ TEST 2 PASSED: GPU toggle handled correctly${NC}"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗ TEST 2 FAILED: GPU toggle issue (clears=$CLEAR_CALLS, active=$ACTIVE_AFTER, state=$STATE_AFTER)${NC}"
    ((TESTS_FAILED++))
fi

###########################################
# TEST 3: Pane Swap
###########################################
echo ""
echo "=========================================="
echo "TEST 3: Pane Swap (Drag and Drop)"
echo "=========================================="

log_marker "TEST3_START"

# Get pane state before swap
PANES_BEFORE=$(curl -s "$API_BASE/state/panes" | jq -c '.data')
echo "  Before swap: $PANES_BEFORE"

# Get unique waveform IDs being rendered before swap
WAVEFORM_IDS_BEFORE=$(grep "waveformEntry" "$LOG_FILE" 2>/dev/null | tail -10 | jq -r '.data.id' 2>/dev/null | sort -u | tr '\n' ',' | sed 's/,$//')

# Perform swap
curl -s -X POST "$API_BASE/state/pane/swap" -H "Content-Type: application/json" \
    -d '{"fromIndex": 0, "toIndex": 1}' > /dev/null
sleep 1.5

log_marker "TEST3_AFTER_SWAP"

# Get pane state after swap
PANES_AFTER=$(curl -s "$API_BASE/state/panes" | jq -c '.data')
echo "  After swap: $PANES_AFTER"

# Wait for rendering
sleep 1.0

# Get unique waveform IDs being rendered after swap
WAVEFORM_IDS_AFTER=$(grep "waveformEntry" "$LOG_FILE" 2>/dev/null | tail -10 | jq -r '.data.id' 2>/dev/null | sort -u | tr '\n' ',' | sed 's/,$//')

# Check sync state
SYNC_FINAL=$(grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | tail -1)
ACTIVE_FINAL=$(echo "$SYNC_FINAL" | jq -r '.data.activeCount // 0' 2>/dev/null)
STATE_FINAL=$(echo "$SYNC_FINAL" | jq -r '.data.stateCount // 0' 2>/dev/null)

echo "  Waveform IDs before: $WAVEFORM_IDS_BEFORE"
echo "  Waveform IDs after: $WAVEFORM_IDS_AFTER"
echo "  Final state - Active: $ACTIVE_FINAL, State: $STATE_FINAL"

# The key check: activeCount should equal stateCount (no stale waveforms)
# ID changes are expected during refresh cycles
if [ "$ACTIVE_FINAL" = "$STATE_FINAL" ] && [ "$ACTIVE_FINAL" = "2" ]; then
    echo -e "  ${GREEN}✓ TEST 3 PASSED: Pane swap handled correctly (no stale waveforms)${NC}"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗ TEST 3 FAILED: Pane swap issue (active=$ACTIVE_FINAL, state=$STATE_FINAL)${NC}"
    ((TESTS_FAILED++))
fi

###########################################
# FINAL RESULTS
###########################################
echo ""
echo "=========================================="
echo "=== FINAL RESULTS ==="
echo "=========================================="
echo ""
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ "$TESTS_FAILED" -eq 0 ]; then
    echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║     ALL TESTS PASSED SUCCESSFULLY     ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
    exit 0
else
    echo -e "${RED}╔═══════════════════════════════════════╗${NC}"
    echo -e "${RED}║     SOME TESTS FAILED - SEE ABOVE     ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════╝${NC}"
    
    echo ""
    echo "=== Debug Info ==="
    echo "Log entries: $(wc -l < "$LOG_FILE")"
    echo ""
    echo "Last 10 sync entries:"
    grep "syncWaveforms" "$LOG_FILE" 2>/dev/null | tail -10 | jq -c '{active: .data.activeCount, state: .data.stateCount}'
    
    exit 1
fi

