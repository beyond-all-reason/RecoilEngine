/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#version 150 compatibility

in vec3 templatePosition;
in vec3 templateNormal;
in vec2 templateGlowUV;
in float templateIsGlow;
in vec3 particlePos;
in vec3 particleVelocity;
in vec4 particleLifetime;
in vec3 particleAparams;
in vec4 particleColor;

uniform float animationFrame;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform vec4 clipPlane;

out vec4 g_color;
out vec3 g_normal;
out vec3 g_worldPos;
out vec3 g_localPos;
out vec3 g_noiseSeed;
out vec2 g_glowUV;
out float g_isGlow;
out float g_seed;
out float gl_ClipDistance[1];

const float DRAW_RADIUS = 1.5;
const float SIZE_VARIATION = 0.3;
const float GLOW_SCALE = 11.0;
const float BASE_ALPHA = 50.0 / 255.0;
const float ALPHA_VARIATION = 2.5;
const float FADE_FRAMES = 4.0;
const float GAME_SPEED = 30.0;

float hash11(float value)
{
	return fract(sin(value) * 43758.5453);
}

vec3 equalizeColor(vec3 color)
{
	const float equalize = 0.7;
	const float targetLuma = 0.55;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	if (luma < 0.001)
		return color;

	color *= pow(targetLuma / luma, equalize);
	float maxChannel = max(color.r, max(color.g, color.b));
	if (maxChannel > 1.0)
		color /= maxChannel;
	return color;
}

mat3 rotXYZ(vec3 angle)
{
	float cx = cos(angle.x), sx = sin(angle.x);
	float cy = cos(angle.y), sy = sin(angle.y);
	float cz = cos(angle.z), sz = sin(angle.z);
	mat3 rotateX = mat3(1, 0, 0, 0, cx, sx, 0, -sx, cx);
	mat3 rotateY = mat3(cy, 0, -sy, 0, 1, 0, sy, 0, cy);
	mat3 rotateZ = mat3(cz, sz, 0, -sz, cz, 0, 0, 0, 1);
	return rotateZ * rotateY * rotateX;
}

void main()
{
	float createFrame = particleLifetime.x;
	float deathFrame = particleLifetime.y;
	if (animationFrame >= deathFrame) {
		g_color = vec4(0.0);
		g_normal = vec3(0.0);
		g_worldPos = vec3(0.0);
		g_localPos = vec3(0.0);
		g_noiseSeed = vec3(0.0);
		g_glowUV = vec2(0.0);
		g_isGlow = 0.0;
		g_seed = 0.0;
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
		gl_ClipDistance[0] = 1.0;
		return;
	}

	float age = max(animationFrame - createFrame, 0.0);
	float ballisticAge = age * particleAparams.x;
	float fade = clamp((deathFrame - animationFrame) / FADE_FRAMES, 0.0, 1.0);
	vec3 center = particlePos + particleVelocity * ballisticAge;

	float particleHash = dot(particleVelocity, vec3(12.9898, 78.233, 37.719)) + createFrame * 0.6180339;
	vec3 randomValues = vec3(
		hash11(particleHash + 1.7),
		hash11(particleHash + 3.3),
		hash11(particleHash + 5.9)
	);

	float sizeMultiplier = 1.0 + SIZE_VARIATION * (hash11(particleHash + 7.1) * 2.0 - 1.0);
	float size = DRAW_RADIUS * sizeMultiplier * (0.5 + 0.5 * fade);
	float alpha = BASE_ALPHA * (1.0 + ALPHA_VARIATION * (hash11(particleHash + 11.3) * 2.0 - 1.0));
	g_color = vec4(equalizeColor(particleColor.rgb), max(alpha, 0.0) * fade);
	g_seed = randomValues.x * 6.2831853;

	vec3 worldOffset;
	if (templateIsGlow > 0.5) {
		worldOffset = (cameraRight * templateGlowUV.x + cameraUp * templateGlowUV.y) * (size * GLOW_SCALE);
		g_normal = vec3(0.0, 1.0, 0.0);
		g_localPos = vec3(0.0);
		g_noiseSeed = vec3(0.0);
		g_glowUV = templateGlowUV;
		g_isGlow = 1.0;
	} else {
		float rotValue = mix(-180.0, 180.0, hash11(particleHash + 13.7));
		float rotVelocity = mix(-40.0, 40.0, hash11(particleHash + 17.9)) / GAME_SPEED;
		float rotation = radians(rotValue + rotVelocity * age);
		vec3 phase = randomValues * 6.2831853;
		mat3 rotationMatrix = rotXYZ(phase + vec3(rotation, rotation * 1.3, rotation * 0.7));
		worldOffset = rotationMatrix * (templatePosition * size);
		g_normal = rotationMatrix * templateNormal;
		g_localPos = worldOffset;
		g_noiseSeed = randomValues * (360.0 * 137.0) + vec3(11.0, 47.0, 83.0);
		g_glowUV = vec2(0.0);
		g_isGlow = 0.0;
	}

	g_worldPos = center + worldOffset;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(g_worldPos, 1.0);
	gl_ClipDistance[0] = dot(vec4(g_worldPos, 1.0), clipPlane);
}
