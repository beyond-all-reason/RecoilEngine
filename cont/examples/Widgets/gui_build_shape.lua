--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_build_shape.lua
--  brief:   Example widget demonstrating the GetBuildShape callin
--  author:  RecoilEngine contributors
--
--  Demonstrates:
--    - Using the GetBuildShape callin to pick the shape a build-drag traces out
--
--  The engine asks GetBuildShape while a building placement is active. Return
--  one of "single", "straightline", "diagonal", "flood", "hollowbox" or
--  "surround" to select the shape, or nil to keep the engine's default
--  modifier-key behaviour. "surround" rings the existing building (or queued
--  build order) under the cursor, degrading to "straightline" without one.
--
--  This example reproduces the engine defaults, but with the queue-key (shift)
--  requirement dropped: dragging always paints buildings.
--
--  License: GNU GPL, v2 or later
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "Build Shape Example",
		desc = "Selects the build-drag shape via the GetBuildShape callin",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

function widget:GetBuildShape(unitDefID, facing, startX, startY, startZ, endX, endY, endZ)
	local alt, ctrl, meta, shift = Spring.GetModKeyState()

	if ctrl and alt then
		return "hollowbox"
	end

	if alt then
		return "flood"
	end

	if ctrl then
		-- rings the building under the cursor, or a straight line without one
		return "surround"
	end

	return "diagonal"
end
