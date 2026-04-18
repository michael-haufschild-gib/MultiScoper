-- reaper_test_lib.lua
-- Shared helpers for Oscil Reaper integration scenarios.
-- Scenarios require() this module; the driver (run_all.lua) loads it first
-- and places its directory on package.path.

local M = {}

-- ---------------------------------------------------------------------------
-- Internal state
-- ---------------------------------------------------------------------------
local results = {}          -- list of { name, status, detail }
local current_test = nil    -- name of the test currently executing

local function log(msg)
  -- Surfaces in the Reaper ReaScript console, which is the user-visible log.
  if reaper and reaper.ShowConsoleMsg then
    reaper.ShowConsoleMsg(tostring(msg) .. "\n")
  else
    io.stderr:write(tostring(msg) .. "\n")
  end
end
M.log = log

-- ---------------------------------------------------------------------------
-- Assertions
-- ---------------------------------------------------------------------------
-- Raises a Lua error if the condition is false; run_all.lua catches this in
-- pcall and records the failing test. Also logs to console for live feedback.
function M.assert_true(cond, msg)
  if not cond then
    local detail = msg or "assert_true failed"
    log(string.format("[FAIL] %s: %s", current_test or "?", detail))
    error(detail, 2)
  end
end

function M.assert_eq(actual, expected, msg)
  if actual ~= expected then
    local detail = string.format("%s: expected %s, got %s",
      msg or "assert_eq", tostring(expected), tostring(actual))
    log(string.format("[FAIL] %s: %s", current_test or "?", detail))
    error(detail, 2)
  end
end

-- ---------------------------------------------------------------------------
-- Plugin loading
-- ReaScript API: reaper.TrackFX_AddByName(track, fxname, recFX, instantiate)
--   instantiate < 0 => always create a new instance (returns new fx index)
-- Track retrieval: reaper.GetTrack(proj, trackidx) — 0-indexed.
-- If the track doesn't exist we insert it first.
-- ---------------------------------------------------------------------------
local function ensure_track(track_idx)
  local have = reaper.CountTracks(0)
  while have <= track_idx do
    -- InsertTrackAtIndex(idx, wantDefaults) appends a new track.
    reaper.InsertTrackAtIndex(have, true)
    have = reaper.CountTracks(0)
  end
  local tr = reaper.GetTrack(0, track_idx)
  M.assert_true(tr ~= nil, "failed to get/create track " .. tostring(track_idx))
  return tr
end

-- Reaper uses prefix conventions in TrackFX_AddByName to disambiguate formats:
--   "VST3:<name>" forces VST3, "AU:<name>" forces AU, "CLAP:<name>" forces CLAP.
-- plugin_name here is the bare name (e.g. "oscil4"); prefix is added per fn.
local function add_fx(track_idx, prefixed_name)
  local tr = ensure_track(track_idx)
  -- instantiate = -1 => always instantiate. Returns fx index or -1 on failure.
  local fx = reaper.TrackFX_AddByName(tr, prefixed_name, false, -1)
  M.assert_true(fx >= 0, "failed to load plugin: " .. prefixed_name)
  return fx
end

function M.load_vst3(plugin_name, track_idx)
  return add_fx(track_idx, "VST3:" .. plugin_name)
end

function M.load_au(plugin_name, track_idx)
  return add_fx(track_idx, "AU:" .. plugin_name)
end

function M.load_clap(plugin_name, track_idx)
  -- TODO: confirm CLAP prefix form supported by the installed Reaper build.
  -- Reaper added CLAP support in 7.x; prefix "CLAP:" is the current convention.
  return add_fx(track_idx, "CLAP:" .. plugin_name)
end

-- ---------------------------------------------------------------------------
-- Transport / project
-- ---------------------------------------------------------------------------
-- Play and busy-wait `seconds` seconds of wall time. reaper.defer is cooperative
-- and would require a re-entry pattern we don't need for short blocking waits;
-- os.clock gives us a simple bounded sleep that still allows audio to flow.
function M.play_seconds(seconds)
  reaper.OnPlayButton()
  local target = os.clock() + seconds
  while os.clock() < target do
    -- no-op; Reaper's audio thread runs independently of this script thread.
  end
  reaper.OnStopButton()
end

function M.save_project(path)
  -- reaper.Main_SaveProjectEx(proj, filename, options). options=0 => default save.
  -- Per ReaScript docs Main_SaveProjectEx returns nothing reliably across
  -- versions (void on some, bool on others). Strategy: call it, then verify
  -- the file exists on disk. That's the only portable truth.
  pcall(function() reaper.Main_SaveProjectEx(0, path, 0) end)
  local f = io.open(path, "rb")
  if not f then
    error("save_project: file not present after save: " .. tostring(path))
  end
  f:close()
end

function M.close_project()
  -- Main_OnCommand 40860 = "File: Close project" (confirmed stable command id).
  reaper.Main_OnCommand(40860, 0)
end

-- Creates a new empty project tab and switches to it.
-- Main_OnCommand 40859 = "File: New project tab".
function M.new_project()
  reaper.Main_OnCommand(40859, 0)
end

