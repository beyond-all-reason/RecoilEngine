return function(scope)
	local baseline = Spring.GetModOptions().debug_validation_baseline == "1"
	for _, name in ipairs({ "getregistry", "getmetatable", "setmetatable" }) do
		assert(type(debug[name]) == (baseline and "function" or "nil"), scope .. ": debug." .. name)
	end
	assert(type(debug.traceback) == "function", scope .. ": traceback missing")
	assert(debug.traceback("validation traceback"):find("validation traceback", 1, true))
	assert(type(debug.getinfo(1, "S")) == "table", scope .. ": getinfo broken")
	local plain = setmetatable({}, { __index = { probe = "ordinary table" } })
	assert(plain.probe == "ordinary table", scope .. ": ordinary metatables broken")

	if scope == "LuaUI" then
		assert(type(debug.emulateKeyPress) == "function", "engine debug entries missing")
		local font = gl.LoadFont("fonts/FreeSansBold.otf", 20, 2, 1.5)
		assert(font ~= nil, "font load failed")
		assert(getmetatable(font) == "protected metatable", "font metatable protection missing")
		assert(not pcall(setmetatable, font, {}), "base setmetatable accepted engine userdata")
		if baseline then
			local original = debug.getmetatable(font)
			assert(type(debug.getregistry().Font) == "table", "registry did not expose Font")
			debug.setmetatable(font, { __index = { probe = "retagged" } })
			local result = font.probe
			debug.setmetatable(font, original)
			assert(result == "retagged", "baseline did not allow retagging engine userdata")
		end
		assert(font:GetTextWidth("validation") > 0, "normal font use failed")
		gl.DeleteFont(font)
	end
	Spring.Echo("[DebugMetatables] " .. scope .. " PASS baseline=" .. tostring(baseline))
end
