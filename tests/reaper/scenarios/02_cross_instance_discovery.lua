-- 02_cross_instance_discovery.lua
-- Task 3.3 — Cross-instance source discovery (HEADLINE FEATURE).
--
-- Validates: when two Oscil instances are loaded in the same Reaper project,
-- the InstanceRegistry inside each plugin sees the other as a known source.
-- This is the single most important scenario in the suite — it exercises
-- Oscil's defining feature (aggregator discovery) inside a real host process,
-- which the in-process harness fakes via a shared-process singleton (see
-- gap #7 in harness_capability_gaps.md).
--
-- Assumptions (cannot be verified without running Reaper):
--   * `reaper.TrackFX_Show(track, fx_idx, showFlag)` with showFlag=3 opens the
--     floating plugin window. This is the documented ReaScript signature.
--   * Cross-instance discovery happens on the editor timer path — showing the
--     editor on at least one instance forces the discovery code to run.
--   * The serialized plugin state chunk includes either a `sources` or
--     `knownSources` element listing discovered peers. This is the contract
--     of PluginProcessorState's OscilState serialization.
--
-- Fallback strategy: if the primary chunk-scan check doesn't find 2 sources,
-- the scenario degrades to a save-reopen round-trip and checks that BOTH
-- plugin chunks mention oscil4 (weaker, but still proves both loaded).

local T = require("reaper_test_lib")

local SAVE_PATH = "/tmp/oscil_reaper_scenario_02.rpp"

-- Count occurrences of `needle` in `haystack` as a literal substring.
local function count_occurrences(haystack, needle)
  if type(haystack) ~= "string" or type(needle) ~= "string" or #needle == 0 then
    return 0
  end
  local n, start = 0, 1
  while true do
    local s = haystack:find(needle, start, true)
    if not s then break end
    n = n + 1
    start = s + #needle
  end
  return n
end

local function run()
  T.new_project()

  -- Two tracks, each with oscil4 VST3.
  local fx0 = T.load_vst3("oscil4", 0)
  local fx1 = T.load_vst3("oscil4", 1)
  T.assert_true(fx0 >= 0 and fx1 >= 0, "both oscil4 instances must load")
  T.assert_eq(T.count_fx(0), 1, "track 0 should have 1 FX")
  T.assert_eq(T.count_fx(1), 1, "track 1 should have 1 FX")

  -- Show the editor on track 0's plugin. showFlag=3 => show floating window.
  -- TODO(verify-reaper-api): showFlag semantics documented in ReaScript as
  -- 0=hide, 1=show chain, 2=hide floating, 3=show floating.
  local tr0 = reaper.GetTrack(0, 0)
  reaper.TrackFX_Show(tr0, 0, 3)

  -- Give the editor timer ~2s to run at least one discovery tick. The
  -- registry publishes peer info on the message thread; the editor's
  -- periodic timer copies it into the serialized state.
  T.play_seconds(2.0)

  -- Primary check: dump track 0's plugin state and look for evidence of 2
  -- distinct source entries. The OscilState XML contains per-source ids,
  -- so counting the "<SOURCE" or "sourceId" marker is the most robust
  -- approach. We fall back to counting "oscil4" mentions if neither marker
  -- is present — which would indicate the state format changed and the
  -- scenario needs updating.
  local chunk = T.dump_plugin_state(0, 0)
  T.assert_true(#chunk > 0, "track 0 plugin state was empty")

  local src_markers = {
    "<SOURCE",     -- XML-style element
    "sourceId",    -- attribute name fallback
    "knownSource", -- legacy key
    "SOURCE_",     -- name=value style
  }

  local max_count = 0
  local used_marker = ""
  for _, m in ipairs(src_markers) do
    local c = count_occurrences(chunk, m)
    if c > max_count then
      max_count = c
      used_marker = m
    end
  end

  if max_count >= 2 then
    T.log(string.format("[02] primary check PASS: found %d occurrences of '%s'",
      max_count, used_marker))
    return -- happy path
  end

  -- Fallback: save+close+reopen and verify both plugins' chunks carry oscil4.
  -- This proves the two instances coexist and both persist state. It does not
  -- prove cross-instance discovery; we log this as a degraded outcome.
  T.log("[02] primary source-marker check did not find >=2 sources; running fallback")

  pcall(os.remove, SAVE_PATH)
  T.save_project(SAVE_PATH)
  T.close_project()
  T.open_project(SAVE_PATH)

  T.assert_true(reaper.CountTracks(0) >= 2, "fallback: expected >=2 tracks after reopen")
  T.assert_eq(T.count_fx(0), 1, "fallback: track 0 should still have 1 FX")
  T.assert_eq(T.count_fx(1), 1, "fallback: track 1 should still have 1 FX")

  local c0 = T.dump_plugin_state(0, 0)
  local c1 = T.dump_plugin_state(1, 0)
  T.assert_true(T.state_mentions_plugin(c0, "oscil4"),
    "fallback: track 0 chunk missing oscil4 marker")
  T.assert_true(T.state_mentions_plugin(c1, "oscil4"),
    "fallback: track 1 chunk missing oscil4 marker")

  -- Re-probe the source markers on the post-reopen chunks before declaring
  -- anything a pass. Merely proving that two oscil4 instances reopened does
  -- NOT prove cross-instance discovery — one-way or fully broken peer-list
  -- serialization would still look healthy by that weaker check. Require at
  -- least one of the two instances to surface a peer marker, and fail the
  -- scenario if neither does.
  local post_max_count = 0
  local post_used_marker = ""
  for _, chunk_to_check in ipairs({ c0, c1 }) do
    for _, m in ipairs(src_markers) do
      local c = count_occurrences(chunk_to_check, m)
      if c > post_max_count then
        post_max_count = c
        post_used_marker = m
      end
    end
  end

  if post_max_count < 2 then
    T.assert_true(false,
      "fallback: neither reopened chunk exposes >=2 source markers — " ..
      "cross-instance discovery cannot be proven. Verify " ..
      "InstanceRegistry serialization or update src_markers[] in this scenario.")
  end

  T.log(string.format(
    "[02] fallback PASS after reopen: found %d occurrences of '%s' across reopened chunks",
    post_max_count, post_used_marker))
end

return { name = "cross_instance_discovery", run = run }
