-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local done = false
local stage = 0
local draws = 0
local failed = false
local iconSamples
local rect = { 1000, 1000, 3000, 2000 }
local function near(a, b, tolerance)
	assert(math.abs(a - b) < (tolerance or 0.2), tostring(a) .. " != " .. tostring(b))
end
local function bounded(state)
	assert(state.px >= rect[1] and state.px <= rect[3], "camera x outside bounds: " .. state.px)
	assert(state.pz >= rect[2] and state.pz <= rect[4], "camera z outside bounds: " .. state.pz)
end
local function setRegion(r)
	rect = r
	assert(Spring.SetCameraBounds(unpack(r)))
	assert(Spring.SetMiniMapView(unpack(r)))
end
local function sample(x, y)
	local pixels = gl.ReadPixels(math.floor(x) - 5, math.floor(y) - 5, 11, 11)
	local sum = 0
	for _, column in ipairs(pixels) do
		for _, pixel in ipairs(column) do
			sum = sum + pixel[1] + pixel[2] + pixel[3]
		end
	end
	return sum / 121
end
local function background()
	local texture = gl.CreateTexture(64, 64, { fbo = true, min_filter = GL.NEAREST, mag_filter = GL.NEAREST })
	assert(texture)
	gl.RenderToTexture(texture, function()
		gl.Color(0, 1, 0, 1)
		gl.Rect(-1, -1, 1, 0)
		gl.Color(0, 0, 1, 1)
		gl.Rect(-1, 0, 1, 1)
	end)
	assert(Spring.SetMapShadingTexture("$minimap", texture))
	gl.Color(1, 1, 1, 1)
end
local function independentIcons()
	-- Caller-owned icon rendering must ignore the built-in minimap/world crop.
	local target =
		gl.CreateTexture(128, 128, { fbo = true, min_filter = GL.NEAREST, mag_filter = GL.NEAREST })
	local function render()
		local total = 0
		gl.RenderToTexture(target, function()
			gl.Clear(GL.COLOR_BUFFER_BIT, 0, 0, 0, 0)
			gl.MatrixMode(GL.PROJECTION)
			gl.LoadIdentity()
			gl.MatrixMode(GL.MODELVIEW)
			gl.LoadIdentity()
			gl.Translate(-1, 1, 0)
			gl.Scale(2, -2, 1)
			gl.DrawMiniMapIcons(0, 0, Game.mapSizeX, Game.mapSizeZ, 100)
			for _, column in ipairs(gl.ReadPixels(0, 0, 128, 128)) do
				for _, pixel in ipairs(column) do
					total = total + pixel[1] + pixel[2] + pixel[3]
				end
			end
		end)
		return total
	end
	assert(Spring.SetWorldDrawBounds(unpack(rect)))
	local before = render()
	Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_frontier"), true)
	local hidden = render()
	Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_frontier"), false)
	assert(Spring.SetWorldDrawBounds())
	gl.DeleteTexture(target)
	assert(before - hidden > 1, "caller-owned icons followed built-in minimap crop")
	Spring.Echo("VIEW_RECTANGLES_INDEPENDENT_ICONS_PASS")
end
local function cameras()
	Spring.SetConfigInt("FPSClampPos", 0)
	Spring.SetConfigInt("RotOverheadClampMap", 0)
	setRegion(rect)
	for mode = 0, 6 do
		Spring.SetCameraState(
			{
				mode = mode,
				px = -1000,
				py = 1000,
				pz = 10000,
				dx = 0,
				dy = -1,
				dz = -0.001,
				rx = 2.677,
				ry = 0,
				height = 100000,
				dist = 100000,
			},
			0
		)
		local state = Spring.GetCameraState()
		bounded(state)
		if mode == 1 then
			near(state.height, 2000 * 9.5 / 8 * Spring.GetConfigFloat("OverheadMaxHeightFactor", 1))
		end
		if mode == 2 then
			near(state.dist, 2000 * 1.333)
		end
		if mode == 5 then
			near(state.px, 2000)
			near(state.pz, 1500)
		end
		Spring.SetCameraTarget(10000, 96, -1000, 0)
		bounded(Spring.GetCameraState())
	end
	-- Invalid rectangles must not change the active view.
	assert(not Spring.SetCameraBounds(1000, 1000, 1000, 2000))
	assert(not Spring.SetCameraBounds(0 / 0, 0, 3000, 2000))
	Spring.SetCameraState({ mode = 2, px = 10000, pz = 10000 }, 0)
	bounded(Spring.GetCameraState())
	assert(Spring.SetCameraBounds())
	Spring.SetCameraState({ mode = 2, px = 4000, pz = 3000, dist = 100000 }, 0)
	local state = Spring.GetCameraState()
	near(state.px, 4000)
	near(state.pz, 3000)
	near(state.dist, Game.mapSizeX * 1.333)
	setRegion(rect)
	Spring.SetCameraState({ mode = 1, px = 2000, pz = 1500, height = 800 }, 0)
	Spring.Echo("VIEW_RECTANGLES_CAMERAS_PASS")
end
local function trace(rotation)
	Spring.SetMiniMapRotation(rotation * math.pi / 2)
	local x, y, w, h = Spring.GetMiniMapGeometry()
	local sx, sz = 0.25, 0.75
	local kind, pos = Spring.TraceScreenRay(x + w * sx, y + h * (1 - sz) - 1, true, true)
	assert(kind == "ground" and pos, "minimap ray missed")
	if rotation == 1 then
		sx, sz = 1 - sz, sx
	elseif rotation == 2 then
		sx, sz = 1 - sx, 1 - sz
	elseif rotation == 3 then
		sx, sz = sz, 1 - sx
	end
	near(pos[1], rect[1] + sx * (rect[3] - rect[1]), 12)
	near(pos[3], rect[2] + sz * (rect[4] - rect[2]), 12)
