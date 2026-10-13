return function(scope)
	local baseline = Spring.GetModOptions().timer_validation_baseline == "1"
	local function check()
		local milliseconds = Spring.GetTimer()
		local microseconds = Spring.GetTimerMicros()
		assert(type(milliseconds) == "userdata")
		assert(Spring.DiffTimers(milliseconds, milliseconds) == 0)
		assert(Spring.DiffTimers(microseconds, microseconds, true, true) == 0)
		local now, nowMicros
		repeat
			now = Spring.GetTimer()
			nowMicros = Spring.GetTimerMicros()
		until Spring.DiffTimers(now, milliseconds) >= 0.01
		for _, pair in ipairs({{now, milliseconds, false}, {nowMicros, microseconds, true}}) do
			local seconds = Spring.DiffTimers(pair[1], pair[2], nil, pair[3])
			local millis = Spring.DiffTimers(pair[1], pair[2], true, pair[3])
			assert(seconds >= 0.009 and seconds < 10)
			assert(math.abs(millis - seconds * 1000) < 0.001)
		end
		local legacyMicros = Spring.DiffTimers(nowMicros, microseconds)
		local correctMicros = Spring.DiffTimers(nowMicros, microseconds, false, true)
		assert(math.abs(legacyMicros - correctMicros * 1000) < 0.001)
		for _, lastFrame in ipairs({false, true}) do
			local frameTimer = Spring.GetFrameTimer(lastFrame)
			assert(Spring.DiffTimers(frameTimer, frameTimer) == 0)
			assert(Spring.DiffTimers(Spring.GetTimer(), frameTimer) >= 0)
		end
		local ephemeral = Spring.GetTimerMicros()
		local weak = setmetatable({ephemeral}, {__mode = "v"})
		ephemeral = nil
		collectgarbage("collect")
		assert((weak[1] ~= nil) == baseline)
		assert(Spring.DiffTimers(now, milliseconds) >= 0.01)
		if not baseline then
			assert(getmetatable(milliseconds) == "protected metatable")
			assert(not pcall(Spring.DiffTimers, milliseconds, now))
		end
		for _, invalid in ipairs({123, {}, "timer", false}) do
			assert(not pcall(Spring.DiffTimers, invalid, milliseconds))
			assert(not pcall(Spring.DiffTimers, now, invalid))
		end
		assert(not pcall(Spring.DiffTimers, nil, milliseconds))
		assert(not pcall(Spring.DiffTimers, now, nil))
		if scope == "LuaUI" then
			collectgarbage("stop")
			local query = assert(gl.CreateQuery())
			local accepted = pcall(Spring.DiffTimers, query, microseconds, false, true)
			gl.DeleteQuery(query)
			collectgarbage("restart")
			assert(accepted == baseline, "foreign query pointer accepted as timer")
			Spring.Echo("[TimerHandles] foreign pointer check PASS")
			local font = assert(gl.LoadFont("fonts/FreeSansBold.otf", 16))
			assert(not pcall(Spring.DiffTimers, font, milliseconds))
			assert(not pcall(Spring.DiffTimers, now, font))
			if not baseline and debug.getmetatable and debug.setmetatable then
				local original = debug.getmetatable(font)
				debug.setmetatable(font, debug.getmetatable(milliseconds))
				assert(not pcall(Spring.DiffTimers, font, milliseconds))
				assert(not pcall(Spring.DiffTimers, now, font))
				debug.setmetatable(font, original)
				Spring.Echo("[TimerHandles] forged metatable check PASS")
			end
			gl.DeleteFont(font)
		end
		if not baseline then
			for iteration = 1, 5000 do
				local transient = Spring.GetTimerMicros()
				assert(Spring.DiffTimers(transient, transient, false, true) == 0)
			end
			collectgarbage("collect")
			assert(Spring.DiffTimers(now, milliseconds) >= 0.01)
		end
		Spring.Echo("[TimerHandles] " .. scope .. " PASS baseline=" .. tostring(baseline))
	end
	if scope == "LuaUI" then
		local completed = false
		function DrawScreen()
			if completed then return end
			completed = true
			local success, message = pcall(check)
			if not success then Spring.Echo("[TimerHandles] FAIL " .. tostring(message)) end
		end
		function Update()
			if Spring.GetGameFrame() == 0 then Spring.SendCommands("forcestart") end
			if completed and Spring.GetGameFrame() >= 30 then Spring.Quit() end
		end
	else
		check()
	end
end
