-- 01_vst3_save_reopen.lua
-- Task 3.2 — Oscil VST3 save/reopen round-trip.
--
-- Validates: plugin state survives a full project save -> close -> reopen cycle
-- when hosted as VST3. Primary defense against wrapper-specific state bugs that
-- the in-process harness cannot catch (gap #14 in harness_capability_gaps.md).
--
-- Assumptions (cannot be verified without running Reaper):
--   * Reaper has already scanned oscil4.vst3 (see README prerequisites).
--   * TrackFX_AddByName accepts the "VST3:oscil4" prefix form.
--   * Main_SaveProjectEx writes synchronously on return (we verify via io.open).
--   * dump_plugin_state returns a non-empty string containing "oscil4" — both
--     the FX-scoped chunk and the track state chunk contain the plugin name.
--
-- If any of those assumptions fails at runtime, the assertions below will
-- raise a clear error and the scenario is recorded as failed.

local T = require("reaper_test_lib")

local SAVE_PATH = "/tmp/oscil_reaper_scenario_01.rpp"

local function run()
  T.new_project()

  -- Load oscil4 VST3 on track 0 (0-indexed).
  local fx = T.load_vst3("oscil4", 0)
  T.assert_true(fx >= 0, "VST3 oscil4 load returned negative fx index")
  T.assert_eq(T.count_fx(0), 1, "expected exactly 1 FX on track 0 after load")

  -- Exercise processBlock briefly. Reaper's transport will call prepareToPlay
  -- + processBlock on all active FX during playback even without a source clip;
  -- we just need a non-zero amount of real-time engagement before save.
  T.play_seconds(3.0)

  -- Capture state BEFORE save. Use length as a stable proxy for equality —
  -- byte-equality can fail across save cycles because Reaper may rewrap the
  -- chunk's line endings or re-encode base64 padding. Length + plugin-name
  -- presence is the strongest version-portable check.
  local before = T.dump_plugin_state(0, 0)
  T.assert_true(#before > 0, "pre-save plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(before, "oscil4"),
    "pre-save chunk does not mention oscil4")
  local before_len = #before

  -- Remove stale save file if any (best-effort; ignore errors).
  pcall(os.remove, SAVE_PATH)

  T.save_project(SAVE_PATH)
  T.close_project()

  T.open_project(SAVE_PATH)

  -- After reopen the track layout must be identical.
  T.assert_true(reaper.CountTracks(0) >= 1, "no tracks after reopen")
  T.assert_eq(T.count_fx(0), 1, "expected 1 FX on track 0 after reopen")
  local name = T.fx_name(0, 0)
  T.assert_true(name:find("oscil4", 1, true) ~= nil,
    "FX at track 0 slot 0 is not oscil4 after reopen (got '" .. name .. "')")

  local after = T.dump_plugin_state(0, 0)
  T.assert_true(#after > 0, "post-reopen plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(after, "oscil4"),
    "post-reopen chunk does not mention oscil4")

  -- Length stability check. Allow a small margin for chunk re-encoding.
  local delta = math.abs(#after - before_len)
  T.assert_true(delta <= 64,
    string.format("state length drifted by %d bytes across save/reopen (before=%d, after=%d)",
      delta, before_len, #after))
end

return { name = "vst3_save_reopen", run = run }
