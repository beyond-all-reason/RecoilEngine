/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <utility>

#include <catch_amalgamated.hpp>

#include "Sim/Units/CommandAI/CommandDescription.h"

TEST_CASE("Command descriptions retain movement classification through copies and moves")
{
	SCommandDescription original;
	CHECK_FALSE(original.isMoveCommand);
	original.id = 34567;
	original.isMoveCommand = true;

	SCommandDescription copied(original);
	CHECK(copied.isMoveCommand);
	CHECK_FALSE(copied != original);

	SCommandDescription moved(std::move(copied));
	CHECK(moved.isMoveCommand);
	CHECK_FALSE(moved != original);

	SCommandDescription assigned;
	assigned = original;
	CHECK(assigned.isMoveCommand);
	assigned = SCommandDescription();
	CHECK_FALSE(assigned.isMoveCommand);
	CHECK(assigned != original);
}

TEST_CASE("Command description cache distinguishes movement classification")
{
	commandDescriptionCache.Init();
	SCommandDescription description;
	description.id = 34567;
	const auto* unmarked = commandDescriptionCache.GetPtr(SCommandDescription(description));
	description.isMoveCommand = true;
	const auto* marked = commandDescriptionCache.GetPtr(SCommandDescription(description));
	const auto* shared = commandDescriptionCache.GetPtr(SCommandDescription(description));

	CHECK(unmarked != marked);
	CHECK_FALSE(unmarked->isMoveCommand);
	CHECK(marked->isMoveCommand);
	CHECK(shared == marked);
	CHECK(marked->refCount == 2);

	commandDescriptionCache.DecRef(*shared);
	commandDescriptionCache.DecRef(*marked);
	commandDescriptionCache.DecRef(*unmarked);

	// Reusing a released cache slot must also reset the flag.
	const auto* reset = commandDescriptionCache.GetPtr(SCommandDescription());
	CHECK_FALSE(reset->isMoveCommand);
	commandDescriptionCache.DecRef(*reset);
}