-- Returns the number of FX on the given track (0-indexed).
function M.count_fx(track_idx)
  local tr = reaper.GetTrack(0, track_idx)
  if tr == nil then return 0 end
  return reaper.TrackFX_GetCount(tr)
end

-- Returns the displayed name of the FX at (track_idx, plugin_idx).
function M.fx_name(track_idx, plugin_idx)
  local tr = reaper.GetTrack(0, track_idx)
  if tr == nil then return "" end
  local _, name = reaper.TrackFX_GetFXName(tr, plugin_idx, "")
  return name or ""
end

function M.open_project(path)
  -- Main_openProject(filename) opens in current project slot.
  reaper.Main_openProject(path)
end

-- ---------------------------------------------------------------------------
-- Plugin state dump
-- TrackFX_GetNamedConfigParm / TrackFX_GetStateChunk differ by Reaper version
-- and some flavors need a large preallocated buffer. We prefer the chunk form.
-- ---------------------------------------------------------------------------
function M.dump_plugin_state(track_idx, plugin_idx)
  local tr = reaper.GetTrack(0, track_idx)
  M.assert_true(tr ~= nil, "no track " .. tostring(track_idx))
  -- Two candidate APIs; their returns differ by Reaper version.
  --  * TrackFX_GetNamedConfigParm(tr, fx, "vst_chunk") -> bool, string  (Reaper 6.11+)
  --  * GetTrackStateChunk(tr, "", false)               -> bool, string  (always)
  -- Prefer the FX-scoped call; fall back to the full track chunk otherwise.
  local ok, chunk = reaper.TrackFX_GetNamedConfigParm(tr, plugin_idx, "vst_chunk")
  if ok and type(chunk) == "string" and #chunk > 0 then
    return chunk
  end
  local got, trchunk = reaper.GetTrackStateChunk(tr, "", false)
  M.assert_true(got and type(trchunk) == "string",
    "dump_plugin_state: no chunk retrievable for track " .. tostring(track_idx))
  return trchunk
end

-- Returns true if the given state chunk contains a recognizable oscil4 FX block.
-- Covers both VST3, AU, and CLAP wrappers — we just check the name.
function M.state_mentions_plugin(chunk, plugin_name)
  if type(chunk) ~= "string" then return false end
  return chunk:find(plugin_name, 1, true) ~= nil
end

-- ---------------------------------------------------------------------------
-- Results file writer. JSON is hand-rolled to avoid a dependency.
-- Schema: { summary: {pass,fail,total}, tests: [ {name,status,detail}, ... ] }
-- ---------------------------------------------------------------------------
local function json_escape(s)
  s = tostring(s)
  s = s:gsub("\\", "\\\\"):gsub("\"", "\\\""):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
  return s
end

function M.set_current_test(name)
  current_test = name
end

function M.write_result(test_name, status, detail)
  table.insert(results, {
    name   = test_name,
    status = status,
    detail = detail or "",
  })
end

function M.flush_results(path)
  local pass, fail = 0, 0
  for _, r in ipairs(results) do
    if r.status == "pass" then pass = pass + 1
    elseif r.status == "fail" then fail = fail + 1 end
  end
  local parts = {}
  table.insert(parts, string.format(
    '{"summary":{"pass":%d,"fail":%d,"total":%d},"tests":[',
    pass, fail, pass + fail))
  for i, r in ipairs(results) do
    if i > 1 then table.insert(parts, ",") end
    table.insert(parts, string.format(
      '{"name":"%s","status":"%s","detail":"%s"}',
      json_escape(r.name), json_escape(r.status), json_escape(r.detail)))
  end
  table.insert(parts, "]}")
  local json = table.concat(parts)

  local f, err = io.open(path, "w")
  if not f then
    log("[reaper_test_lib] failed to open results file: " .. tostring(err))
    return false
  end
  f:write(json)
  f:close()
  log(string.format("[reaper_test_lib] wrote results: %d pass, %d fail -> %s", pass, fail, path))
  return true
end

-- ---------------------------------------------------------------------------
-- Teardown: close any open project silently. Safe to call repeatedly.
-- ---------------------------------------------------------------------------
function M.teardown()
  -- Determinism strategy: mark the current project NOT dirty so closing it does
  -- not prompt, then close it. This avoids waiting on a modal save dialog.
  -- reaper.IsProjectDirty(0) returns 1 when dirty. MarkProjectDirty exists but
  -- there is no "mark clean" API; instead we set the undo state by issuing
  -- Main_OnCommand 40026 (File: Save project) only if the caller saved — we
  -- do not do that here. Safer: use reaper.Main_OnCommand(40860, 0) and rely
  -- on the individual scenarios to either save_project() first or accept that
  -- close will prompt. To guard against prompts stalling the suite, we first
  -- open a new empty tab (40859) which makes "close project" operate on a
  -- clean tab.
  pcall(function()
    reaper.Main_OnCommand(40859, 0) -- File: New project tab (ignore save prompt on prior tab)
    reaper.Main_OnCommand(40860, 0) -- File: Close project (closes the newly opened tab harmlessly)
  end)
end

return M
