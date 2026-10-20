-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local counts = { 0, 0 }
local phase, draws, done = 0, 0, false
function NanoParticleUpdate(events, count)
	for n = 0, count - 1 do
		local z = events[n * 13 + 5]
		if events[n * 13 + 1] == 1 then
			local region = z < 2500 and 1 or 2
			counts[region] = counts[region] + 1
		end
	end
end
local function pixels(z)
	local sx, sy = Spring.WorldToScreenCoords(2100, 130, z)
	local total = 0
	for _, column in ipairs(gl.ReadPixels(sx - 180, sy - 80, 360, 160)) do
		for _, c in ipairs(column) do
			if c[2] > 0.45 and c[3] > 0.45 and c[1] < c[2] * 0.6 then
				total = total + 1
			end
		end
	end
	return total
end
function DrawScreen()
	if done or Spring.GetGameFrame() < 60 then
		return
	end
	local ok, err = pcall(function()
		draws = draws + 1
		if phase == 0 then
			Spring.SendCommands("minimap min 1")
			Spring.SetCameraState(
				{ mode = 1, px = 2200, py = 96, pz = 2200, height = 3700, angle = 0.05, flipped = 0 },
				0
			)
			phase = 1
			draws = 0
		elseif draws > 20 then
			if phase == 1 then
				assert(counts[1] > 0 and counts[2] > 0, "real construction emitted no nano events")
				local w, h = Spring.GetViewGeometry()
				gl.SaveImage(0, 0, w, h, "native-nanos.png")
				local a, b = pixels(1300), pixels(3200)
				assert(
					a > 4 and b > 4,
					"native nanos missing from whole-map view "
						.. a
						.. ","
						.. b
						.. " events="
						.. counts[1]
						.. ","
						.. counts[2]
				)
				Spring.SetWorldDrawBounds(0, 0, 5120, 2500)
			elseif phase == 2 then
				assert(pixels(1300) > 4 and pixels(3200) == 0, "native nanos did not follow first region")
				Spring.SetWorldDrawBounds(0, 2800, 5120, 4096)
			elseif phase == 3 then
				assert(pixels(3200) > 4 and pixels(1300) == 0, "native nanos did not follow second region")
				assert(Spring.SetObjectDrawBounds(-256, -256, 5376, 4352))
			elseif phase == 4 then
				assert(pixels(1300) > 4 and pixels(3200) > 4, "object override did not restore native nanos")
				assert(Spring.SetObjectDrawBounds(0, 0, 5120, 2500))
			elseif phase == 5 then
				assert(pixels(1300) > 4 and pixels(3200) == 0, "native nanos followed terrain instead of object bounds")
				Spring.SetWorldDrawBounds()
			else
				assert(pixels(1300) > 4 and pixels(3200) > 4, "clearing did not restore native nanos")
				Spring.Echo("NANO_BOUNDS_PASS events=" .. counts[1] .. "," .. counts[2])
				done = true
				Spring.SendCommands("quitforce")
			end
			phase = phase + 1
			draws = 0
		end
	end)
	if not ok then
		done = true
		Spring.Echo("NANO_BOUNDS_FAIL " .. err)
		Spring.SendCommands("quitforce")
	end
end
