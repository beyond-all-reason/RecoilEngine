-- Freezes the scene and reports where the frame time goes.
--
-- Copy or symlink into the throwaway game's LuaUI/Widgets/. install-probe.sh
-- does that, and keeps the source in the repo so it is not lost with the copy.
--
-- Why a widget rather than an engine change: Spring.GetProfilerRecordNames and
-- Spring.GetProfilerTimeRecord already expose every SCOPED_TIMER to Lua, and
-- "debug 1 0" turns the profiler on with its on-screen overlay off, so the whole
-- per-timer breakdown is reachable without touching the engine.
--
-- Why it freezes the scene: a live sim draws something different every run, and
-- differences between runs on this platform have already been large enough to
-- swamp what was being measured. Pausing at a fixed sim frame and re-applying
-- one camera state every frame makes the drawn scene the same from that point on,
-- for the whole run and between runs.

function widget:GetInfo()
	return {
		name    = "Perf probe",
		desc    = "Freezes the scene at a fixed sim frame and logs the profiler breakdown",
		author  = "",
		date    = "",
		license = "GPL v2 or later",
		layer   = 1000,
		enabled = true,
	}
end

local LOCK_FRAME = 90   -- sim frame to freeze at, 3 seconds in
local INTERVAL   = 5    -- seconds between reports
local TOP_N      = 16   -- timers reported, by share of wall clock

-- Seconds after the freeze to issue "luaui disable", or 0 to never issue it.
-- install-probe.sh --luaui-off <seconds> rewrites this line in the installed copy
-- and then reads it back, because os.getenv is stripped from the Lua sandbox at
-- LuaLibs.cpp:57 so the widget cannot be told at run time.
--
-- The disable removes the probe along with every other widget, which is the point:
-- it splits the game's cost into its 167 widgets against its gadgets, where the
-- unit materials live. Nothing re-enables it, so a run holds one switch and the
-- cycles either side of it are the two sides of the comparison.
--
-- The freeze survives, because "pause" is engine state rather than widget state,
-- and nothing is left to move the camera. Frame rates keep coming from the engine
-- through SPRING_FPS_LOG, which does not go through Lua at all.
local DISABLE_AFTER = 0

-- 1 to send the starting unit to the middle of the map and have the camera
-- follow it, instead of pausing. install-probe.sh --move sets it.
--
-- This is for hunting artefacts, not for measuring. A frozen scene reads to 2.5%
-- between intervals and a live one varies by a factor of two, so a frame rate
-- taken in this mode means nothing. What it buys is a scene that a frozen one
-- cannot produce: a moving unit, command line and waypoint overlays, and line of
-- sight sweeping across new ground, which is where the engine's own world-space
-- drawing actually gets exercised.
--
-- It also removes a dependency on the faction. A locked camera frames whatever
-- the commander's start position happens to be, and different factions start
-- differently, so tracking is what makes two runs comparable by eye.
--
-- Tracking is engine state, like pause, so it survives "luaui disable" and the
-- unit keeps walking on its queued order after every widget is gone.
local MOVE_AND_TRACK = 0

local lockedCam  = nil
local frozenAt   = nil
local disabled   = false
local drawFrames = 0
local lastReport = nil

-- Forward declared rather than left as an implicit global, because some widget
-- handlers refuse writes to new globals.
local reportProfile

