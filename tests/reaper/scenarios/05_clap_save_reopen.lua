-- 05_clap_save_reopen.lua
-- Task 3.6 — Oscil CLAP save/reopen round-trip.
--
-- Validates: CLAP wrapper persistence works end-to-end in a real DAW. Mirrors
-- scenario 01 but exercises the CLAP code path, added to Reaper in 7.x.
--
-- Assumptions (cannot be verified without running Reaper):
--   * Reaper was built with CLAP support (Reaper >= 7.0).
--   * The CLAP prefix used by `TrackFX_AddByName` on the host's Reaper version
--     is "CLAP:". Older docs also mention "CLAPi:" for instruments and the raw
--     form "clap."; we attempt all three in order and record which one worked.
--     This is the only place in the suite where format prefix is probed at
--     runtime because CLAP support is the newest and most version-variable.
--   * oscil4 is registered as an effect-type CLAP plugin (not instrument).

local T = require("reaper_test_lib")

local SAVE_PATH = (os.getenv("TMPDIR") or os.getenv("TMP") or os.getenv("TEMP") or "/tmp")
  .. "/oscil_reaper_scenario_05.rpp"

-- Try multiple prefix forms. Returns (fx_index, prefix_used) on success, or
-- (-1, "") on total failure. load_clap in the lib uses "CLAP:" by default;
-- we duplicate the logic here so we can cycle through fallbacks.
local function try_add_clap(track_idx)
  local tr = reaper.GetTrack(0, track_idx)
  T.assert_true(tr ~= nil, "no track " .. tostring(track_idx))
  -- Keep CLAP-specific identifiers only. The generic "oscil4" probe can
  -- resolve to AU/VST3 on systems with multiple formats installed, which
  -- would let this scenario pass without ever touching the CLAP wrapper.
  local candidates = { "CLAP:oscil4", "CLAPi:oscil4", "clap.oscil4" }
  for _, name in ipairs(candidates) do
    local fx = reaper.TrackFX_AddByName(tr, name, false, -1)
    if fx >= 0 then
      return fx, name
    end
  end
  return -1, ""
end

local function run()
  T.new_project()

  -- Track 0 must exist for try_add_clap to call GetTrack on it.
  if reaper.CountTracks(0) == 0 then
    reaper.InsertTrackAtIndex(0, true)
  end

  local fx, used = try_add_clap(0)
  T.assert_true(fx >= 0,
    "CLAP oscil4 load failed for all prefix forms (CLAP:, CLAPi:, clap., bare). " ..
    "Ensure Reaper >=7.0 is installed and has scanned the CLAP build.")
  T.log("[05] CLAP loaded via prefix form: " .. used)

  T.assert_eq(T.count_fx(0), 1, "expected exactly 1 FX after CLAP load")

  T.play_seconds(3.0)

  local before = T.dump_plugin_state(0, 0)
  T.assert_true(#before > 0, "pre-save CLAP plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(before, "oscil4"),
    "pre-save CLAP chunk does not mention oscil4")
  local before_len = #before

  pcall(os.remove, SAVE_PATH)
  T.save_project(SAVE_PATH)
  T.close_project()
  T.open_project(SAVE_PATH)

  T.assert_true(reaper.CountTracks(0) >= 1, "no tracks after CLAP reopen")
  T.assert_eq(T.count_fx(0), 1, "expected 1 FX on track 0 after CLAP reopen")
  local name = T.fx_name(0, 0)
  T.assert_true(name:find("oscil4", 1, true) ~= nil,
    "FX at track 0 slot 0 is not oscil4 after CLAP reopen (got '" .. name .. "')")

  local after = T.dump_plugin_state(0, 0)
  T.assert_true(#after > 0, "post-reopen CLAP plugin state chunk was empty")
  T.assert_true(T.state_mentions_plugin(after, "oscil4"),
    "post-reopen CLAP chunk does not mention oscil4")

  local delta = math.abs(#after - before_len)
  T.assert_true(delta <= 64,
    string.format("CLAP state length drifted by %d bytes (before=%d, after=%d)",
      delta, before_len, #after))
end

return { name = "clap_save_reopen", run = run }
