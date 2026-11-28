#version 330

// The one display pass: decode, exposure, one curve, one sRGB encode.
// Uniform values match Display::Transform (DisplayTransforms.h).

uniform sampler2D sceneColor;
// 1 where the colour is scene-linear and is tone mapped; everything else is already display-referred
uniform sampler2D sceneMask;
uniform int useSceneMask; // 0 only in the transform test
uniform int displayTransform;
uniform float exposureLinear;
uniform float ditherAmount;
uniform float ditherSeed;

out vec4 fragColor;

// DisplayCurves.glsl is prepended at load (SceneTarget.cpp).
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

	vec3 outColor = DitherDisplay(ApplyDisplayCurve(lin, displayTransform), gl_FragCoord.xy, ditherSeed, ditherAmount);

	fragColor = vec4(outColor, stored.a);
}
