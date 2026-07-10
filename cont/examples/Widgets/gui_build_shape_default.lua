--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_build_shape_default.lua
--  brief:   GetBuildShape callin example reproducing the engine defaults
--  author:  RecoilEngine contributors
--
--  Demonstrates:
--    - Using the GetBuildShape callin to pick the shape a build-drag traces out
--
--  The engine asks GetBuildShape while a building placement is active. Return
--  one of "single", "cardinalline", "freeangleline", "flood", "hollowbox" or
--  "surround" to select the shape, or nil to keep the engine's default
--  modifier-key behaviour. "hollowbox" and "surround" both ring an existing
--  building (or queued build order) under the cursor; without such a target the
--  former stays a hollow box while the latter degrades to a cardinal line.
--
--  This example reproduces the engine's default modifier-key behaviour exactly,
--  so it can be used as a drop-in starting point for your own shape logic.
--  See gui_build_shape_showcase.lua for a taste of what else is possible.
--
--  License: GNU GPL, v2 or later
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "Build Shape: Engine Defaults",
		desc = "Reimplements the default build-drag shapes via the GetBuildShape callin",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

function widget:GetBuildShape(unitDefID, facing, startX, startY, startZ, endX, endY, endZ)
	local alt, ctrl, meta, shift = Spring.GetModKeyState()

	-- dragging out more than one building requires the queue key
	local queue = shift
	if Spring.GetConfigInt("InvertQueueKey", 0) ~= 0 then
		queue = not shift
	end

	if not queue then
		-- no queue key means no drag: always a single building, so none of the
		-- drag shapes below can be reached without it
		return "single"
	end

	-- a drag too small to fit a second building degrades to "single" engine-side,
	-- so the drag shapes below need no size checks of their own

	if ctrl then
		-- ring the building under the cursor; without one, ctrl+alt traces the
		-- dragged box's perimeter and plain ctrl an axis-locked cardinal line
		return alt and "hollowbox" or "surround"
	end

	if alt then
		-- fill the dragged box with buildings
		return "flood"
	end

	-- queue key alone: a free-angle line following the drag
	return "freeangleline"
end
