#!/bin/bash
# Put the perf probe widget into the throwaway game, or take it back out.
#
#   install-probe.sh                     copy it in, overwriting any earlier copy
#   install-probe.sh --luaui-off 30      the same, then issue "luaui disable" 30
#                                        seconds after the freeze
#   install-probe.sh --shots 25,60,90    also install the screenshot gadget, which
#                                        fires at those times after it loads
#   install-probe.sh --move              send the unit to the middle of the map and
#                                        track it, rather than freezing the scene
#   install-probe.sh --loops 2000        also install the artefact amplifier
#   install-probe.sh --loops 2000 --alt  the same, alternating the batch colour so
#                                        an inherited attribute is visible
#   install-probe.sh --remove            take them all out again
#
# GAME=<path to a .sdd> targets a game other than SplinterFaction. It has to be an
# unpacked directory, not a .sdz, because this writes into it.
#
# The game copy is throwaway but has to end up unmodified, so both files live in
# the repo and this is the only thing that touches the game.
#
# The screenshot gadget goes in LuaRules rather than LuaUI on purpose. What most
# needs proving is the scene after "luaui disable", and only a gadget survives it.
#
# Both flags rewrite a constant in the installed copy rather than the source, and
# read the result back before returning. Three results in one session were
# mislabelled by patches that silently did not apply, so a substitution that
# cannot be seen to have worked is worse than no substitution.
set -eu

GAME=${GAME:-$HOME/dev/spring-testdata/games/SplinterFaction_0.1.78.sdd}

# Games disagree on the case of these directories. SplinterFaction ships LuaUI
# and LuaRules, Metal Factions ships luaui and luarules, and the engine's VFS is
# case insensitive where a shell is not.
WIDGET_DIR=""
GADGET_DIR=""

for D in "$GAME/LuaUI/Widgets" "$GAME/luaui/widgets"; do
	[ -d "$D" ] && WIDGET_DIR=$D && break
done

for D in "$GAME/LuaRules/Gadgets" "$GAME/luarules/gadgets"; do
	[ -d "$D" ] && GADGET_DIR=$D && break
done

if [ -z "$WIDGET_DIR" ]; then
	echo "no widget directory under $GAME"
	exit 1
fi

DEST=$WIDGET_DIR/test_perf_probe.lua
SRC=$(dirname "$0")/widget_perf_probe.lua
SHOT_DEST=${GADGET_DIR:-$GAME/LuaRules/Gadgets}/test_shot_probe.lua
SHOT_SRC=$(dirname "$0")/gadget_shot_probe.lua
AMP_DEST=$WIDGET_DIR/test_loop_amp.lua
AMP_SRC=$(dirname "$0")/widget_loop_amp.lua

if [ "${1:-}" = "--remove" ]; then
	for F in "$DEST" "$SHOT_DEST" "$AMP_DEST"; do
		if [ -e "$F" ]; then
			rm "$F"
			echo "removed $F"
		else
			echo "nothing at $F"
		fi
	done
	exit 0
fi

LUAUI_OFF=0
SHOTS=""
MOVE=0
LOOPS=0
ALT=0

while [ $# -gt 0 ]; do
	case $1 in
		--luaui-off)
			LUAUI_OFF=${2:?--luaui-off needs a number of seconds}
			case $LUAUI_OFF in
				''|*[!0-9]*) echo "--luaui-off takes whole seconds, got \"$LUAUI_OFF\""; exit 1 ;;
			esac
			shift 2
			;;
		--shots)
			SHOTS=${2:?--shots needs a comma separated list of seconds}
			case $SHOTS in
				''|*[!0-9,]*) echo "--shots takes whole seconds separated by commas, got \"$SHOTS\""; exit 1 ;;
			esac
			shift 2
			;;
		--move)
			MOVE=1
			shift
			;;
		--loops)
			LOOPS=${2:?--loops needs a count}
			case $LOOPS in
				''|*[!0-9]*) echo "--loops takes a whole number, got \"$LOOPS\""; exit 1 ;;
			esac
			shift 2
			;;
		--alt)
			ALT=1
			shift
			;;
		*)
			echo "unknown argument \"$1\""
			exit 1
			;;
	esac
done

