-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local resources = VFS.Include("gamedata/resources.lua", nil, VFS.BASE)
resources.graphics.projectiletextures.testparticle = "flare.bmp"
resources.graphics.groundfx.groundflash = "groundflash.tga"
resources.graphics.scars = { "scars/scar1.tga", "scars/scar2.tga" }
return resources
