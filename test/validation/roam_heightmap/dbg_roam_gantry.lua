-- Local single-player reproducer. Place in luarules/gadgets/ of a BAR development game.
function gadget:GetInfo()
    return {name = "Ditched ROAM gantry reproducer", layer = 99998, enabled = true}
end

if Game.mapName ~= "Ditched_V1" then return false end
local x, z = 352, 2048

if not gadgetHandler:IsSyncedCode() then
    local positioned = false
    function gadget:Update()
        if positioned or Spring.GetGameFrame() < 75 then return end
        positioned = true
        local dx, dy, dz = 0, -267, -330
        local length = math.sqrt(dy * dy + dz * dz)
        Spring.SendCommands({"shadows 1", "grounddetail 200"})
        Spring.SetCameraState({
            name = "fps", mode = 0, px = x + 48,
            py = Spring.GetGroundHeight(x,z) + 296, pz = z + 330,
            rx = math.acos(dy / length), ry = 0, rz = 0,
            dx = dx, dy = dy / length, dz = dz / length, fov = 45,
        }, 0)
    end
    return
end

local builder
function gadget:GameFrame(f)
    if f == 15 then
        builder = Spring.CreateUnit("armaca", x + 140, Spring.GetGroundHeight(x + 140,z) + 60, z, 0, 0)
        assert(builder, "Could not create team 0 construction aircraft")
        Spring.SetUnitBuildSpeed(builder, 12000)
    elseif f == 175 then
        Spring.GiveOrderToUnit(builder, -UnitDefNames.armshltx.id, {x, Spring.GetGroundHeight(x,z), z, 3}, 0)
    end
    if f == 120 then
        Spring.SetTeamResource(0, "ms", 100000)
        Spring.SetTeamResource(0, "es", 100000)
        Spring.SetTeamResource(0, "m", 90000)
        Spring.SetTeamResource(0, "e", 90000)
    end
end
