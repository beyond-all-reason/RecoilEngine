// Converted from ARB/water.fp
// Reflective water: bump-perturbed reflection lookup

#version 130

uniform sampler2D reflectTex; // texture[0] = reflectTexture
uniform sampler2D bumpTex;    // texture[1] = bumpTexture

// Camera forward direction (2D rotation for reflection alignment)
// env[0] = {forward.z,  forward.x, 0, 0}
// env[1] = {-forward.x, forward.z, 0, 0}
uniform vec4 env0;
uniform vec4 env1;

in vec2 reflectCoord;
in vec2 bumpCoord;
in vec4 vertColor;

void main() {
	// Sample bump texture and bias to [-0.5, 0.5]
	vec4 bump = texture2D(bumpTex, bumpCoord) + vec4(-0.5, -0.5, 0.0, 0.0);

	// Rotate bump in camera-forward space:
	//   tx = dot(bump, env0)
	//   ty = dot(bump, env1)
	float tx = dot(bump, env0);
	float ty = dot(bump, env1);

	// Apply small bump offset to reflection texcoord
	vec2 perturbedTC = vec2(tx, ty) * vec2(0.02, 0.02) + reflectCoord;
	vec4 reflColor   = texture2D(reflectTex, perturbedTC);

	// Modulate alpha by bump y-component for edge fade
	vec4 color = vertColor;
	color.a   += ty * 0.03;

	gl_FragColor = reflColor * color;
}
