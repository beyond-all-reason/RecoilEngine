-- visreg capture widget (visreg.sh, from Adam Burgess's qant-au/tellurion). Installed into an ISOLATED write
-- dir only, never the player's. During demo playback it pauses at fixed SIM
-- frames, points a fixed camera at fixed map positions, hides the interface,
-- waits a few draw frames for the picture to settle, takes an engine
-- screenshot, then resumes. Shots are keyed to sim frames, not wall time, so
-- the same replay gives the same pictures on every run.
--
-- Frames come from the env-free config file LuaUI/visreg_frames.txt written by
-- visreg.sh: one "<frame> <name> <x-fraction> <z-fraction> <height>" per line.

function widget:GetInfo()
  return {
    name    = "visreg capture",
    desc    = "Visual regression screenshots at fixed sim frames (test only)",
    layer   = 1000000,
    enabled = true,
    handler = true,
  }
end

local shots = {}
local idx = 1
local state = "run"   -- run -> settle -> shoot -> resume
local settle = 0
local SETTLE_FRAMES = 30
local lastPos = nil
local still = 0

local function log(s) Spring.Echo("[visreg] " .. s) end

function widget:Initialize()
  -- the player's pointer may rest on the window edge: edge scrolling would then
  -- move the camera during a shot (isolated write dir, so nothing persists)
  Spring.SetConfigFloat("EdgeMoveWidth", 0)
  Spring.SetConfigInt("WindowedEdgeMove", 0)
  local text = VFS.LoadFile("LuaUI/visreg_frames.txt", VFS.RAW)
  if not text then log("FATAL no LuaUI/visreg_frames.txt"); return end
  for line in text:gmatch("[^\n]+") do
    local fr, name, xf, zf, h = line:match("^(%d+)%s+(%S+)%s+([%d%.]+)%s+([%d%.]+)%s+(%d+)")
    if fr then shots[#shots + 1] = { frame = tonumber(fr), name = name, xf = tonumber(xf), zf = tonumber(zf), h = tonumber(h) } end
  end
  table.sort(shots, function(a, b) return a.frame < b.frame end)
  log("loaded " .. #shots .. " shots")
end

local function aim(s)
  Spring.SetCameraState({
    name = "ta", mode = 1,
    px = Game.mapSizeX * s.xf, py = 0, pz = Game.mapSizeZ * s.zf,
    height = s.h, angle = 0.8, flipped = -1, zscale = 0.5,
  }, 0)
end

-- A paused demo still plays the frames already queued, and at 20x that is
-- hundreds. Drop to 1x shortly before each shot so the pause lands within a
-- frame or two, and go back to full speed after it.
local SLOW_LEAD = 150
local slowed = false
local function speed(x)
  if x < 2 then
    Spring.SendCommands("setminspeed " .. x); Spring.SendCommands("setmaxspeed " .. x)
  else
    Spring.SendCommands("setmaxspeed " .. x); Spring.SendCommands("setminspeed " .. x)
  end
end

function widget:GameFrame(n)
  local s = shots[idx]
  if state == "run" and s and not slowed and n >= s.frame - SLOW_LEAD then
    speed(1); slowed = true
    log(string.format("slow at frame %d", n))
  end
  if state == "run" and s and n >= s.frame then
    Spring.SendCommands("pause 1")
    Spring.SendCommands("hideinterface 1")
    aim(s)
    state, settle = "settle", 0
    log(string.format("pause at frame %d for %s", n, s.name))
  end
end

function widget:Update()
  local s = shots[idx]
  if state == "settle" then
    -- wait until the camera has stopped moving (BAR smooths camera moves) for
    -- 20 frames in a row, at least SETTLE_FRAMES, at most 600
    aim(s)
    settle = settle + 1
    local x, y, z = Spring.GetCameraPosition()
    if lastPos and math.abs(x - lastPos[1]) + math.abs(y - lastPos[2]) + math.abs(z - lastPos[3]) < 0.01 then
      still = still + 1
    else
      still = 0
    end
    lastPos = { x, y, z }
    if (settle >= SETTLE_FRAMES and still >= 20) or settle >= 600 then
      if settle >= 600 then log("camera never settled for " .. s.name) end
      state, lastPos, still = "shoot", nil, 0
    end
  elseif state == "shoot" then
    Spring.SendCommands("screenshot png")
    log(string.format("SHOT %s frame %d", s.name, Spring.GetGameFrame()))
    state = "resume"
  elseif state == "resume" then
    idx = idx + 1
    Spring.SendCommands("hideinterface 0")
    if idx > #shots then
      log("DONE")
      Spring.SendCommands("quitforce")
      return
    end
    Spring.SendCommands("pause 0")
    slowed = false
    if shots[idx].frame - SLOW_LEAD > Spring.GetGameFrame() then speed(20) end
    state = "run"
  end
end