end
function DrawInMiniMap()
	-- A deliberately oversized pixel-space chart must be clipped to the minimap.
	local x, y, w, h = Spring.GetMiniMapGeometry()
	gl.Color(1, 0, 1, 1)
	gl.Rect(-100, -100, 20, 20)
	gl.Color(1, 1, 1, 1)
end
function DrawScreen()
	if done or failed or Spring.GetGameFrame() < 2 then
		return
	end
	local ok, error = pcall(function()
		draws = draws + 1
		if stage == 0 then
			gl.SlaveMiniMap(true)
			gl.ConfigMiniMap(100, 100, 400, 200)
			Spring.SendCommands(
				"minimap min 0",
				"minimap drawpings 0",
				"minimap icons 1",
				"minimap unitsize 5"
			)
			background()
			cameras()
			Spring.SendCommands("minimap min 0")
			stage = 100
			draws = 0
			return
		end
		if stage >= 1 and stage <= 4 then
			Spring.SetMiniMapRotation((stage - 1) * math.pi / 2)
		end
		local bx, by, bw, bh = Spring.GetMiniMapGeometry()
		gl.Color(0.2, 0.1, 0.05, 1)
		gl.Rect(bx - 40, by, bx - 1, by + bh)
		gl.Color(1, 1, 1, 1)
		gl.DrawMiniMap(true)
		if draws < 8 then
			return
		end -- allow a cache refresh before pixel assertions
		local mx, my, mw, mh = Spring.GetMiniMapGeometry()
		if stage == 100 then
			independentIcons()
			iconSamples = { sample(mx + mw * 0.25, my + mh * 0.75), sample(mx + 2, my + mh * 0.6) }
			Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_home"), true)
			Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_edge"), true)
			stage = 101
			draws = 0
		elseif stage == 101 then
			assert(
				math.abs(sample(mx + mw * 0.25, my + mh * 0.75) - iconSamples[1]) > 0.005,
				"in-region icon missing"
			)
			near(sample(mx + 2, my + mh * 0.6), iconSamples[2], 0.01) -- outside icon must not bleed over edge
			Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_home"), false)
			Spring.SetUnitNoMinimap(Spring.GetGameRulesParam("view_edge"), false)
			stage = 1
			draws = 0
		elseif stage <= 4 then
			trace(stage - 1)
			for _, v in ipairs({ 0.1, 0.9 }) do
				local u = 0.65
				local a, b = u, 1 - v
				if stage == 2 then
					a, b = v, u
				elseif stage == 3 then
					a, b = 1 - u, v
				elseif stage == 4 then
					a, b = 1 - v, 1 - u
				end
				local r, g, blue = gl.ReadPixels(mx + mw * a, my + mh * b, 1, 1)
				assert(g > 0.2 and g > r + 0.1 and g > blue + 0.1, "background not cropped to home")
			end
			local x, y, w, h = Spring.GetMiniMapGeometry()
			local r, g, b = gl.ReadPixels(x + 5, y + 5, 1, 1)
			assert(r > 0.9 and b > 0.9 and g < 0.1, "chart missing from minimap")
			local outsideR, outsideG, outsideB = gl.ReadPixels(x - 10, y + 5, 1, 1)
			near(outsideR, 0.2, 0.03)
			near(outsideG, 0.1, 0.03)
			near(outsideB, 0.05, 0.03)
			stage = stage + 1
			draws = 0
		elseif stage == 5 then
			setRegion({ 1000, 2500, 3000, 3500 })
			Spring.SetCameraState({ mode = 5 }, 0)
			near(Spring.GetCameraState().pz, 3000)
			Spring.SendCommands("minimap min 0")
			stage = 6
			draws = 0
		elseif stage == 6 then
			trace(0)
			for _, v in ipairs({ 0.1, 0.9 }) do
				local r, g, b = gl.ReadPixels(mx + mw * 0.65, my + mh * (1 - v), 1, 1)
				assert(b > 0.2 and b > r + 0.1 and b > g + 0.1, "background did not switch to frontier")
			end
			assert(Spring.SetMiniMapView())
			assert(Spring.SetCameraBounds())
			rect = { 0, 0, Game.mapSizeX, Game.mapSizeZ }
			stage = 7
			draws = 0
		elseif stage == 7 then
			trace(0)
			setRegion({ 1000, 1000, 3000, 2000 })
			Spring.SetCameraState({ mode = 1, px = 1100, pz = 1100, height = 500 }, 0)
			Spring.SendCommands("moveup", "movereset")
			stage = 8
			draws = 0
		elseif stage == 8 then
			local state = Spring.GetCameraState()
			near(state.px, 2000)
			near(state.pz, 1500)
			near(state.height, 2000 * 9.5 / 8 * Spring.GetConfigFloat("OverheadMaxHeightFactor", 1))
			Spring.SetCameraState({ mode = 2, px = 1100, pz = 1100, dist = 500 }, 0)
			stage = 9
			draws = 0
		else
			local state = Spring.GetCameraState()
			near(state.px, 2000)
			near(state.pz, 1500)
			near(state.dist, 2000 * 1.333)
			Spring.Echo("VIEW_RECTANGLES_PASS")
			done = true
			Spring.SendCommands("quitforce")
		end
	end)
	if not ok then
		failed = true
		Spring.Echo("VIEW_RECTANGLES_FAIL " .. tostring(error))
		Spring.SendCommands("quitforce")
	end
end
