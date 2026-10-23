-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local phase = 0
local frames = 0
local done = false
local baseline = {}
local hidden = {}
local switched = {}
local samples = {}
local units = {}
local features = {}
local shotPositions = {}
local offmapHidden = {}
local function worldFlags(flag)
	return flag % 128
end -- exclude minimap/icon bit
local function near(a, b)
	assert(math.abs(a - b) < 0.02, tostring(a) .. " != " .. tostring(b))
end
local function pixels(x, y, z)
	local px, py, depth = Spring.WorldToScreenCoords(x, y, z)
	local w, h = Spring.GetViewGeometry()
	assert(
		depth > 0 and depth < 1 and px > 20 and px < w - 20 and py > 20 and py < h - 20,
		"test object outside tilted view: " .. x .. "," .. y .. "," .. z .. " -> " .. px .. "," .. py .. "," .. depth
	)
	local image = gl.ReadPixels(math.floor(px) - 16, math.floor(py) - 16, 33, 33)
	local result = {}
	for _, column in ipairs(image) do
		for _, pixel in ipairs(column) do
			for channel = 1, 3 do
				result[#result + 1] = pixel[channel]
			end
		end
	end
	return result
end
local function difference(a, b)
	local total = 0
	for i, value in ipairs(a) do
		total = total + math.abs(value - b[i])
	end
	return total / #a
end
local function capture()
	local w, h = Spring.GetViewGeometry()
	gl.SaveImage(0, 0, w, h, "draw-phase-" .. phase .. ".png")
	local result = {}
	for name, pos in pairs(samples) do
		result[name] = pixels(pos[1], pos[2], pos[3])
	end
	return result
end
local function flags(region, visible)
	local unitFlag = worldFlags(Spring.GetUnitDrawFlag(units[region]))
	local featureFlag = worldFlags(Spring.GetFeatureDrawFlag(features[region]))
	if visible then
		assert(unitFlag ~= 0 and featureFlag ~= 0, "in-region models disappeared: " .. region)
		assert(unitFlag % 32 >= 16 and featureFlag % 32 >= 16, "baseline shadow casters missing: " .. region)
	else
		assert(unitFlag == 0 and featureFlag == 0, "outside models/shadows have draw flags: " .. region)
	end
end
local function simulation()
	assert(#Spring.GetAllUnits() == 2 and #Spring.GetAllFeatures() == 2, "draw bounds changed object lifetimes")
	for id, pos in pairs(shotPositions) do
		local x, y, z = Spring.GetProjectilePosition(id)
		assert(x, "test projectile unavailable: " .. id)
		near(x, pos[1])
		near(y, pos[2])
		near(z, pos[3])
	end
end
function DrawScreen()
	if done or Spring.GetGameFrame() < 3 then
		return
	end
	local ok, error = pcall(function()
		frames = frames + 1
		if phase == 0 then
			Spring.SendCommands("minimap min 1", "disticon 100000")
			Spring.SetCameraState(
				{ mode = 1, px = 2200, py = 96, pz = 2250, height = 4000, angle = 0.35, flipped = 0 },
				0
			)
			for _, region in ipairs({ "home", "frontier" }) do
				local z = region == "home" and 1300 or 3200
				units[region] = Spring.GetGameRulesParam(region .. "_unit")
				features[region] = Spring.GetGameRulesParam(region .. "_feature")
				local projectile = Spring.GetGameRulesParam(region .. "_projectile")
				shotPositions[projectile] = { 2600, 230, z }
				local _, _, _, x, y, zmid = Spring.GetUnitPosition(units[region], true)
				samples[region .. "_unit"] = { x, y, zmid }
				local _, _, _, fx, fy, fz = Spring.GetFeaturePosition(features[region], true)
				samples[region .. "_feature"] = { fx, fy, fz }
				samples[region .. "_particle"] = { 2300, 230, z }
				samples[region .. "_projectile"] = { 2600, 260, z }
				samples[region .. "_flash"] = { 2900, 98, z }
				samples[region .. "_terrain"] = { 3400, 96, z }
			end
			phase = 1
			frames = 0
			return
		end
		if frames < 12 then
			return
		end -- let draw flags, shadow pass and GPU queues settle
		if phase == 1 then
			flags("home", true)
			flags("frontier", true)
			baseline = capture()
			assert(Spring.SetWorldDrawBounds(0, 2800, 5120, 4096))
			phase = 2
			frames = 0
		elseif phase == 2 then
			flags("home", false)
			flags("frontier", true)
			simulation()
			hidden = capture()
			for name, values in pairs(baseline) do
				local diff = difference(values, hidden[name])
				Spring.Echo("DRAW_BOUNDS_DIFF " .. name .. " " .. diff)
				if name:find("home_") then
					assert(diff > 0.0005, "outside render pixels unchanged: " .. name)
				else
					assert(diff < 0.005, "in-region render changed: " .. name)
				end
			end
			assert(not Spring.SetWorldDrawBounds(0, 0, 0, 1000))
			phase = 3
			frames = 0
		elseif phase == 3 then
			flags("home", false)
			flags("frontier", true)
			assert(Spring.SetWorldDrawBounds(0, 0, 5120, 2500))
			phase = 4
			frames = 0
		elseif phase == 4 then
			flags("home", true)
			flags("frontier", false)
			simulation()
			switched = capture()
			for name, values in pairs(hidden) do
				assert(difference(values, switched[name]) > 0.0005, "region switch did not change pixels: " .. name)
			end
			assert(Engine.FeatureSupport.objectDrawBounds)
			assert(Spring.SetObjectDrawBounds(-256, -256, 5376, 4352))
			phase = 5
			frames = 0
		elseif phase == 5 then
			flags("home", true)
			flags("frontier", true)
			simulation()
			local expanded = capture()
			for name, values in pairs(expanded) do
				if name == "frontier_terrain" then
					assert(difference(values, switched[name]) < 0.005, "object override exposed terrain")
				elseif name:find("frontier_") then
					assert(difference(values, switched[name]) > 0.0005, "object override did not restore " .. name)
				end
			end
			assert(not Spring.SetObjectDrawBounds(0, 0, 0, 1000))
			assert(not Spring.SetObjectDrawBounds(0, 0, math.huge, 1000))
			assert(not Spring.SetObjectDrawBounds(0, 0 / 0, 1000, 1000))
			assert(not pcall(Spring.SetObjectDrawBounds, 0, 0, 1000))
			phase = 6
			frames = 0
		elseif phase == 6 then
			flags("home", true)
			flags("frontier", true)
			assert(Spring.SetObjectDrawBounds())
			phase = 7
			frames = 0
		elseif phase == 7 then
			flags("home", true)
			flags("frontier", true)
			local clearedObjects = capture()
			assert(
				difference(clearedObjects.frontier_terrain, switched.frontier_terrain) < 0.005,
				"clearing object bounds cleared terrain bounds"
			)
			assert(Spring.SetWorldDrawBounds(0, 0, 5120, 2500))
			phase = 8
			frames = 0
		elseif phase == 8 then
			flags("home", true)
			flags("frontier", false)
			assert(Spring.SetWorldDrawBounds())
			phase = 9
			frames = 0
		elseif phase == 9 then
			flags("home", true)
			flags("frontier", true)
			simulation()
			local cleared = capture()
			for name, values in pairs(cleared) do
				if name:find("frontier_") then
					assert(difference(values, switched[name]) > 0.0005, "clear did not restore render: " .. name)
					assert(difference(values, hidden[name]) < 0.005, "clear changed restored render: " .. name)
				end
			end
			-- Exercise a unit and a real projectile beyond the physical map edge.
			Spring.SendLuaRulesMsg("draw_bounds_offmap")
			for id, pos in pairs(shotPositions) do
				if pos[3] == 1300 then
					shotPositions[id] = { -128, 230, 1600 }
				end
			end
			Spring.SetCameraState(
				{ mode = 1, px = 100, py = 96, pz = 1500, height = 1600, angle = 0.35, flipped = 0 },
				0
			)
			assert(Spring.SetWorldDrawBounds(0, 0, 5120, 4096))
			phase = 10
			frames = 0
		elseif phase == 10 then
			local x, _, _, mx, my, mz = Spring.GetUnitPosition(units.home, true)
			assert(x < 0, "fixture unit was clamped back onto the map")
			assert(worldFlags(Spring.GetUnitDrawFlag(units.home)) == 0)
			offmapHidden.unit = pixels(mx, my, mz)
			offmapHidden.projectile = pixels(-128, 260, 1600)
			assert(Spring.SetObjectDrawBounds(-256, -256, 5376, 4352))
			phase = 11
			frames = 0
		else
			local _, _, _, x, y, z = Spring.GetUnitPosition(units.home, true)
			assert(worldFlags(Spring.GetUnitDrawFlag(units.home)) ~= 0, "offmap unit missing")
			assert(difference(pixels(x, y, z), offmapHidden.unit) > 0.0005, "offmap model pixels missing")
			assert(
				difference(pixels(-128, 260, 1600), offmapHidden.projectile) > 0.0005,
				"offmap projectile pixels missing"
			)
			simulation()
			local w, h = Spring.GetViewGeometry()
			gl.SaveImage(0, 0, w, h, "draw-offmap.png")
			Spring.Echo("DRAW_BOUNDS_PASS")
			done = true
			Spring.SendCommands("quitforce")
		end
	end)
	if not ok then
		done = true
		Spring.Echo("DRAW_BOUNDS_FAIL " .. tostring(error))
		Spring.SendCommands("quitforce")
	end
end
