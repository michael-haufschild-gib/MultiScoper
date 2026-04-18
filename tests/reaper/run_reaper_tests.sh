#!/usr/bin/env bash
# Oscil Reaper on-demand integration test runner.
# Launches Reaper with a Lua driver, reads JSON results, reports pass/fail.

set -euo pipefail

# --- Resolve paths relative to this script, so invocation from any cwd works. ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# --- Configurable via env ---
REAPER_PATH="${REAPER_PATH:-/Applications/REAPER.app/Contents/MacOS/REAPER}"
OSCIL_PLUGIN_DIR="${OSCIL_PLUGIN_DIR:-${REPO_ROOT}/build/dev/Oscil_artefacts/Debug/VST3}"
RESULTS_FILE="${RESULTS_FILE:-/tmp/oscil_reaper_results.json}"

DRIVER_LUA="${SCRIPT_DIR}/lib/run_all.lua"
SCENARIOS_DIR="${SCRIPT_DIR}/scenarios"

# Optional single-scenario filter
SCENARIO_FILTER="${1:-}"

log()  { printf '[reaper-tests] %s\n' "$*"; }
fail() { printf '[reaper-tests][ERROR] %s\n' "$*" >&2; exit 1; }

# --- Pre-flight ---
if [[ ! -x "${REAPER_PATH}" ]]; then
  fail "Reaper binary not found at ${REAPER_PATH}. Install Reaper or set REAPER_PATH."
fi

if [[ ! -d "${OSCIL_PLUGIN_DIR}" ]]; then
  fail "Oscil plugin dir missing: ${OSCIL_PLUGIN_DIR}
Build the plugin first (cmake --build --preset dev) or set OSCIL_PLUGIN_DIR."
fi

if ! compgen -G "${OSCIL_PLUGIN_DIR}/oscil4.vst3" > /dev/null; then
  fail "oscil4.vst3 not found in ${OSCIL_PLUGIN_DIR}. Build the plugin first."
fi

if [[ ! -f "${DRIVER_LUA}" ]]; then
  fail "Driver missing: ${DRIVER_LUA}"
fi

# --- Reset results file so we only see results from this run. ---
rm -f "${RESULTS_FILE}"

# --- Export context for Lua driver (Reaper exposes OS env to scripts). ---
export OSCIL_TEST_SCENARIOS_DIR="${SCENARIOS_DIR}"
export OSCIL_TEST_RESULTS_FILE="${RESULTS_FILE}"
export OSCIL_TEST_SCENARIO_FILTER="${SCENARIO_FILTER}"
export OSCIL_TEST_LIB_DIR="${SCRIPT_DIR}/lib"

log "Reaper:      ${REAPER_PATH}"
log "Plugin dir:  ${OSCIL_PLUGIN_DIR}"
log "Scenarios:   ${SCENARIOS_DIR}"
log "Results:     ${RESULTS_FILE}"
if [[ -n "${SCENARIO_FILTER}" ]]; then
  log "Filter:      ${SCENARIO_FILTER}"
fi

# --- Launch Reaper.
# -nonewinst: do not spawn a second Reaper instance if one is already running.
# -new:       start with a new/empty project so we don't clobber user state.
# Trailing arg: path to the driver .lua — Reaper auto-runs ReaScripts passed this way.
log "Launching Reaper..."
"${REAPER_PATH}" -nonewinst -new "${DRIVER_LUA}" || true

# --- Wait for results file.
# The driver writes synchronously on completion; bounded poll guards against hangs.
WAIT_SECS="${OSCIL_TEST_WAIT_SECS:-120}"
elapsed=0
while [[ ! -s "${RESULTS_FILE}" && ${elapsed} -lt ${WAIT_SECS} ]]; do
  sleep 1
  elapsed=$((elapsed + 1))
done

if [[ ! -s "${RESULTS_FILE}" ]]; then
  fail "No results written to ${RESULTS_FILE} within ${WAIT_SECS}s. Reaper may not have run the driver — check Reaper's ReaScript console."
fi

log "Results:"
cat "${RESULTS_FILE}"
echo

# --- Parse pass/fail. Results file schema: {"summary":{"pass":N,"fail":N},"tests":[...]}
# Prefer python3 for JSON parsing; fall back to grep if absent (degraded).
if command -v python3 >/dev/null 2>&1; then
  python3 - "${RESULTS_FILE}" <<'PY'
import json, sys
with open(sys.argv[1]) as f:
    data = json.load(f)
summary = data.get("summary", {})
tests   = data.get("tests", [])
passed  = int(summary.get("pass", 0))
failed  = int(summary.get("fail", 0))
for t in tests:
    status = t.get("status", "?")
    name   = t.get("name", "?")
    detail = t.get("detail", "")
    print(f"  [{status:4}] {name}  {detail}")
print(f"\n{passed} passed, {failed} failed")
sys.exit(0 if failed == 0 else 1)
PY
  exit $?
else
  log "python3 not available; falling back to naive grep parse."
  if grep -q '"status":"fail"' "${RESULTS_FILE}"; then
    fail "At least one scenario failed. See ${RESULTS_FILE}."
  fi
  log "All scenarios passed (grep fallback)."
fi
