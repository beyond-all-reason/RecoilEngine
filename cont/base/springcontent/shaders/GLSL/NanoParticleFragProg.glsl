/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#version 150 compatibility

uniform float animationFrame;
uniform vec3 cameraPos;

in vec4 g_color;
in vec3 g_normal;
in vec3 g_worldPos;
in vec3 g_localPos;
in vec3 g_noiseSeed;
in vec2 g_glowUV;
in float g_isGlow;
in float g_seed;

out vec4 fragColor;

const float CUBE_SHOW_INSIDE = 4.0;
const float CUBE_NOISE = 6.0;
const float CUBE_NOISE_SPEED = 25.0;
const float CUBE_NOISE_SCALE = 1.75;
const float GLOW_INTENSITY = 0.35;
const float GLOW_FALLOFF = 9.5;
const float CORE_BOOST = 0.3;
const float HUE_JITTER = 0.1;
const float WHITE_HOTSPOT = 1.5;
const float WHITE_HOTSPOT_THRESHOLD = 0.6;

float hash13(vec3 value)
{
	value = fract(value * 0.1031);
	value += dot(value, value.zyx + 31.32);
	return fract((value.x + value.y) * value.z);
}

float valueNoise3(vec3 value)
{
	vec3 cell = floor(value);
	vec3 fraction = fract(value);
	fraction = fraction * fraction * fraction * (fraction * (fraction * 6.0 - 15.0) + 10.0);

	float n000 = hash13(cell + vec3(0, 0, 0));
	float n100 = hash13(cell + vec3(1, 0, 0));
	float n010 = hash13(cell + vec3(0, 1, 0));
	float n110 = hash13(cell + vec3(1, 1, 0));
	float n001 = hash13(cell + vec3(0, 0, 1));
	float n101 = hash13(cell + vec3(1, 0, 1));
	float n011 = hash13(cell + vec3(0, 1, 1));
	float n111 = hash13(cell + vec3(1, 1, 1));
	vec4 xMix = mix(vec4(n000, n010, n001, n011), vec4(n100, n110, n101, n111), fraction.x);
	vec2 yMix = mix(xMix.xz, xMix.yw, fraction.y);
	return mix(yMix.x, yMix.y, fraction.z);
}

void main()
{
	vec3 tint = vec3(1.0) + HUE_JITTER * vec3(
		sin(g_seed),
		sin(g_seed + 2.094),
		sin(g_seed + 4.188)
	);

	if (g_isGlow > 0.5) {
		float radialDistance = length(g_glowUV);
		if (radialDistance > 1.0)
			discard;

		float glow = pow(clamp(1.0 - radialDistance, 0.0, 1.0), GLOW_FALLOFF) * GLOW_INTENSITY;
		vec3 glowTint = g_color.rgb / max(max(g_color.r, max(g_color.g, g_color.b)), 0.001);
		float glowLuma = dot(glowTint, vec3(0.2126, 0.7152, 0.0722));
		float glowBoost = min(0.55 / max(glowLuma, 0.001), 5.0);
		fragColor = vec4(glowTint * tint * (glow * glowBoost), g_color.a * glow);
		return;
	}

	vec3 normal = normalize(g_normal);
	vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.25));
	float directionalShade = 0.85 + 0.15 * max(dot(normal, lightDirection), 0.0);
	float shade = directionalShade;
	float alphaMultiplier = 1.0;

	vec3 viewDirection = normalize(cameraPos - g_worldPos);
	float normalDotView = dot(normal, viewDirection);
	float shade3D;
	float alpha3D;
	if (normalDotView >= 0.0) {
		shade3D = 0.80 + 0.45 * (1.0 - normalDotView);
		alpha3D = 1.0;
	} else {
		shade3D = 0.30 + 0.30 * (-normalDotView);
		alpha3D = 0.55;
	}
	shade = mix(directionalShade, directionalShade * shade3D, CUBE_SHOW_INSIDE);
	alphaMultiplier = mix(1.0, alpha3D, CUBE_SHOW_INSIDE);

	float noiseTime = animationFrame * CUBE_NOISE_SPEED * (1.0 / 30.0);
	vec3 noisePosition = g_localPos * CUBE_NOISE_SCALE + g_noiseSeed + vec3(noiseTime, noiseTime * 0.7, noiseTime * 1.3);
	float noiseValue = valueNoise3(noisePosition);
	shade *= 1.0 + CUBE_NOISE * (noiseValue * 2.0 - 1.0);

	vec3 baseColor = g_color.rgb * tint * shade * CORE_BOOST;
	float hotspot = smoothstep(WHITE_HOTSPOT_THRESHOLD, 1.0, noiseValue) * WHITE_HOTSPOT;
	baseColor = mix(baseColor, vec3(1.0) * max(shade, 0.6), hotspot);

	fragColor = vec4(baseColor, g_color.a * alphaMultiplier);
}