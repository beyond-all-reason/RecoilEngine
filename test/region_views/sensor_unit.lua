-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local unit = VFS.Include("units/region_test_unit.lua").region_test_unit
unit.name = "Sensor region test unit"
unit.activateWhenBuilt = true
unit.onoffable = true
unit.maxVelocity = 2
unit.acceleration = 1
unit.brakeRate = 1
unit.turnRate = 2000
unit.movementClass = "SENSOR_LAND"
unit.seismicSignature = 4
unit.canCloak = true
unit.cloakCost = 0
unit.cloakCostMoving = 0
unit.cloakTimeout = 0
return { region_sensor_unit = unit }
