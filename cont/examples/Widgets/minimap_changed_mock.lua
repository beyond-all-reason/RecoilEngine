function widget:GetInfo()
	return {
		name = "MiniMap Callins Test",
		desc = "Testing the MiniMap Changed Callins",
		author = "TheFutureKnight",
		date = "2025-5-14",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = true,
	}
end

function widget:MiniMapRotationChanged(newRot, oldRot)
	Spring.Echo("MMRC: from " .. oldRot .. " to " .. newRot)
end


function widget:MiniMapGeometryChanged(newPosX, newPosY, newDimX, newDimY, oldPosX, oldPosY, oldDimX, oldDimY)
	Spring.Echo("MMGC: from " .. oldPosX, oldPosY, oldDimX, oldDimY .. " to " .. newPosX, newPosY, newDimX, newDimY)
end

function widget:MiniMapStateChanged(isMini, isMax)
	Spring.Echo("MMSC: Minimized - " .. tostring(isMini) .. " Maximized - " .. tostring(isMax))
end