cp "$SRC" "$DEST"

if [ "$LUAUI_OFF" != 0 ]; then
	sed -i '' "s/^local DISABLE_AFTER = 0\$/local DISABLE_AFTER = $LUAUI_OFF/" "$DEST"
fi

# Read back what is actually in the installed file, whether or not it was edited.
GOT=$(grep -c "^local DISABLE_AFTER = $LUAUI_OFF\$" "$DEST" || true)

if [ "$GOT" != 1 ]; then
	echo "install-probe: DISABLE_AFTER is not $LUAUI_OFF in $DEST, found:"
	grep -n "^local DISABLE_AFTER" "$DEST" || echo "  no DISABLE_AFTER line at all"
	exit 1
fi

if [ "$MOVE" != 0 ]; then
	sed -i '' "s/^local MOVE_AND_TRACK = 0\$/local MOVE_AND_TRACK = 1/" "$DEST"
fi

GOT=$(grep -c "^local MOVE_AND_TRACK = $MOVE\$" "$DEST" || true)

if [ "$GOT" != 1 ]; then
	echo "install-probe: MOVE_AND_TRACK is not $MOVE in $DEST, found:"
	grep -n "^local MOVE_AND_TRACK" "$DEST" || echo "  no MOVE_AND_TRACK line at all"
	exit 1
fi

echo "installed $DEST with DISABLE_AFTER = $LUAUI_OFF and MOVE_AND_TRACK = $MOVE"

if [ -n "$SHOTS" ]; then
	if [ -z "$GADGET_DIR" ]; then
		echo "no gadget directory under $GAME, cannot install the screenshot gadget"
		exit 1
	fi

	cp "$SHOT_SRC" "$SHOT_DEST"
	sed -i '' "s/^local SHOT_TIMES = {}\$/local SHOT_TIMES = {$SHOTS}/" "$SHOT_DEST"

	GOT=$(grep -c "^local SHOT_TIMES = {$SHOTS}\$" "$SHOT_DEST" || true)

	if [ "$GOT" != 1 ]; then
		echo "install-probe: SHOT_TIMES is not {$SHOTS} in $SHOT_DEST, found:"
		grep -n "^local SHOT_TIMES" "$SHOT_DEST" || echo "  no SHOT_TIMES line at all"
		exit 1
	fi

	echo "installed $SHOT_DEST with SHOT_TIMES = {$SHOTS}"
elif [ -e "$SHOT_DEST" ]; then
	rm "$SHOT_DEST"
	echo "removed $SHOT_DEST, no --shots given"
fi

if [ "$LOOPS" != 0 ]; then
	cp "$AMP_SRC" "$AMP_DEST"
	sed -i '' "s/^local LOOP_COUNT = 0\$/local LOOP_COUNT = $LOOPS/" "$AMP_DEST"

	GOT=$(grep -c "^local LOOP_COUNT = $LOOPS\$" "$AMP_DEST" || true)

	if [ "$GOT" != 1 ]; then
		echo "install-probe: LOOP_COUNT is not $LOOPS in $AMP_DEST, found:"
		grep -n "^local LOOP_COUNT" "$AMP_DEST" || echo "  no LOOP_COUNT line at all"
		exit 1
	fi

	if [ "$ALT" != 0 ]; then
		sed -i '' "s/^local ALT_COLOURS = 0\$/local ALT_COLOURS = 1/" "$AMP_DEST"
	fi

	GOT=$(grep -c "^local ALT_COLOURS = $ALT\$" "$AMP_DEST" || true)

	if [ "$GOT" != 1 ]; then
		echo "install-probe: ALT_COLOURS is not $ALT in $AMP_DEST, found:"
		grep -n "^local ALT_COLOURS" "$AMP_DEST" || echo "  no ALT_COLOURS line at all"
		exit 1
	fi

	echo "installed $AMP_DEST with LOOP_COUNT = $LOOPS and ALT_COLOURS = $ALT"
elif [ "$ALT" != 0 ]; then
	echo "--alt needs --loops, there is no amplifier to alternate"
	exit 1
elif [ -e "$AMP_DEST" ]; then
	rm "$AMP_DEST"
	echo "removed $AMP_DEST, no --loops given"
fi
