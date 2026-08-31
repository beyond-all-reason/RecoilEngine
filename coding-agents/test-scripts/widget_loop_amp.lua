-- Draws a lot of identical LINE_LOOP batches, to make the batch-merging artefact
-- frequent enough to measure.
--
-- The artefact appears in about 8% of frames on SplinterFaction's minimap, which
-- issues one gl.BeginEnd(GL_LINE_LOOP) per metal spot, roughly fifty a frame.
-- At that rate a 240 sample run yields nine events, which cannot separate two
-- configurations: baseline against flush came out 9 of 115 against 4 of 115,
-- p is about 0.25.
--
-- This draws the same shape many times more per frame. If merging happens with
-- some probability per adjacent pair of batches, raising the count raises the
-- events per frame in proportion, and the score stops being "did it happen" and
-- becomes "how often", which is far more powerful for the same number of samples.
--
-- The circles are drawn in a colour nothing else on screen uses, in a fixed
-- rectangle, so scoring is a plain pixel count. A clean frame gives the circles
-- alone. A merged pair adds a long connecting segment between two circle centres,
-- which is many pixels and cannot be confused with the circles themselves.
--
-- ALT_COLOURS measures a different defect with the same grid. Identical batches
-- are what makes merging visible, and they are exactly what hides a batch
-- inheriting the previous one's attributes, because an inherited attribute is
-- then the same attribute. So with ALT_COLOURS the colour alternates by batch
-- index and nothing else changes: same shape, same vertex count, same vertex
-- format. A batch that takes the previous batch's colour draws a whole circle in
-- the wrong one, which is every pixel of that circle rather than a stray line.

function widget:GetInfo()
	return {
		name    = "Loop amp",
		desc    = "Draws many identical LINE_LOOP batches so batch merging can be measured",
		author  = "",
		date    = "",
		license = "GPL v2 or later",
		layer   = 2000,
		enabled = true,
	}
end

-- install-probe.sh --loops <n> rewrites this and reads it back.
local LOOP_COUNT = 0

-- install-probe.sh --alt rewrites this and reads it back.
local ALT_COLOURS = 0

-- Centre right, clear of the build menu on the left and of the probe's own echo
-- text along the bottom, both of which overlapped the grid on the first attempt.
local COLS    = 55
local RADIUS  = 7
local DIVS    = 16
local SPACING = 22
local ORIGIN_X = 1620
local ORIGIN_Y = 500
local MARGIN   = 20

local glBeginEnd = gl.BeginEnd
local glVertex   = gl.Vertex
local glColor    = gl.Color
local glRect     = gl.Rect
local GL_LINE_LOOP = GL.LINE_LOOP

local cosines = {}
local sines   = {}

local function drawCircle(cx, cy)
	for i = 1, DIVS do
		glVertex(cx + cosines[i], cy + sines[i], 0)
	end
end

function widget:Initialize()
	if LOOP_COUNT <= 0 then
		widgetHandler:RemoveWidget(self)
		return
	end

	for i = 1, DIVS do
		local a = (i / DIVS) * math.pi * 2
		cosines[i] = math.cos(a) * RADIUS
		sines[i]   = math.sin(a) * RADIUS
	end

	Spring.Echo(string.format("[loopamp] %d line loops a frame at %d,%d, alt colours %d",
		LOOP_COUNT, ORIGIN_X, ORIGIN_Y, ALT_COLOURS))
end

function widget:DrawScreen()
	if LOOP_COUNT <= 0 then
		return
	end

	local rows = math.ceil(LOOP_COUNT / COLS)

	-- An opaque black backdrop, so the scored rectangle contains the circles and
	-- nothing else. Without it the terrain scrolls under the grid as the camera
	-- tracks the unit, and any terrain pixel that happens to pass the colour test
	-- lands in the count as a false stray.
	--
	-- gl.Rect goes through glRectf rather than glBegin, so it is not itself a
	-- batch and cannot take part in the merging being measured.
	glColor(0, 0, 0, 1)
	glRect(ORIGIN_X - RADIUS - MARGIN,
	       ORIGIN_Y - RADIUS - MARGIN,
	       ORIGIN_X + (COLS - 1) * SPACING + RADIUS + MARGIN,
	       ORIGIN_Y + (rows - 1) * SPACING + RADIUS + MARGIN)

	-- Pure cyan on black. On the terrain a colour test has to survive whatever the
	-- map and the game's UI happen to contain, and magenta was close enough to the
	-- red rock to be a risk. Against a backdrop we control, nothing else is there.
	glColor(0, 1, 1, 1)

	for i = 0, LOOP_COUNT - 1 do
		local cx = ORIGIN_X + (i % COLS) * SPACING
		local cy = ORIGIN_Y + math.floor(i / COLS) * SPACING

		-- The colour is set outside the batch, which is the state a batch is
		-- meant to pick up when it begins. Magenta rather than a third hue so
		-- both colours are saturated in channels the backdrop has none of.
		if ALT_COLOURS ~= 0 then
			if i % 2 == 0 then
				glColor(0, 1, 1, 1)
			else
				glColor(1, 0, 1, 1)
			end
		end

		-- One batch per circle, exactly as game_metal_spot_minimap_drawer.lua
		-- does it. The point is the batch boundary, not the shape.
		glBeginEnd(GL_LINE_LOOP, drawCircle, cx, cy)
	end

	glColor(1, 1, 1, 1)
end
