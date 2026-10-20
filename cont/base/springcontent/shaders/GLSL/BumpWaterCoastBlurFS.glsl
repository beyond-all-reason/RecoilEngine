#version 130

/**
 * @project Spring RTS
 * @file bumpWaterCoastBlurFS.glsl
 * @brief Input is a 0/1 bitmap and 1 indicates land.
 *        Now this shader blurs this map, so you get something like
 *        a distance to land map (->coastmap).
 *        The first pass (radius 0) creates the input from the heightmap.
 * @author jK
 *
 * Copyright (C) 2008,2009.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

in vec4 vTexCoord;

uniform sampler2D tex0; //! final (fullsize) texture
uniform sampler2D tex1; //! scratch texture with the to be updated rects
uniform sampler2D heightTex; //! corner heightmap, read by the first pass
uniform ivec2     args;

#define res 15.0
#define renderToScratch (args.x > 0.5)
#define radius args.y

out vec4 fragColor;

const float kernel = 1.0 / 10.0;

vec2 texelScissor = vec2(dFdx(vTexCoord.p), dFdy(vTexCoord.q)); // heightmap pos
vec2 texel0 = vec2(dFdx(vTexCoord.s), dFdy(vTexCoord.t)); // 0..1

vec4 tex2D(vec2 offset) {
	if (renderToScratch) {
		return texture2D(tex0, vTexCoord.st + offset * texel0);
	} else {
		vec2 scissor = vTexCoord.pq + (offset * texelScissor);
		bool outOfRectBound = any(greaterThan(scissor,vec2(1.0))) || any(lessThan(scissor,vec2(0.0)));
		if (outOfRectBound) {
			return texture2D(tex1, vTexCoord.st);
		} else {
			return texture2D(tex1, vTexCoord.st + offset * texel0);
		}
	}
}

vec2 getDistRect(float d, vec2 offset) {
	vec2 dist;
	float minDist = res - d * res;
	dist.x = floor(minDist);
	float iDist = dist.x * dist.x;
	minDist *= minDist;
	dist.y = sqrt(minDist - iDist);
	dist += offset;
	return dist;
}

float sqlength(vec2 v) {
	return dot(v, v);
}


void LoopIter(inout float maxDist, inout vec3 minDist, float i) {
	// 0____1____2
	// |         |
	// |         |
	// 3    x    4
	// |         |
	// |         |
	// 5____6____7

	vec4 v1, v2;
		v1.x = tex2D(vec2(-i,  radius)).g;
		v1.y = tex2D(vec2( i,  radius)).g;
		v1.z = tex2D(vec2(-i, -radius)).g;
		v1.w = tex2D(vec2( i, -radius)).g;

		v2.x = tex2D(vec2( radius,  i)).g;
		v2.y = tex2D(vec2( radius, -i)).g;
		v2.z = tex2D(vec2(-radius,  i)).g;
		v2.w = tex2D(vec2(-radius, -i)).g;

	v1    = max(v1,    v2   );
	v1.xy = max(v1.xy, v1.zw);
	v1.x  = max(v1.x,  v1.y );
	v1.x  = max(v1.x,  maxDist );

	vec2 dist = getDistRect(v1.x, vec2(radius, i));
	//minDist.z = min(minDist.z, sqlength(dist));

	if (sqlength(dist) < minDist.z) {
		maxDist = v1.x;
		minDist = vec3(dist, sqlength(dist));
	}
}


void main() {
	if (radius < 0.5) {
		//! initialize: r = is ground, g = is above water (blurred into the coast distance below),
		//! b = water depth in the encoding of CReadMap::EncodeHeight; one texel per heightmap corner
		float height = texelFetch(heightTex, ivec2(gl_FragCoord.xy), 0).r;
		fragColor = vec4(
			float(height > 10.0),
			float(height >  0.0),
			(height < 0.0)? (float(max(0, 255 + int(10.0 * height))) / 255.0): 1.0,
			0.0
		);
		return;
	}

	if (radius > 9.5) {
		//! blur the texture in the final stage
		vec2
			groundSurrounding  = tex2D(vec2( 1.0, 1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0, 1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0,-1.0)).rb;
			groundSurrounding += tex2D(vec2( 1.0,-1.0)).rb;

		fragColor = texture2D(tex1, vTexCoord.st);

		if (groundSurrounding.x + fragColor.r == 5.0) {
			fragColor.r = 1.0;
		} else {
			fragColor.r = 0.93 - (groundSurrounding.y + fragColor.b) / 5.0;
		}

		return;
	} else if (radius > 8.5) {
		//! blur the texture in the final stage
		vec2
			blur  = texture2D(tex0, vTexCoord.st + vec2( 1.0, 1.0) * texel0).rg;
			blur += texture2D(tex0, vTexCoord.st + vec2(-1.0, 1.0) * texel0).rg;
			blur += texture2D(tex0, vTexCoord.st + vec2(-1.0,-1.0) * texel0).rg;
			blur += texture2D(tex0, vTexCoord.st + vec2( 1.0,-1.0) * texel0).rg;

		fragColor = texture2D(tex0, vTexCoord.st);
		fragColor.r = step(5.0, blur.x + fragColor.r);
		fragColor.g = mix(fragColor.g, blur.y * 0.25, 0.4);
		return;
	}

	vec4 center;
	if (renderToScratch) {
		center = texture2D(tex0, vTexCoord.st);
	} else {
		center = texture2D(tex1, vTexCoord.st);
	}

	//! the search below can only raise g, texels already at 1.0 (land) keep their value
	if (center.g >= 1.0) {
		fragColor = center;
		return;
	}

	float maxValue = 0.0;
	vec3 minDist = vec3(1e9);

	// driver fails at unrolling when count is not known
	// at compile-time, so we do it manually (radius is
	// in [0.5, 8.5], so we need at most 8 iterations)
	float iter = 0.0;
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }

	//! PROCESS maxValue
	//if (maxValue == 0.0)
	//	discard;

	float fDist = 1.0 - (min(res, sqrt(minDist.z)) / res);

	fragColor = center;
	fragColor.g = max(fragColor.g, fDist * fDist * fDist);
}
