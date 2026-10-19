/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TrajectoryTable.h"
#include <algorithm>
#include "System/SpringMath.h"

void TrajectoryTable::Build(float range_, const TrajectoryFunc& getTrajectory)
{
	range = range_;
	heights.clear();
	heights.reserve(DIST_FRACS.size() * SLOPES.size() * NUM_HEIGHTS);

	std::vector<float3> path;

	for (const float distFrac: DIST_FRACS) {
		for (const float slope: SLOPES) {
			const float xzDist = range * distFrac;
			const float rise = xzDist * slope;

			path.clear();
			path.emplace_back();

			getTrajectory(xzDist, rise, path);
			path.emplace_back(xzDist, rise, 0.0f);

			// heights above the straight line to the target, interpolated along the path
			size_t j = 1;

			for (uint32_t i = 1; i < NUM_SAMPLES; i++) {
				const float x = xzDist * i / NUM_SAMPLES;

				while (j < (path.size() - 1) && path[j].x < x)
					j++;

				const float3& p0 = path[j - 1];
				const float3& p1 = path[j];
				const float t = (p1.x > p0.x)? std::clamp((x - p0.x) / (p1.x - p0.x), 0.0f, 1.0f): 1.0f;

				heights.push_back((mix(p0.y, p1.y, t) - rise * i / NUM_SAMPLES) / xzDist);
			}
		}
	}
}

void TrajectoryTable::Sample(float xzDist, float rise, float* dist, float* height) const
{
	// fractional cell position along each evenly spaced axis: whole part picks the cell, remainder blends across it
	const float distCell = (std::clamp(xzDist / range, DIST_FRACS.front(), DIST_FRACS.back()) - DIST_FRACS.front()) / (DIST_FRACS[1] - DIST_FRACS[0]);
	const float slopeCell = (std::clamp(rise / xzDist, SLOPES.front(), SLOPES.back()) - SLOPES.front()) / (SLOPES[1] - SLOPES[0]);

	const size_t di = std::min(static_cast<size_t>(distCell), DIST_FRACS.size() - 2);
	const size_t si = std::min(static_cast<size_t>(slopeCell), SLOPES.size() - 2);
	const float df = distCell - di;
	const float sf = slopeCell - si;

	const float* h00 = &heights[((di + 0) * SLOPES.size() + (si + 0)) * NUM_HEIGHTS];
	const float* h01 = &heights[((di + 0) * SLOPES.size() + (si + 1)) * NUM_HEIGHTS];
	const float* h10 = &heights[((di + 1) * SLOPES.size() + (si + 0)) * NUM_HEIGHTS];
	const float* h11 = &heights[((di + 1) * SLOPES.size() + (si + 1)) * NUM_HEIGHTS];

	for (uint32_t i = 1; i < NUM_SAMPLES; i++) {
		const uint32_t k = i - 1;
		const float h = mix(mix(h00[k], h01[k], sf), mix(h10[k], h11[k], sf), df);

		dist[i] = xzDist * i / NUM_SAMPLES;
		height[i] = rise * i / NUM_SAMPLES + xzDist * h;
	}
}
