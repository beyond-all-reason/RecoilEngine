-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

local empty_root_piece = piece("empty_root_piece")

function script.Create()
end

function script.Killed(recentDamage, maxHealth)
	return 0
end

function script.QueryNanoPiece() return empty_root_piece end
function script.StartBuilding() SetUnitValue(COB.INBUILDSTANCE, 1) end
function script.StopBuilding() SetUnitValue(COB.INBUILDSTANCE, 0) end
