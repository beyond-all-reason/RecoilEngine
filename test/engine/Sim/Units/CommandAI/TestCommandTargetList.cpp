/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandTargetList.h"

TEST_CASE("Attack target lists retain their ordered parameters")
{
	Command command(CMD_ATTACK_TARGETS);

	for (int targetID = 10; targetID < 30; ++targetID)
		command.PushParam(targetID);

	REQUIRE(command.GetNumParams() == 20);
	CHECK(command.IsPooledCommand());
	CHECK(command.IsAttackCommand());
	CHECK(command.IsMoveCommand());
	CHECK_FALSE(command.IsAreaCommand());

	int objectParam = -1;
	CHECK_FALSE(command.IsObjectCommand(objectParam));

	unsigned int firstSourceIndex = 0;
	unsigned int secondSourceIndex = 0;

	CHECK(CommandTargetList::NextTargetID(command, firstSourceIndex) == 10);
	CHECK(CommandTargetList::NextTargetID(command, firstSourceIndex) == 11);
	CHECK(CommandTargetList::NextTargetID(command, secondSourceIndex) == 10);

	for (int expectedID = 12; expectedID < 30; ++expectedID)
		CHECK(CommandTargetList::NextTargetID(command, firstSourceIndex) == expectedID);

	CHECK_FALSE(CommandTargetList::NextTargetID(command, firstSourceIndex).has_value());
	CHECK(CommandTargetList::NextTargetID(command, secondSourceIndex) == 11);
	CHECK(CommandTargetList::ContainsTarget(command, 10));
	CHECK(CommandTargetList::ContainsTarget(command, 29));
	CHECK_FALSE(CommandTargetList::ContainsTarget(command, 30));
}

TEST_CASE("Copied attack target lists retain independent traversal state")
{
	Command original(CMD_ATTACK_TARGETS);
	original.PushParam(101);
	original.PushParam(202);
	original.PushParam(303);

	const Command firstSourceCommand = original;
	const Command secondSourceCommand = original;
	unsigned int firstSourceIndex = 0;
	unsigned int secondSourceIndex = 0;

	CHECK(CommandTargetList::NextTargetID(firstSourceCommand, firstSourceIndex) == 101);
	CHECK(CommandTargetList::NextTargetID(firstSourceCommand, firstSourceIndex) == 202);
	CHECK(CommandTargetList::NextTargetID(secondSourceCommand, secondSourceIndex) == 101);

	firstSourceIndex = 0;
	CHECK(CommandTargetList::NextTargetID(firstSourceCommand, firstSourceIndex) == 101);
}

TEST_CASE("Only identical ordered target lists match")
{
	Command original(CMD_ATTACK_TARGETS);
	original.PushParam(101);
	original.PushParam(202);
	original.PushParam(303);

	Command identical = original;
	CHECK(CommandTargetList::HaveSameTargets(original, identical));

	Command reordered(CMD_ATTACK_TARGETS);
	reordered.PushParam(101);
	reordered.PushParam(303);
	reordered.PushParam(202);
	CHECK_FALSE(CommandTargetList::HaveSameTargets(original, reordered));

	Command shorter(CMD_ATTACK_TARGETS);
	shorter.PushParam(101);
	shorter.PushParam(202);
	CHECK_FALSE(CommandTargetList::HaveSameTargets(original, shorter));
}
