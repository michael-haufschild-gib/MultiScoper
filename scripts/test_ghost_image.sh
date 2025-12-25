#!/bin/bash
# Automated test script for ghost image bug reproduction
# This script uses the test harness HTTP API to trigger the ghost image issue

set -e

API_BASE="http://127.0.0.1:8765"
LOG_FILE="/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"

echo "=== Ghost Image Bug Reproduction Test ==="
echo ""

# Clear log file
rm -f "$LOG_FILE"
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

# Step 1: Reset state to clean slate
echo ""
echo "Step 1: Resetting state..."
curl -s -X POST "$API_BASE/state/reset" | jq -c .
sleep 0.5

# Step 2: Start transport and generate audio so we have a waveform
echo ""
echo "Step 2: Starting transport and generating audio..."
curl -s -X POST "$API_BASE/transport/play" | jq -c .
sleep 0.3

# Generate a burst of audio on track 1
curl -s -X POST "$API_BASE/track/1/burst" \
    -H "Content-Type: application/json" \
    -d '{"waveform": "sine", "frequency": 440, "amplitude": 0.8, "durationMs": 1000}' | jq -c .
sleep 1.0

# Step 3: Get current layout state
echo ""
echo "Step 3: Current pane state:"
curl -s "$API_BASE/state/panes" | jq -c .
sleep 0.3

# Step 4: Add log marker before layout change
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== LAYOUT CHANGE 1->2 START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 5: Click on layout dropdown to open it
echo ""
echo "Step 4: Clicking on layout dropdown..."
curl -s -X POST "$API_BASE/ui/click" \
    -H "Content-Type: application/json" \
    -d '{"testId": "sidebar_options_layoutDropdown"}' | jq -c .
sleep 0.5

# Step 6: Type key to select 2 columns (or click on menu item)
# We'll use keypress to navigate the dropdown
echo ""
echo "Step 5: Selecting 2 columns layout..."
curl -s -X POST "$API_BASE/ui/keyPress" \
    -H "Content-Type: application/json" \
    -d '{"key": "down"}' | jq -c .
sleep 0.2
curl -s -X POST "$API_BASE/ui/keyPress" \
    -H "Content-Type: application/json" \
    -d '{"key": "return"}' | jq -c .
sleep 1.0

# Step 7: Add log marker after layout change
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== LAYOUT CHANGE 1->2 END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 8: Get pane state after layout change
echo ""
echo "Step 6: Pane state after layout change:"
curl -s "$API_BASE/state/panes" | jq -c .
sleep 0.5

# Step 9: Take a screenshot for visual verification
echo ""
echo "Step 7: Taking screenshot after layout change..."
curl -s "$API_BASE/screenshot" | jq -r '.data.path // "Screenshot not available"'
sleep 0.5

# Step 10: Test GPU/CPU toggle
echo ""
echo "Step 8: Testing GPU toggle..."
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== GPU TOGGLE START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Click GPU toggle off
curl -s -X POST "$API_BASE/ui/click" \
    -H "Content-Type: application/json" \
    -d '{"testId": "sidebar_options_gpuRenderingToggle"}' | jq -c .
sleep 0.5

# Click GPU toggle back on
curl -s -X POST "$API_BASE/ui/click" \
    -H "Content-Type: application/json" \
    -d '{"testId": "sidebar_options_gpuRenderingToggle"}' | jq -c .
sleep 0.5

echo '{"hypothesisId":"TEST","location":"test_script","message":"=== GPU TOGGLE END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Step 11: Take another screenshot
echo ""
echo "Step 9: Taking screenshot after GPU toggle..."
curl -s "$API_BASE/screenshot" | jq -r '.data.path // "Screenshot not available"'

# Step 12: Change back to 1 column
echo ""
echo "Step 10: Changing back to 1-column layout..."
echo '{"hypothesisId":"TEST","location":"test_script","message":"=== LAYOUT CHANGE 2->1 START ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

curl -s -X POST "$API_BASE/ui/click" \
    -H "Content-Type: application/json" \
    -d '{"testId": "sidebar_options_layoutDropdown"}' | jq -c .
sleep 0.3
curl -s -X POST "$API_BASE/ui/keyPress" \
    -H "Content-Type: application/json" \
    -d '{"key": "up"}' | jq -c .
sleep 0.2
curl -s -X POST "$API_BASE/ui/keyPress" \
    -H "Content-Type: application/json" \
    -d '{"key": "return"}' | jq -c .
sleep 1.0

echo '{"hypothesisId":"TEST","location":"test_script","message":"=== LAYOUT CHANGE 2->1 END ===","timestamp":'$(date +%s000)'}' >> "$LOG_FILE"

# Final screenshot
echo ""
echo "Step 11: Taking final screenshot..."
curl -s "$API_BASE/screenshot" | jq -r '.data.path // "Screenshot not available"'

echo ""
echo "=== Test Complete ==="
echo ""
echo "Log file analysis:"
echo "=================="
if [ -f "$LOG_FILE" ]; then
    echo "Total log entries: $(wc -l < "$LOG_FILE")"
    echo ""
    echo "Key events:"
    grep -E "TEST|refreshPanels|resized|beginFrame|clearAllWaveforms" "$LOG_FILE" 2>/dev/null | head -50 || echo "(No matching entries)"
    echo ""
    echo "Full log (first 100 lines):"
    head -100 "$LOG_FILE"
else
    echo "(No log file found)"
fi
