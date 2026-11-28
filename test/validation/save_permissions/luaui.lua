local options = Spring.GetModOptions()
local useCheats = options.save_validation_cheats == "1"
local nextAttempt = 30
local attempt = 0
local commands = {
	"save validation",
	"luasave validation",
	"save overwrite -y",
	"luasave overwrite -y",
}

function Update()
	local frame = Spring.GetGameFrame()
	if frame == 0 then
		Spring.SendCommands("forcestart")
		return
	end
	if useCheats and not Spring.IsCheatingEnabled() then
		Spring.SendCommands("cheat 1")
		return
	end
	if frame < nextAttempt then
		return
	end
	if attempt == #commands then
		Spring.Echo("[SavePermissions] complete cheats=" .. tostring(Spring.IsCheatingEnabled()))
		Spring.Quit()
		return
	end
	attempt = attempt + 1
	Spring.Echo("[SavePermissions] attempt " .. commands[attempt])
	if attempt % 2 == 0 then
		Spring.SendCommands({ commands[attempt] })
	else
		Spring.SendCommands(commands[attempt])
	end
	nextAttempt = frame + 30
end
