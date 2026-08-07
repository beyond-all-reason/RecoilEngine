/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#version 150 compatibility

layout(points) in;
layout(triangle_strip, max_vertices = 28) out;

uniform float animationFrame;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform vec4 clipPlane;

in vec3 v_velocity[];
in vec2 v_lifetime[];
in vec4 v_color[];

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

void emitShapeVertex(
	vec3 center,
	vec3 worldOffset,
	vec3 localPos,
	vec3 normal,
	vec4 color,
	vec3 noiseSeed,
	vec2 glowUV,
	float isGlow,
	float seed
)
{
	g_color = color;
	g_normal = normal;
	g_worldPos = center + worldOffset;
	g_localPos = localPos;
	g_noiseSeed = noiseSeed;
	g_glowUV = glowUV;
	g_isGlow = isGlow;
	g_seed = seed;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(g_worldPos, 1.0);
	gl_ClipDistance[0] = dot(vec4(g_worldPos, 1.0), clipPlane);
	EmitVertex();
}

void emitFace(
	vec3 corner0,
	vec3 corner1,
	vec3 corner2,
	vec3 corner3,
	vec3 normal,
	vec3 center,
	vec4 color,
	vec3 noiseSeed,
	float seed
)
{
	emitShapeVertex(center, corner0, corner0, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner1, corner1, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner2, corner2, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	emitShapeVertex(center, corner3, corner3, normal, color, noiseSeed, vec2(0.0), 0.0, seed);
	EndPrimitive();
}

void emitGlow(vec3 center, vec4 color, float halfSize, float seed)
{
	vec3 right = cameraRight * halfSize;
	vec3 up = cameraUp * halfSize;
	vec3 normal = vec3(0.0, 1.0, 0.0);
	vec3 noiseSeed = vec3(0.0);

	emitShapeVertex(center, -right - up, vec3(0.0), normal, color, noiseSeed, vec2(-1.0, -1.0), 1.0, seed);
	emitShapeVertex(center,  right - up, vec3(0.0), normal, color, noiseSeed, vec2( 1.0, -1.0), 1.0, seed);
	emitShapeVertex(center, -right + up, vec3(0.0), normal, color, noiseSeed, vec2(-1.0,  1.0), 1.0, seed);
	emitShapeVertex(center,  right + up, vec3(0.0), normal, color, noiseSeed, vec2( 1.0,  1.0), 1.0, seed);
	EndPrimitive();
}

void main()
{
	vec3 center = gl_in[0].gl_Position.xyz;
	float createFrame = v_lifetime[0].x;
	float deathFrame = v_lifetime[0].y;
	float age = max(animationFrame - createFrame, 0.0);
	float fade = clamp((deathFrame - animationFrame) / FADE_FRAMES, 0.0, 1.0);

	float particleHash = dot(v_velocity[0], vec3(12.9898, 78.233, 37.719)) + createFrame * 0.6180339;
	vec3 randomValues = vec3(
		hash11(particleHash + 1.7),
		hash11(particleHash + 3.3),
		hash11(particleHash + 5.9)
	);

	float sizeMultiplier = 1.0 + SIZE_VARIATION * (hash11(particleHash + 7.1) * 2.0 - 1.0);
	float size = DRAW_RADIUS * sizeMultiplier * (0.5 + 0.5 * fade);
	float alpha = BASE_ALPHA * (1.0 + ALPHA_VARIATION * (hash11(particleHash + 11.3) * 2.0 - 1.0));
	alpha = max(alpha, 0.0) * fade;
	vec4 color = vec4(equalizeColor(v_color[0].rgb), alpha);

	float rotValue = mix(-180.0, 180.0, hash11(particleHash + 13.7));
	float rotVelocity = mix(-40.0, 40.0, hash11(particleHash + 17.9)) / GAME_SPEED;
	float rotation = radians(rotValue + rotVelocity * age);
	vec3 phase = randomValues * 6.2831853;
	mat3 rotationMatrix = rotXYZ(phase + vec3(rotation, rotation * 1.3, rotation * 0.7));

	vec3 noiseSeed = randomValues * (360.0 * 137.0) + vec3(11.0, 47.0, 83.0);
	float seed = randomValues.x * 6.2831853;

	vec3 xAxis = rotationMatrix * vec3(size, 0.0, 0.0);
	vec3 yAxis = rotationMatrix * vec3(0.0, size, 0.0);
	vec3 zAxis = rotationMatrix * vec3(0.0, 0.0, size);
	vec3 normalX = rotationMatrix[0];
	vec3 normalY = rotationMatrix[1];
	vec3 normalZ = rotationMatrix[2];

	emitFace( xAxis-yAxis-zAxis,  xAxis+yAxis-zAxis,  xAxis-yAxis+zAxis,  xAxis+yAxis+zAxis,  normalX, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis, -xAxis-yAxis+zAxis, -xAxis+yAxis-zAxis, -xAxis+yAxis+zAxis, -normalX, center, color, noiseSeed, seed);
	emitFace(-xAxis+yAxis-zAxis, -xAxis+yAxis+zAxis,  xAxis+yAxis-zAxis,  xAxis+yAxis+zAxis,  normalY, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis,  xAxis-yAxis-zAxis, -xAxis-yAxis+zAxis,  xAxis-yAxis+zAxis, -normalY, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis+zAxis,  xAxis-yAxis+zAxis, -xAxis+yAxis+zAxis,  xAxis+yAxis+zAxis,  normalZ, center, color, noiseSeed, seed);
	emitFace(-xAxis-yAxis-zAxis, -xAxis+yAxis-zAxis,  xAxis-yAxis-zAxis,  xAxis+yAxis-zAxis, -normalZ, center, color, noiseSeed, seed);

	emitGlow(center, color, size * GLOW_SCALE, seed);
}