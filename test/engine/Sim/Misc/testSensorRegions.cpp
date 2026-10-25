/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <catch_amalgamated.hpp>
#include <vector>

#include "Sim/Misc/RegionPartitions.h"

TEST_CASE("Sensor partitions reject overlapping, unaligned and out-of-map coverage")
{
	std::vector<SRectangle> regions{{0, 0, 2048, 4096}, {3072, 0, 5120, 4096}};
	CHECK(ValidRegionPartitions(regions, 5120, 4096, 64));
	CHECK(ValidRegionPartitions({}, 5120, 4096, 64));
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 0));

	regions[1] = {1984, 0, 5120, 4096};
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 64));
	regions[1] = {2048, 0, 5120, 4096};
	CHECK(ValidRegionPartitions(regions, 5120, 4096, 64)); // touching half-open edges
	regions[1] = {2049, 0, 5120, 4096};
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 64));
	regions[1] = {-64, 0, 1024, 4096};
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 64));
	regions[1] = {3072, 0, 5184, 4096};
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 64));
	regions[1] = {3072, 0, 3072, 4096};
	CHECK_FALSE(ValidRegionPartitions(regions, 5120, 4096, 64));
}

TEST_CASE("Region membership uses half-open coordinates independently of sensor alignment")
{
	const std::vector<SRectangle> regions{{0, 0, 2048, 4096}, {2560, 0, 5120, 4096}};
	CHECK(ValidRegionPartitions(regions, 5120, 4096, 8));
	CHECK(FindRegionPartition(regions, 0, 0) == &regions[0]);
	CHECK(FindRegionPartition(regions, 2048, 0) == nullptr);
	CHECK(FindRegionPartition(regions, 2304, 1000) == nullptr);
	CHECK(FindRegionPartition(regions, 2560, 0) == &regions[1]);
	CHECK(FindRegionPartition(regions, 2560, 4096) == nullptr);
}
