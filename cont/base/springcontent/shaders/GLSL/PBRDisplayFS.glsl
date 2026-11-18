#version 330

// The one display pass: decode, exposure, one curve, one sRGB encode.
// Uniform values match PBRDisplay::Transform (DisplayTransforms.h).

uniform sampler2D sceneColor;
// 1 where a PBR model was drawn; everything else is already display-referred
uniform sampler2D sceneMask;
uniform int useSceneMask; // 0 only in the transform test
uniform int displayTransform;
uniform float exposureLinear;
uniform float ditherAmount;
uniform float ditherSeed;

out vec4 fragColor;

// PBRDisplayCurves.glsl is prepended at load (SceneTarget.cpp).
void main() {
	vec4 stored = texelFetch(sceneColor, ivec2(gl_FragCoord.xy), 0);

	if (displayTransform == TM_LEGACY) {
		fragColor = stored;
		return;
	}
	if (useSceneMask == 1 && texelFetch(sceneMask, ivec2(gl_FragCoord.xy), 0).r < 0.5) {
		fragColor = stored;
		return;
	}

	vec3 lin = SRGBDecode(stored.rgb) * exposureLinear;

	vec3 outColor = PBRDitherDisplay(PBRApplyDisplayTransform(lin, displayTransform), gl_FragCoord.xy, ditherSeed, ditherAmount);

	fragColor = vec4(outColor, stored.a);
}