-- Every run has to prove what it measured. One sweep reported a 2.5x win that was
-- really an empty map with a faction picker over it, because disabling a gadget
-- had stopped units spawning, and nothing in the log said so. A frame rate with
-- units=0 beside it is obviously void rather than quietly wrong.
local function sceneSummary()
	local units = Spring.GetAllUnits() or {}
	local vsx, vsy = Spring.GetViewGeometry()

	return string.format("units=%d view=%dx%d", #units, vsx or 0, vsy or 0)
end

-- Ordered rather than paused. The unit walks to the middle of the map and the
-- camera follows it, which draws the move line, the waypoint marker and a moving
-- line of sight, none of which a frozen scene ever shows.
local function startMoveAndTrack(f)
	local units = Spring.GetAllUnits() or {}
	local unitID = units[1]

	if unitID == nil then
		Spring.Echo("[probe] no unit to move, falling back to a frozen scene")
		lockedCam = Spring.GetCameraState()
		Spring.SendCommands("pause 1")
		return
	end

	local x = Game.mapSizeX * 0.5
	local z = Game.mapSizeZ * 0.5
	local y = Spring.GetGroundHeight(x, z) or 0

	Spring.SelectUnitArray({ unitID })
	Spring.GiveOrderToUnit(unitID, CMD.MOVE, { x, y, z }, {})

	-- "on" with an explicit id rather than the bare toggle, so this does not
	-- depend on what happens to be selected and cannot turn tracking back off.
	Spring.SendCommands("track on " .. unitID)

	Spring.Echo(string.format("[probe] unit %d ordered to %d,%d and tracked at sim frame %d, %s",
		unitID, x, z, f, sceneSummary()))
end

function widget:Initialize()
	-- Profiler on, overlay off. The overlay is itself a per-frame draw cost and
	-- would be measuring the measurement.
	Spring.SendCommands("debug 1 0")
	lastReport = Spring.GetTimer()
	Spring.Echo(string.format("[probe] profiler enabled, at sim frame %d will %s",
		LOCK_FRAME, (MOVE_AND_TRACK > 0) and "move the unit and track it, frame rates are void in this mode" or "freeze the scene"))
end

function widget:GameFrame(f)
	if f ~= LOCK_FRAME or lockedCam ~= nil then
		return
	end

	frozenAt = Spring.GetTimer()

	if MOVE_AND_TRACK > 0 then
		startMoveAndTrack(f)
	else
		lockedCam = Spring.GetCameraState()
		Spring.SendCommands("pause 1")
		Spring.Echo(string.format("[probe] scene frozen at sim frame %d, %s", f, sceneSummary()))
	end

	if DISABLE_AFTER > 0 then
		Spring.Echo(string.format("[probe] luaui disable scheduled for %ds after the freeze", DISABLE_AFTER))
	else
		Spring.Echo("[probe] luaui disable not scheduled")
	end
end

-- Issued from Update rather than from a draw callback, because it unloads every
-- widget including this one and the widget list is being walked during a draw.
local function maybeDisableLuaUI()
	if disabled or DISABLE_AFTER <= 0 or frozenAt == nil then
		return
	end

	if Spring.DiffTimers(Spring.GetTimer(), frozenAt) < DISABLE_AFTER then
		return
	end

	disabled = true

	-- Everything worth recording has to be echoed first. Nothing below the
	-- SendCommands is guaranteed to run, and no widget survives to log the frames
	-- that follow. The unit count carries over because the sim is paused, so no
	-- unit can spawn or die after this point.
	Spring.Echo(string.format("[probe] issuing luaui disable, %s", sceneSummary()))
	Spring.SendCommands("luaui disable")
end

function widget:DrawScreen()
	drawFrames = drawFrames + 1
end

function widget:Update()
	-- Every frame, not once: this also holds the camera against edge scroll and a
	-- knocked mouse, either of which would change the scene mid-measurement.
	if lockedCam ~= nil then
		Spring.SetCameraState(lockedCam, 0)
	end

	local now = Spring.GetTimer()
	local dt = Spring.DiffTimers(now, lastReport)

	if dt < INTERVAL then
		return
	end

	Spring.Echo(string.format("[probe] fps=%.2f drawframes=%d over %.2fs %s", drawFrames / dt, drawFrames, dt, sceneSummary()))

	drawFrames = 0
	lastReport = now

	-- Guarded, because an error in a widget removes the widget, and removing this
	-- one would quietly unfreeze the scene mid-run. That already happened once.
	local ok, err = pcall(reportProfile)
	if not ok then
		Spring.Echo("[probe] profiler read failed, scene stays frozen: " .. tostring(err))
	end

	-- Last, so a full report always immediately precedes the switch and the
	-- LuaUI-on side of the run ends on a logged scene summary.
	maybeDisableLuaUI()
end

-- Spring.GetProfilerTimeRecord had two bugs: it read its optional second
-- argument after pushing its results, so index 2 held the pushed total and any
-- one-argument call errored, and the frameData branch rawset into a number
-- because no table was ever created. Both call forms are exercised here so a fix
-- is checked rather than assumed.
local checkedApi = false

local function checkApi(name)
	checkedApi = true

	local ok1, a = pcall(Spring.GetProfilerTimeRecord, name)
	Spring.Echo(string.format("[probe] api one arg: ok=%s first=%s", tostring(ok1), tostring(a)))

	local ok2, b = pcall(Spring.GetProfilerTimeRecord, name, false)
	Spring.Echo(string.format("[probe] api false: ok=%s first=%s", tostring(ok2), tostring(b)))

	local ok3, r1, r2, r3, r4, r5, frames = pcall(Spring.GetProfilerTimeRecord, name, true)
	local n = 0
	if ok3 and type(frames) == "table" then
		for _ in pairs(frames) do n = n + 1 end
	end
	Spring.Echo(string.format("[probe] api true: ok=%s type=%s entries=%d", tostring(ok3), type(frames), n))
end

reportProfile = function()
	local rows = {}
	for _, name in ipairs(Spring.GetProfilerRecordNames()) do
		-- The second argument has to be passed. GetProfilerTimeRecord pushes its
		-- five results before reading it with luaL_optboolean(L, 2), so on a
		-- one-argument call index 2 is the pushed total and the call errors with
		-- "boolean expected, got number". An engine bug, not a Lua one.
		local _, current, maxdt, pct = Spring.GetProfilerTimeRecord(name, false)
		rows[#rows + 1] = { name = name, pct = pct or 0, current = current or 0, maxdt = maxdt or 0 }
	end
	table.sort(rows, function(a, b) return a.pct > b.pct end)

	if not checkedApi and rows[1] ~= nil then
		checkApi(rows[1].name)
	end

	for i = 1, math.min(TOP_N, #rows) do
		local r = rows[i]
		Spring.Echo(string.format("[probe]   %5.1f%% max %6.2fms  %s", r.pct * 100, r.maxdt, r.name))
	end
end
