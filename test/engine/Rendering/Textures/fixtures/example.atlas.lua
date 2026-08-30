return {
	schema = "recoil.texture-atlas",
	version = 2,
	name = "fixture",
	target = "2d",
	width = 32,
	height = 16,
	mipLevels = 5,
	coordinateOrigin = "top-left",
	variants = {
		{ id = "rgba8", format = "rgba8", files = { "fixture.rgba8.dds" } },
	},
	pages = {
		{ reserve = { { x = 16, y = 0, width = 16, height = 16 } } },
	},
	defaultPadding = { mode = "clamp", pixels = 1 },
	entries = {
		["images/fixture.png"] = {
			page = 0,
			source = "Images/Fixture.png",
			sourceWidth = 6,
			sourceHeight = 6,
			content = { x = 1, y = 1, width = 6, height = 6 },
			allocation = { x = 0, y = 0, width = 8, height = 8 },
			padding = { mode = "clamp", pixels = 1 },
		},
	},
}
