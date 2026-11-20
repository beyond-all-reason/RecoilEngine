/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "System/float3.h"

/**
 * @brief Precomputed trajectories of trajectoryHeight missiles, for line-of-fire checks.
 *
 * Holds the paths of a grid of stationary-target shots: distance as a fraction of
 * range, and target height as a slope of the horizontal distance. Each path is
 * stored as heights above the straight line to its target; a sample blends the
 * four nearest shots.
 */
class TrajectoryTable
{
public:
	static constexpr uint32_t NUM_SAMPLES = 16;

	/**
	 * @brief traces one shot in its vertical plane, from the origin towards (xzDist, rise),
	 * filling path with positions (x horizontal distance, y height); a path that ends
	 * short of the target is joined straight to it
	 */
	using TrajectoryFunc = std::function<void(float xzDist, float rise, std::vector<float3>& path)>;

	void Build(float range, const TrajectoryFunc& getTrajectory);
	bool Covers(float xzDist, float rise) const;

	/**
	 * @brief fill the interior samples [1, NUM_SAMPLES - 1] of a shot's path,
	 * as horizontal distance and height relative to its start
	 */
	void Sample(float xzDist, float rise, float* dist, float* height) const;
	/**
	 * @brief Resample a traced path to the same evenly spaced points Build produces.
	 */
	static void TraceToSample(const std::vector<float3>& path, float xzDist, float rise, float* dist, float* height);

private:
	static constexpr std::array<float, 4> DIST_FRACS = {0.25f, 0.5f, 0.75f, 1.0f};
	static constexpr std::array<float, 5> SLOPES = {-0.6f, -0.3f, 0.0f, 0.3f, 0.6f};
	static constexpr uint32_t NUM_HEIGHTS = NUM_SAMPLES - 1;

	float range = 0.0f;

	// NUM_HEIGHTS heights per shot, as fractions of its horizontal distance; shots by distance, then slope
	std::vector<float> heights;
};
