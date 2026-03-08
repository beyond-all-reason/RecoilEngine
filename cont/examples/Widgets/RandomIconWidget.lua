--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    RandomIconWidget.lua
--  brief:   Demonstrates custom unit icon API - randomly changes selected unit's icon
--  author:  MiniMax M2.5 LLM
--
--  Copyright (C) 2026.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--  Usage:
--    Press TAB while a unit is selected to change its icon randomly
--    Or type /randomicon in the chat
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "RandomUnitIcon",
		desc = "Randomly changes selected unit's icon (press TAB)\n" ..
		       "Usage: Press TAB or type /randomicon",
		author = "MiniMax M2.5 LLM",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = true,
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Cached Spring functions
local spGetSelectedUnits = Spring.GetSelectedUnits
local spGetAllIconDataArray = Spring.GetAllIconDataArray
local spGetUnitIcon = Spring.GetUnitIcon
local spSetUnitIcon = Spring.SetUnitIcon
local spEcho = Spring.Echo

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:RandomizeSelectedUnitIcon()
	local selUnits = spGetSelectedUnits()
	if #selUnits == 0 then
		spEcho("[RandomIcon] No units selected")
		return
	end

	local unitID = selUnits[1]

	-- Get all available icons
	local icons = spGetAllIconDataArray(true)
	if not icons or #icons == 0 then
		spEcho("[RandomIcon] No icons available")
		return
	end

	-- Extract icon names
	local iconNames = {}
	for i, iconData in ipairs(icons) do
		if iconData.name then
			iconNames[#iconNames + 1] = iconData.name
		end
	end

	if #iconNames == 0 then
		spEcho("[RandomIcon] No icon names found")
		return
	end

	-- Get current icon for the unit
	local currentIconName = spGetUnitIcon(unitID)
	spEcho("[RandomIcon] Current icon: " .. tostring(currentIconName))

	-- Pick a random icon (different from current)
	local randomIndex = math.random(1, #iconNames)
	local newIconName = iconNames[randomIndex]

	-- Keep picking until we get a different icon
	local attempts = 0
	while newIconName == currentIconName and attempts < 10 do
		randomIndex = math.random(1, #iconNames)
		newIconName = iconNames[randomIndex]
		attempts = attempts + 1
	end

	-- Set the new icon
	spSetUnitIcon(unitID, newIconName)
	spEcho("[RandomIcon] Changed icon to: " .. newIconName)

	-- Show some available icons in console (limit to first 10)
	local numToShow = math.min(10, #iconNames)
	local iconsToShow = {}
	for i = 1, numToShow do
		iconsToShow[#iconsToShow + 1] = iconNames[i]
	end
	spEcho("[RandomIcon] Sample icons: " .. table.concat(iconsToShow, ", "))
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:TextCommand(cmd)
	if cmd == "randomicon" then
		widget:RandomizeSelectedUnitIcon()
		return true
	end
end

-- Handle key presses
function widget:KeyPress(key)
	-- TAB key = 270
	if key == 270 then
		widget:RandomizeSelectedUnitIcon()
		return true
	end
	return false
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
	spEcho("[RandomIcon] Widget loaded. Press TAB or type /randomicon to randomize selected unit's icon")
end

function widget:Shutdown()
	spEcho("[RandomIcon] Widget shutdown")
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
