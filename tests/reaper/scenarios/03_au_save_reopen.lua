-- 03_au_save_reopen.lua
-- Task 3.4 — Oscil AU save/reopen round-trip (macOS only).
--
-- Validates: AU wrapper persistence works end-to-end in a real DAW. Mirrors
-- scenario 01 but exercises the Audio Unit code path, which differs from VST3
-- in state chunk format and in Logic's sandboxed-process model.
--
-- Platform gate: AU is macOS-only. On Windows/Linux this scenario returns
-- without raising, which the current runner (`tests/reaper/lib/run_all.lua`)
-- records as `pass` — there is no explicit skip status in the runner
-- contract today. The log line below leaves an audit trail so a pass on
-- a non-macOS runner is interpretable as "AU path not exercised" rather
-- than "AU path verified".
--
-- Assumptions (cannot be verified without running Reaper):
--   * `reaper.GetOS()` returns a string starting with "OSX" on macOS per docs.
--   * The AU bundle name exposed to Reaper is "oscil4" (matches VST3 name).
--   * `TrackFX_AddByName` with prefix "AU:oscil4" is the correct form on mac.

local T = require("reaper_test_lib")

local SAVE_PATH = (os.getenv("TMPDIR") or os.getenv("TMP") or os.getenv("TEMP") or "/tmp")
  .. "/oscil_reaper_scenario_03.rpp"

local function is_macos()
  local os_name = reaper.GetOS() or ""
  -- Reaper documents values: "Win32", "Win64", "OSX32", "OSX64", "macOS-arm64",
  -- "Other". Match any of the mac flavors.
  return os_name:match("OSX") ~= nil or os_name:match("[Mm]ac") ~= nil
end

local function run()
  if not is_macos() then
    T.log("[03] no-op: platform is not macOS (GetOS=" .. tostring(reaper.GetOS())
          .. "); AU path not exercised on this runner")
    -- Return without raising. The runner records this as `pass` (see header
    -- comment); the log prefix above distinguishes a platform-skip from a
    -- real AU verification pass when scanning scenario output.
    return
  end

  T.new_project()

  local fx = T.load_au("oscil4", 0)
  T.assert_true(fx >= 0, "AU oscil4 load returned negative fx index")
  T.assert_eq(T.count_fx(0), 1, "expected exactly 1 FX on track 0 after AU load")

  T.play_seconds(3.0)

  local before = T.dump_plugin_state(0, 0)
  T.assert_true(#before > 0, "pre-save AU plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(before, "oscil4"),
    "pre-save AU chunk does not mention oscil4")
  local before_len = #before

  pcall(os.remove, SAVE_PATH)
  T.save_project(SAVE_PATH)
  T.close_project()
  T.open_project(SAVE_PATH)

  T.assert_true(reaper.CountTracks(0) >= 1, "no tracks after AU reopen")
  T.assert_eq(T.count_fx(0), 1, "expected 1 FX on track 0 after AU reopen")
  local name = T.fx_name(0, 0)
  T.assert_true(name:find("oscil4", 1, true) ~= nil,
    "FX at track 0 slot 0 is not oscil4 after AU reopen (got '" .. name .. "')")

  local after = T.dump_plugin_state(0, 0)
  T.assert_true(#after > 0, "post-reopen AU plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(after, "oscil4"),
    "post-reopen AU chunk does not mention oscil4")

  local delta = math.abs(#after - before_len)
  T.assert_true(delta <= 64,
    string.format("AU state length drifted by %d bytes (before=%d, after=%d)",
      delta, before_len, #after))
end

return { name = "au_save_reopen", run = run }
