// Converted from ARB/waterRefractT2D.fp
// Reflective+refractive water: bump perturbation + sub-surface colour

#version 130

uniform sampler2D subSurfTex;     // texture[2] = sub-surface (power-of-two)
uniform sampler2D reflectTex;     // texture[0] = reflectTexture
uniform sampler2D bumpTex;        // texture[1] = bumpTexture
uniform sampler2D waterDepthTex;  // texture[3] = map shading (alpha = water depth)

// Camera basis rotation (same as AdvWater)
uniform vec4 env0;  // {forward.z,  forward.x, 0, 0}
uniform vec4 env1;  // {-forward.x, forward.z, 0, 0}

// env2 = {10, 10, 0, 0}         refraction strength scale
// env3 = {1/pow2Width, 1/pow2Height, 0, 0}  normalise screen position to [0,1]
uniform vec4 env2;
uniform vec4 env3;

in vec2 reflectCoord;
in vec2 bumpCoord;
in vec2 waterDepthCoord;
in vec4 vertColor;

void main() {
	// Bump sample and bias
	vec4 bump = texture2D(bumpTex, bumpCoord) + vec4(-0.5, -0.5, 0.0, 0.0);
	float tx  = dot(bump, env0);
	float ty  = dot(bump, env1);

	// Reflection lookup (same as AdvWater)
	vec2 perturbedTC = vec2(tx, ty) * vec2(0.02) + reflectCoord;
	vec4 reflColor   = texture2D(reflectTex, perturbedTC) * vertColor;

	// Water depth from shading texture alpha (depth encoded as 255+groundHeight)
	vec4 waterDepth = texture2D(waterDepthTex, waterDepthCoord);

	// Subsurface refraction offset proportional to depth and view-space w
	float depthFactor = (1.0 - waterDepth.a) * gl_FragCoord.w;
	vec2 offset       = vec2(tx, ty) * env2.xy * depthFactor;

	// Normalise fragment position to [0,1] then add perturbation
	vec2 ssTC = gl_FragCoord.xy * env3.xy + offset;
	vec4 subsurfColor = texture2D(subSurfTex, ssTC);

	// Deeper water → darker/bluer subsurface
	float blend = waterDepth.a * 0.5 + 0.4;
	gl_FragColor = mix(reflColor, subsurfColor, blend);
}
