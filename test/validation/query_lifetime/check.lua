local baseline = Spring.GetModOptions().query_validation_baseline == "1"
local completed = false

local function run(query)
	local called = false
	gl.RunQuery(query, function(argument)
		assert(argument == "argument")
		called = true
	end, "argument")
	return called
end

local function check()
	assert(gl.CreateQuery, "occlusion queries unavailable")
	collectgarbage("stop")
	local first = assert(gl.CreateQuery())
	local second = assert(gl.CreateQuery())
	assert(run(first))
	assert(type(gl.GetQuery(first)) == "number")
	gl.DeleteQuery(first)
	gl.DeleteQuery(first)
	assert(run(second) == not baseline, "double deletion affected another query")
	if not baseline then
		assert(getmetatable(second) == "protected metatable")
		assert(not run(first))
		assert(gl.GetQuery(first) == nil)
	end
	gl.DeleteQuery(second)
	local weak = setmetatable({second}, {__mode = "v"})
	first, second = nil, nil
	collectgarbage("restart")
	collectgarbage("collect")
	assert((weak[1] ~= nil) == baseline, "handle ownership mismatch")
	if baseline then
		Spring.Echo("[QueryLifetime] PASS baseline=true")
		return
	end
	local query = assert(gl.CreateQuery())
	for iteration = 1, 8 do
		local garbage = {}
		for allocation = 1, 1000 do garbage[allocation] = {allocation} end
		collectgarbage("collect")
		assert(run(query))
		assert(type(gl.GetQuery(query)) == "number")
	end
	gl.DeleteQuery(nil)
	local font = assert(gl.LoadFont("fonts/FreeSansBold.otf", 16))
	for _, invalid in ipairs({123, {}, font, Spring.GetTimer()}) do
		assert(not pcall(gl.DeleteQuery, invalid))
		assert(not pcall(gl.GetQuery, invalid))
		assert(not pcall(gl.RunQuery, invalid, function() end))
	end
	assert(not pcall(gl.GetQuery, nil))
	assert(not pcall(gl.RunQuery, nil, function() end))
	assert(not pcall(gl.RunQuery, query, "not a function"))
	gl.RunQuery(query, function()
		assert(not pcall(gl.RunQuery, query, function() end))
		assert(not pcall(gl.GetQuery, query))
		local others = {}
		for iteration = 1, 128 do others[iteration] = assert(gl.CreateQuery()) end
		for _, other in ipairs(others) do gl.DeleteQuery(other) end
	end)
	local success, message = pcall(gl.RunQuery, query, function() error("query-callback-marker") end)
	assert(not success and string.find(message, "query%-callback%-marker"))
	assert(run(query))
	gl.DeleteQuery(query)
	assert(not run(query) and gl.GetQuery(query) == nil)
	local active = assert(gl.CreateQuery())
	local other = assert(gl.CreateQuery())
	assert(run(other))
	gl.RunQuery(active, function()
		gl.DeleteQuery(active)
		gl.DeleteQuery(active)
		assert(gl.GetQuery(active) == nil)
		assert(not pcall(gl.RunQuery, other, function() end))
		local replacement = assert(gl.CreateQuery())
		gl.DeleteQuery(replacement)
		assert(type(gl.GetQuery(other)) == "number")
	end)
	assert(not run(active))
	assert(run(other))
	gl.DeleteQuery(other)
	if debug.getmetatable and debug.setmetatable then
		local metatable = debug.getmetatable(query)
		local original = metatable.__gc
		local finalized = 0
		metatable.__gc = function(handle)
			finalized = finalized + 1
			original(handle)
		end
		local ephemeral = assert(gl.CreateQuery())
		local collectible = setmetatable({ephemeral}, {__mode = "v"})
		ephemeral = nil
		collectgarbage("collect")
		assert(collectible[1] == nil and finalized >= 1)
		metatable.__gc = original
		original(query)
		original(query)
		local fontMetatable = debug.getmetatable(font)
		debug.setmetatable(font, metatable)
		gl.DeleteQuery(font)
		assert(not run(font) and gl.GetQuery(font) == nil)
		debug.setmetatable(font, fontMetatable)
		local abandoned = assert(gl.CreateQuery())
		debug.setmetatable(abandoned, nil)
		abandoned = nil
		collectgarbage("collect")
		assert(run(assert(gl.CreateQuery())))
		Spring.Echo("[QueryLifetime] debug tampering PASS")
	end
	gl.DeleteFont(font)
	shutdownQuery = assert(gl.CreateQuery())
	assert(run(shutdownQuery))
	Spring.Echo("[QueryLifetime] PASS baseline=false")
end

function DrawScreen()
	if completed then return end
	completed = true
	local success, message = pcall(check)
	if not success then Spring.Echo("[QueryLifetime] FAIL " .. tostring(message)) end
end

function Update()
	if Spring.GetGameFrame() == 0 then Spring.SendCommands("forcestart") end
	if completed and Spring.GetGameFrame() >= 30 then Spring.Quit() end
end
