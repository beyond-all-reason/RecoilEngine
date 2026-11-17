-- Takes engine screenshots at fixed times, so a run can prove what was on screen.
--
-- A gadget rather than a widget, because the measurement that most needs proving
-- is the one taken after "luaui disable", where no widget survives to call the
-- action. LuaRules gadgets are untouched by that, so this keeps firing.
--
-- It replaced a synthetic Ctrl+Alt+S through System Events, which produced no
-- files at all. That route depended on an accessibility grant, the keyboard
-- layout, and macOS not claiming the combination, none of which the log could
-- confirm. This depends on nothing outside the engine.
--
-- install-probe.sh --shots <seconds>[,<seconds>...] puts it in and sets the times.

function gadget:GetInfo()
	return {
		name    = "Shot probe",
		desc    = "Takes engine screenshots at fixed times after load",
		author  = "",
		date    = "",
		license = "GPL v2 or later",
		layer   = 1000,
		enabled = true,
	}
end

if gadgetHandler:IsSyncedCode() then
	return
end

-- Seconds after this gadget loads. install-probe.sh rewrites the line and reads
-- it back, so a run cannot silently take no pictures.
local SHOT_TIMES = {}

local started = nil
local taken = 0

function gadget:Initialize()
	if #SHOT_TIMES == 0 then
		gadgetHandler:RemoveGadget(self)
		return
	end

	started = Spring.GetTimer()
	Spring.Echo(string.format("[shots] %d scheduled, first at %ds", #SHOT_TIMES, SHOT_TIMES[1]))
end

function gadget:Update()
	if taken >= #SHOT_TIMES then
		return
	end

	if Spring.DiffTimers(Spring.GetTimer(), started) < SHOT_TIMES[taken + 1] then
		return
	end

	taken = taken + 1

	-- Echoed rather than assumed. TakeScreenshot writes screenshots/screen_<time>.png
	-- and logs nothing itself, so without this line a missing file cannot be told
	-- apart from a shot that was never requested.
	local units = Spring.GetAllUnits() or {}
	Spring.Echo(string.format("[shots] taking %d of %d at %ds, units=%d", taken, #SHOT_TIMES, SHOT_TIMES[taken], #units))
	Spring.SendCommands("screenshot")
end
