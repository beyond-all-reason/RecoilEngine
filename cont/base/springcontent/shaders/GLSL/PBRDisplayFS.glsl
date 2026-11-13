#version 330

// The one PBR display pass (planning M1, increment I6): decode the scene
// target's storage transfer, apply the manual exposure, run exactly one curve
// and encode once for an sRGB display. The CPU reference with the same
// functions is rts/Rendering/GL/DisplayTransforms_pbr.h; change both together.
// Uniform values match the C++ enums PBRDisplay::Transform and SceneTransfer.

uniform sampler2D sceneColor;
// 1 where a PBR model was drawn last (SceneTarget.h); everything else holds display-referred colour already
uniform sampler2D sceneMask;
uniform int useSceneMask; // 0 only in the transform test, whose ramp is all PBR light
uniform int displayTransform;
uniform int sceneTransfer;
uniform float exposureLinear;
uniform float ditherAmount; // PBRDisplayDither: 1/255 (unreal) or 0 (off)
uniform float ditherSeed;   // changes every frame

out vec4 fragColor;

// Prepended at load (SceneTarget.cpp): PBRDisplayCurves.glsl, which defines TM_*, SRGBDecode, SRGBEncode and
// PBRApplyDisplayTransform.
const int TRANSFER_SRGB_EXTENDED = 0;

void main() {
	vec4 stored = texelFetch(sceneColor, ivec2(gl_FragCoord.xy), 0);

	if (displayTransform == TM_LEGACY) {
		fragColor = stored;
		return;
	}
	// legacy models, terrain, water and sky: today's direct value, untouched (no curve, exposure or grain)
	if (useSceneMask == 1 && texelFetch(sceneMask, ivec2(gl_FragCoord.xy), 0).r < 0.5) {
		fragColor = stored;
		return;
	}

	vec3 lin = (sceneTransfer == TRANSFER_SRGB_EXTENDED) ? SRGBDecode(stored.rgb) : stored.rgb;
	lin *= exposureLinear;

	vec3 outColor = PBRDitherDisplay(PBRApplyDisplayTransform(lin, displayTransform), gl_FragCoord.xy, ditherSeed, ditherAmount);

	// alpha passes through so window captures keep today's alpha
	fragColor = vec4(outColor, stored.a);
}
