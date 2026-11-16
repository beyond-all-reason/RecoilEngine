-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function widget:GetInfo()
	return { name = "Ghost color validation", author = "BAR contributors", layer = 0, enabled = false }
end
local states = { "ghost", "ghostHighlight", "radarGhost", "radarGhostHighlight" }
local colors = {
	{ 0.1, 1, 0.1, 0.7 },
	{ 1, 0.1, 0.1, 0.8 },
	{ 0.1, 0.1, 1, 0.7 },
	{ 1, 0.1, 1, 0.8 },
}
local reset = false
function widget:Initialize()
	assert(Spring.SetGhostColor, "missing Spring.SetGhostColor")
	for _, state in ipairs(states) do
		assert(pcall(Spring.SetGhostColor, state, 0, 0, 0, 0))
		assert(pcall(Spring.SetGhostColor, state, 1, 1, 1, 1))
		assert(pcall(Spring.SetGhostColor, state))
		assert(pcall(Spring.SetGhostColor, state, nil))
	end
	assert(not pcall(Spring.SetGhostColor, "unknown", 1, 1, 1, 1))
	assert(not pcall(Spring.SetGhostColor, "ghost", 1, 1, 1))
	for component = 1, 4 do
		for _, invalid in ipairs({ -0.1, 1.1, math.huge, -math.huge, 0/0 }) do
			local color = { 1, 1, 1, 1 }
			color[component] = invalid
			assert(not pcall(Spring.SetGhostColor, "ghost", unpack(color)))
		end
	end
	for i, state in ipairs(states) do
		Spring.SetGhostColor(state, unpack(colors[i]))
	end
	Spring.Echo("GHOST_TEST PASS ghost color API validation")
end
function widget:Update()
	if Spring.GetGameFrame() >= 230 * 6 and not reset then
		self:Shutdown()
		reset = true
		Spring.Echo("GHOST_TEST PASS ghost colors reset")
	end
end
function widget:Shutdown()
	for _, state in ipairs(states) do
		Spring.SetGhostColor(state)
	end
end
