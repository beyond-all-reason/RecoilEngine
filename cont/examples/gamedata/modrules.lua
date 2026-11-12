-- The path to this file should be `./gamedata/modrules.lua` and it controls
-- various game-wide behaviours. Many of them are heavily Total Annihilation
-- oriented, including the defaults. They are also quite rigid so you might
-- want to consider a gadgeted implementation instead. The returned format is
-- a table exactly as below (in particular, values must be in their respective
-- subtables), with the values set to their defaults. Of course, this is a Lua
-- file so you can use code/logic to set them. You can use the standard Lua
-- parser interfaces such as VFS (check the docs for a full listing) with the
-- notable addition of `Spring.GetModOptions`.

return {

	system = {

		-- How large is each quad for collision checks? Must divide 1024.
		-- Larger values increase baseline performance (less constant overhead),
		-- but reduce performance when lots of units are adjacent to each other
		-- such that they are all in the same quad. Note that this value is NOT
		-- related to quad-tree pathfinder (QTPFS).
		quadFieldQuadSizeInElmos = 128,


		-- Choose a pathfinding system, with various trade-offs.
		-- -1: no pathfinder. For use in editors and such, not actual games.
		--  0: A* algorithm (aka HAPFS). Generally slower but uses less RAM.
		--  1: quad tree algorithm (aka QTPFS). Generally faster but more memory-hungry.
		pathFinderSystem = 0,

		-- Multiplier for the default distance at which a unit will check if there's
		-- a straight line path to the target. Default distance is 50 quads, and thus (FIXME is it?),
		-- depends on the setting above.
		pathFinderRawDistMult = 1.25,

		-- A multiplier for pathfinder work intensity, tweak for quality vs performance.
		pathFinderUpdateRateScale = 1.0,

		-- The minimum number of sim frames that must pass before a unit is allowed
		-- to request a new path. Until then, (FIXME actually what then? uses old path?).
		pfRepathMaxRateInFrames = 150, -- max 300

		-- Controls the speed modifier (which includes typemap boosts and up/down hill modifiers)
		-- under which units will never do raw move, regardless of distance etc. Defaults to 0,
		-- which means units will not try to raw-move into unpathable terrain (e.g. typemapped lava,
		-- cliffs, water). You can set it to some positive value to make them avoid pathable but slow
		-- slow terrain (for example if you set it to 0.2 then they will not raw-move across terrain
		-- where they move at 20% speed or less, and will use normal pathing instead - which may still
		-- end up taking them through that path).
		pfRawMoveSpeedThreshold = 0.0,

		-- QTPFS only. The minimum size a path has to be in order to be eligible for
		-- an automatic incomplete path repath (FIXME: what is an incomplete path and what are the tradeoffs to tweaking this?).
		qtRefreshPathMinDist = 512,

		-- QTPFS only. Can be used to limit the number of nodes searched in a path
		-- request, either absolute or relative to the number of pathable quads on
		-- the map. These are minimums so the final value is the larger of the two.
		qtMaxNodesSearched = 8192,
		qtMaxNodesSearchedRelativeToMapOpenNodes = 0.25,


		-- Should a smoothed ground mesh be used? Helps with some cases
		-- of sharp terrain drops (for example aircraft will know about
		-- mountains earlier and climb accordingly) but has a performance cost
		-- and gameplay consequences (e.g. aircraft won't descend into ravines).
		-- Also used for panning by some camera modes.
		enableSmoothMesh = true,

		-- Reduces the resolution of the smoothmesh. Increase to get better performance at the cost of worse accuracy.
		smoothMeshResDivider = 2,

		-- The radius for smoothing, in elmos.
		smoothMeshSmoothRadius = 40,


		-- Overrides map gravity, in elmo/s^2.
		-- `nil` falls back to map-defined gravity,
		-- which itself defaults to 130.
		forcedMapGravityStrength = nil,

		-- Is the `/take` command allowed? Mostly used when somebody
		-- disconnects, in which case the rest of the team can use
		-- his stuff, but also this kills him so he won't get to play
		-- even if he reconnects.
		allowTake = true,

		-- How much does Lua allocate, in megabytes?
		-- Shared limit across all Lua states.
		LuaAllocLimit = 1536,

		-- should the built-in playerlist via `/info` be available?
		-- It exposes some information that a game may want to hide,
		-- or mature games that already have a better playerlist may
		-- want to disable it because it's pretty crude.
		allowEnginePlayerlist = true,

		-- When using the "choose start position ingame" mode, does
		-- the engine capture mouseclicks and send startpoint requests?
		useStartPositionSelecter = true,

		-- Controls whether the resource sharing level (aka "red slider") applies.
		-- Note that this also prevents excess from flowing back to the allyteam.
		nativeExcessSharing = true,
	},

	movement = {
		-- Can *gunships* leave the map? Otherwise they get
		-- stuck on the edge. Note that despite the name, it
		-- only affects gunships (aka "hovering" aircraft)
		-- and not fixed-wing (aka "strafing") planes.
		allowAirPlanesToLeaveMap = true,

		-- Can aircraft hit the ground, for example if they
		-- are ordered into a tall cliff?
		-- (FIXME what if false? do they ghost through?)
		allowAircraftToHitGround = true,

		-- Can enemy units be pushed when colliding?
		allowPushingEnemyUnits = false,

		-- Can units with lots of momentum crush allied units?
		-- Note that enemy units are always crushable as a general
		-- rule, but also all units default to being uncrushable.
		-- Also note that units always path through crushables,
		-- with this setting allies are just ghosted through, NOT avoided.
		allowCrushingAlliedUnits = false,

		-- Can units receive collision damage when thrown?
		-- Applies to both unit-unit and unit-ground collisions.
		-- Does not happen when units are moving on their own,
		-- only when skidding due to impulse.
		allowUnitCollisionDamage = false,

		-- Can units partially overlap when colliding during movement?
		-- Makes for smoother pathing, but you get some clipping.
		allowUnitCollisionOverlap = true,

		-- Should collision checks use the Separating Axis Theorem?
		-- Better at handling edge cases with non-spherical units,
		-- but also slower.
		allowSepAxisCollisionTest = false,

		-- Should units be subject to gravity even if not skidding?
		-- This can make fast units glide if there is a sharp drop
		-- in terrain (think car stunt jumps, or C&C Generals GLA Bike).
		-- In particular, units can fall off cliffs by accident. The
		-- pathfinder will not take this "ability" into account.
		allowGroundUnitGravity = false,

		-- Can hovercraft do drifts (turn without braking)?
		-- Note, this requires `allowGroundUnitGravity`.
		allowHoverUnitStrafing = false,

		-- How many times faster can a unit be pushed,
		-- compared to its nominal max speed?
		maxCollisionPushMultiplier = math.huge,

		-- Frequency, in frames, of updating a unit's quadfield position.
		-- Can be between 1 and 15. Values higher than 1 increase performance,
		-- but can result in inaccurate collisions (up to weapons randomly not hitting).
		unitQuadPositionUpdateRate = 3,

		-- Frequency, in frames, of updating a unit's avoidance vectors.
		-- Higher values increase performance, but units can bump into
		-- each other needlessly.
		groundUnitCollisionAvoidanceUpdateRate = 3,
	},

	construction = {
		-- Do unfinished buildings start to decay when not worked on?
		-- Note that a building is still considered worked on even if
		-- there are insufficient resources to actually progress, and
		-- that units being built in factories are not subject do decay.
		-- Also, decay refunds metal at 100% return, but not energy,
		-- and it reduces both build progress and health.
		constructionDecay = true,

		-- How many seconds does it take for decay to start?
		constructionDecayTime = 6.66,

		-- How fast is the decay? This is a MULTIPLIER for the TIME
		-- it takes, which is normally proportional to the build time
		-- interpreted as HALVES of seconds. For example, a unit with
		-- a buildtime of 200 would decay in 100s under a multiplier
		-- of 1, or in 3s under the default multiplier of 0.03. If you
		-- want a less arcane formula you will need a gadget.
		constructionDecaySpeed = 0.03,

		-- Do units built from a factory receive a move command to exit?
		insertBuiltUnitMoveCommand = true,

		-- Are yardmap details taken into account when deciding overlap
		-- when queuing up buildings? If false, the whole footprint is
		-- always taken, even if there's empty edges in the yardmap.
		useYardmapsForQueuedBuildOverlap = false,
	},

	damage = {
		-- How much damage do shards from exploding units do?
		-- These are the ones spawned from a unit script with
		-- the `SFX.EXPLODE` flag. Non-damage aspects of this
		-- explosion (such as area of effect) cannot currently
		-- be controlled.
		debris = 50,
	},

	repair = {
		-- How much does unit repair cost?
		-- A fraction of the unit's energy cost. Note that you
		-- can't make repair cost metal via this interface or
		-- to make it not linearly proportional to the target
		-- unit's energy cost, but that is still doable via
		-- gadgets.
		energyCostFactor = 0.0,
	},

	capture = {
		-- How much does capturing cost?
		-- Same remarks as repair apply.
		energyCostFactor = 0.0,
	},

	resurrect = {
		-- How much does resurrection cost?
		-- Same remarks as repair and capture apply,
		-- with the cost being of the unit that will
		-- be the result of the resurrection (such
		-- that the feature's contained energy does
		-- not matter at all).
		energyCostFactor = 0.5,
	},

	reclaim = {

		-- Can multiple units reclaim the same feature?
		-- Note that this value uses 0/1, not false/true.
		-- It also does not apply to reclaiming units, only features.
		multiReclaim = 0,

		-- Is feature reclaim progress split into discrete chunks?
		-- If the value is 0, reclaim refunds resources gradually.
		-- With values higher than 1, reclaim will grant resources
		-- at this many discrete chunks. For example, with 1, all
		-- the resources are granted in a single chunk when the
		-- feature is fully reclaimed. With 2, half of the value is
		-- granted when 50% is reached, and the other at completion.
		-- With 3, 1/3 of the value is granted at 66%, 33% and full
		-- completion, etc. As a minor side effect, values other than
		-- 0 and 1 cause a feature to be unreclaimable if it starts
		-- being resurrected, until it reaches 100% value again.
		reclaimMethod = 1,

		-- How does reclaim of units work?
		-- 0: units are turned into a nanoframe (with all the usual
		--    consequences such as becoming unable to act) and then
		--    resources are refunded gradually.
		-- 1: reclaim just drains health, with no immediate refund.
		--    If this results in the unit reaching 0 health, it is
		--    removed (without a death explosion) and resources are
		--    refunded.
		unitMethod = 1,

		-- Does reclaim drain unit health?
		-- Note that this is mostly useful when `unitMethod` is 0,
		-- so that reclaim only works on build progress. Otherwise,
		-- if `unitMethod` is 1, disabling health drain makes unit
		-- reclaim do nothing.
		unitDrainHealth = true,

		-- Can finished enemy units be reclaimed?
		-- Note that nanoframes (unfinished units) can always be reclaimed,
		-- and that ceasefired enemies are still enemies.
		allowEnemies = true,

		-- Can finished ally units be reclaimed?
		-- Note that this means allies other than yourself (your own units
		-- can always be reclaimed).
		allowAllies = true,

		-- What fraction of a unit's metal cost is refunded? Energy is always 0.
		reclaimUnitEfficiency = 1.0,

		-- What fraction of the unit's energy cost does it take to reclaim?
		-- Note that this is paid gradually even if `unitMethod` is 1.
		-- Also note that setting a negative value will make reclaim refund
		-- energy, but it is still nominally a cost (matters for example for
		-- statistics, will cancel other costs instead of counting as income).
		unitEnergyCostFactor = 0.0,

		-- How much does it cost to reclaim metal from a feature?
		-- This is a (negative) multiplier for the metal income and happens in
		-- parallel to energy reclaim, so doesn't follow the same rule as units.
		-- For example, if the value is 0.5 and a feature contains 30 metal and
		-- 10 energy, it will ultimately grant 30 metal and cost 5 energy, since
		-- the cost is 15 (30 * 0.5) but happens in parallel to the 10 gain.
		-- Can be negative, but see the caveat above.
		featureEnergyCostFactor = 0.0,
	},

	paralyze = {
		-- How many seconds does it take for paralyze to
		-- decay by 100%? Note that paralyze works by going
		-- above 100% so, equivalently, this controls how
		-- much damage is required per 1s paralyze (in case
		-- of the default 40s, that's 2.5%).
		paralyzeDeclineRate = 40,

		-- Does paralyze threshold and decay work off maximum
		-- health, or current health?
		paralyzeOnMaxHealth = true,
	},

	fireAtDead = {
		-- Are units undergoing death animation valid targets?
		-- By default firing at them is useless, but a gadget
		-- could make them die quicker when fired at.
		fireAtKilled = false,

		-- Are crashing aircraft fired at? Same remarks as above.
		fireAtCrashing = false,
	},

	transportability = {
		-- Can ground units be transported (subject to other limitations)?
		-- Applies to mobile units with the KBot and Tank movetypes.
		transportGround = true,

		-- Can hovercraft (units with the Hover movetype) be transported?
		transportHover = false,

		-- Can ships (units with the Ship/Boat movetype) be transported?
		transportShip = false,

		-- Can aircraft (both hovering/gunships and fixed-wing/planes) be transported?
		transportAir = false,

		-- Can units being transported be targeted individually?
		-- If this is true then, for example, anti-ground weapons can
		-- shoot at an aircraft transport if it contains a ground unit.
		targetableTransportedUnits = false,
	},

	experience = {
		-- The multiplier for experience gain. With a x1.0 mult,
		-- a unit gains 0.1 experience for dealing 100% health's
		-- worth of damage to a target with an equal power and
		-- a further 0.1 bonus for landing the killing blow.
		-- For example if a unit hits an enemy unit which is
		-- 3 times as powerful for 90% damage, and then a second
		-- unit of the same type finishes it, then the first one will
		-- have 0.27 XP (90% * 3 * 0.1) and the other will have
		-- 0.33 XP (0.03 for dealing damage and 0.30 for last hit).
		-- There is no separate control for the two components.
		experienceMult = 1.0,

		-- The maximum bonuses as XP approaches infinity.
		-- The scaling is (XP / (XP + 1)), so for example
		-- at 1 XP the unit has half the bonus, at 2 XP
		-- it has 66%, etc., which is hardcoded.
		powerScale  = 1.0,
		healthScale = 0.7,
		reloadScale = 0.4,
	},

	featureLOS = {
		-- Should some features be always visible, even outside sight?
		-- Note that this doesn't grant sight on their surroundings,
		-- but still lets you know everything, so for example you might
		-- see them being reclaimed or destroyed.
		--
		-- 0: no features are visible out of sight.
		-- 1: Gaia features (usually map decoration features
		--    like trees or rocks) are always visible.
		-- 2: Gaia and allied (usually meaning wreckage of
		--    destroyed allied units) are always visible.
		-- 3: all features (including enemy) are visible.
		--    In particular, in a free-for-all you might see
		--    wrecks appear out of nowhere on the other side
		--    of the map when two enemies fight each other.
		featureVisibility = 3,
	},

	sensors = {
		-- Do underwater units ignore regular sight if they
		-- are not also detected by sonar? Note that sight
		-- still applies as far as detecting the specific
		-- unit type (as opposed to just a radar dot), etc.
		requireSonarUnderWater = true,

		-- Are units set to be always visible (via gadget)
		-- actually always visible even when cloaked?
		alwaysVisibleOverridesCloaked = false,

		-- Does proximity-based decloak also require detectors
		-- to have nominal sight range over the cloaked unit?
		-- Note that doesn't check actual sight as far as it
		-- can be blocked by terrain, just the radius (which
		-- can also be 3D if the cloaker has `decloakSpherical`)
		-- Use to make blind units (things like mines, or
		-- nanoframes, or fake technical units) not decloak.
		decloakRequiresLineOfSight = false,

		-- Does each allyTeam only jam enemy radar?
		-- Without it, they will jam their own radar, too.
		separateJammers = true,

		los = { -- Note the subtable.

			-- These values are resolution exponents for
			-- sight types. Each increment doubles the size of
			-- a sight tile (so 3 means x8, and 4 is x16).
			-- Increase to get better performance, but also
			-- less precise sight. Must be between 0 and 6.
			losMipLevel = 1,
			airMipLevel = 1,
			radarMipLevel = 2,
		},
	},

	guard = {

		-- Distance that a guardee must move before the guard goal is recalculated
		guardRecalculateThreshold = 100,

		-- Distance that a guardian will stop at nearing a stopped guardee
		guardStoppedProximityGoal = 50,

		-- The extra distance a guardian will keep from a stopped guardee
		guardStoppedExtraDistance = 100,

		-- Distance the guardian is considered to be in guarding range, and will match guardee velocity
		guardMovingProximityGoal = 150,

		-- Seconds' worth of guardee movement to reflect before the next position update.
		-- Smaller values will result in higher detail movement but more performance cost
		guardMovingIntervalMultiplier = 0.0,

		-- Limit for movement interception (leading a distant moving guardee to meet it)
		-- when a guardian is not in guarding range. In frames.
		guardInterceptionLimit = 0.0,
	},

	misc = {
		-- In seconds, how often wind generators receive
		-- the "wind is currently X" event. Does NOT affect
		-- the actual wind update rate.
		windChangeReportPeriod = 15,
	},
}
