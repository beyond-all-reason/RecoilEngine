-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local stage, phase, frames, failed = 1, 0, 0, false
local changedFrame = 0
function DrawScreen()
	if failed or Spring.GetGameFrame() < 30 then
		return
	end
	local ok, err = pcall(function()
		if stage > 4 then
			Spring.Echo("FLAME_BOUNDS_PASS")
			Spring.SendCommands("quitforce")
			return
		end
		frames = frames + 1
		if phase == 0 then
			Spring.SetCameraState(
				{
					mode = 1,
					px = 2048,
					py = 96,
					pz = stage <= 2 and 1200 or 3000,
					height = 1400,
					angle = stage % 2 == 1 and 0.05 or 0.8,
					flipped = 0,
				},
				0
			)
			phase = 1
			frames = 0
			changedFrame = Spring.GetGameFrame()
		elseif phase == 1 and frames > 15 and Spring.GetGameFrame() > changedFrame + 1 then
			local w, h = Spring.GetViewGeometry()
			local x, y, z = Spring.GetCameraPosition()
			local dx, dy, dz = Spring.GetPixelDir(w + 8, h / 2)
			local distance = -400 / dy
			Spring.SendLuaRulesMsg(
				string.format("flare %d %.4f %.4f %.4f", stage, x + dx * distance, y - 400, z + dz * distance)
			)
			phase = 2
			frames = 0
			changedFrame = Spring.GetGameFrame()
		elseif
			phase == 2
			and frames > 15
			and Spring.GetGameFrame() > changedFrame + 2
			and Spring.GetGameRulesParam("flare_ready") == stage
		then
			local w, h = Spring.GetViewGeometry()
			local count = 0
			for _, row in ipairs(gl.ReadPixels(w - 100, h / 2 - 120, 100, 240)) do
				for _, c in ipairs(row) do
					if c[2] > 0.8 and c[3] > 0.8 and c[1] < 0.3 then
						count = count + 1
					end
				end
			end
			assert(count > 20, "off-frustum negative-size flare missing " .. stage .. " cyan=" .. count)
			Spring.Echo("FLAME_BOUNDS_CASE " .. stage .. " cyan=" .. count)
			stage = stage + 1
			phase = 0
			frames = 0
		end
	end)
	if not ok then
		failed = true
		Spring.Echo("FLAME_BOUNDS_FAIL " .. tostring(err))
		Spring.SendCommands("quitforce")
	end
end
