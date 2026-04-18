-- run_all.lua
-- Driver that Reaper auto-executes (passed as trailing arg to REAPER binary).
-- Responsibilities:
--   1. Put lib/ on package.path so scenarios can require("reaper_test_lib").
--   2. Discover every scenarios/*.lua.
--   3. Execute each (respecting OSCIL_TEST_SCENARIO_FILTER).
--   4. Aggregate results; write JSON to OSCIL_TEST_RESULTS_FILE.
--   5. Tear down project state between scenarios.

-- ---------------------------------------------------------------------------
-- Environment
-- ---------------------------------------------------------------------------
local function getenv(name, default)
  local v = os.getenv(name)
  if v == nil or v == "" then return default end
  return v
end

local LIB_DIR       = getenv("OSCIL_TEST_LIB_DIR",       nil)
local SCENARIOS_DIR = getenv("OSCIL_TEST_SCENARIOS_DIR", nil)
local RESULTS_FILE  = getenv("OSCIL_TEST_RESULTS_FILE",  "/tmp/oscil_reaper_results.json")
local FILTER        = getenv("OSCIL_TEST_SCENARIO_FILTER", "")

-- Fallback: if env vars absent (e.g. running manually from Reaper actions),
-- infer from this script's location.
if not LIB_DIR or not SCENARIOS_DIR then
  -- reaper.get_action_context returns the path of the running script.
  local _, filename = reaper.get_action_context()
  local this_dir = filename:match("(.*/)")
  LIB_DIR       = LIB_DIR       or this_dir
  SCENARIOS_DIR = SCENARIOS_DIR or (this_dir .. "../scenarios")
end

-- Ensure Lua can find our helper module.
package.path = LIB_DIR .. "/?.lua;" .. package.path

local T = require("reaper_test_lib")
T.log("[run_all] starting")
T.log("[run_all] lib dir: " .. LIB_DIR)
T.log("[run_all] scenarios: " .. SCENARIOS_DIR)
T.log("[run_all] results file: " .. RESULTS_FILE)
if FILTER ~= "" then T.log("[run_all] filter: " .. FILTER) end

-- ---------------------------------------------------------------------------
-- Scenario discovery.
-- Reaper ships with EnumerateFiles for this purpose on all platforms.
-- ---------------------------------------------------------------------------
local scenarios = {}
local i = 0
while true do
  local name = reaper.EnumerateFiles(SCENARIOS_DIR, i)
  if not name then break end
  if name:sub(-4) == ".lua" then
    local stem = name:sub(1, -5) -- strip .lua
    if FILTER == "" or FILTER == stem then
      table.insert(scenarios, { stem = stem, path = SCENARIOS_DIR .. "/" .. name })
    end
  end
  i = i + 1
end

table.sort(scenarios, function(a, b) return a.stem < b.stem end)
T.log(string.format("[run_all] %d scenario(s) to run", #scenarios))

if #scenarios == 0 then
  if FILTER ~= "" then
    T.write_result(FILTER, "fail", "no scenario matches filter: " .. FILTER)
  else
    -- Not an error — empty suite is valid during initial scaffolding.
    T.log("[run_all] no scenarios found (scaffolding stage)")
  end
end

-- ---------------------------------------------------------------------------
-- Execute scenarios. Each scenario file must `return { name = ..., run = fn }`.
-- We dofile() the scenario, then pcall the run fn so a Lua error in one
-- scenario does not abort the suite.
-- ---------------------------------------------------------------------------
for _, sc in ipairs(scenarios) do
  T.set_current_test(sc.stem)
  T.log(string.format("[run_all] running %s", sc.stem))

  local load_ok, mod_or_err = pcall(dofile, sc.path)
  if not load_ok then
    T.write_result(sc.stem, "fail", "load error: " .. tostring(mod_or_err))
  elseif type(mod_or_err) ~= "table" or type(mod_or_err.run) ~= "function" then
    T.write_result(sc.stem, "fail", "scenario did not return {name, run}")
  else
    local run_ok, err = pcall(mod_or_err.run)
    if run_ok then
      T.write_result(mod_or_err.name or sc.stem, "pass", "")
    else
      T.write_result(mod_or_err.name or sc.stem, "fail", tostring(err))
    end
  end

  -- Best-effort teardown between scenarios. Failure here is logged but does
  -- not mark the previous scenario failed (it already passed or failed).
  local td_ok, td_err = pcall(T.teardown)
  if not td_ok then
    T.log("[run_all] teardown error: " .. tostring(td_err))
  end
end

-- ---------------------------------------------------------------------------
-- Flush results and exit. We do NOT call reaper.Main_OnCommand(40004) ("File:
-- Quit Reaper") automatically — leaving Reaper open lets the engineer inspect
-- state after a failure. The shell script polls for the results file.
-- ---------------------------------------------------------------------------
T.flush_results(RESULTS_FILE)
T.log("[run_all] done")
