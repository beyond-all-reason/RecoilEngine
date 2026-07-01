if (Engine.Unsynced.GetConfigInt('Headless', 0) == 0) then
   return false
end

function widget:GetInfo()
   return {
      name = "HeadlessSetup",
      desc = "Setup for headless botrunner games.",
      author = "aegis",
      date = "October 18, 2009",
      license = "Public Domain",
      layer = 0,
      enabled = true,
   }
end

local startingSpeed = 120
local timer
local headless

function widget:Initialize()
   headless = (Engine.Unsynced.GetConfigInt('Headless', 0) ~= 0)
   if (headless) then
      Engine.Shared.Echo('Prepping for headless...')
      Engine.Unsynced.SendCommands(
         string.format('setmaxspeed %i', startingSpeed),
         string.format('setminspeed %i', startingSpeed),
         'hideinterface'
         )
   end
end

function widget:GameStart()
   Engine.Shared.Echo('Game started... starting timer.')
   timer = Engine.Unsynced.GetTimer()
end

function widget:GameOver()
   local time = Engine.Unsynced.DiffTimers(Engine.Unsynced.GetTimer(), timer)
   Engine.Shared.Echo(string.format('Game over, realtime: %i seconds, gametime: %i seconds', time, Engine.Shared.GetGameSeconds()))
end

