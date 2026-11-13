#version 330

// The one PBR display pass (planning M1, increment I6): decode the scene
// target's storage transfer, apply the manual exposure, run exactly one curve
// and encode once for an sRGB display. The CPU reference with the same
// functions is rts/Rendering/GL/DisplayTransforms_pbr.h; change both together.
// Uniform values match the C++ enums PBRDisplay::Transform and SceneTransfer.

uniform sampler2D sceneColor;
uniform int displayTransform;
uniform int sceneTransfer;
uniform float exposureLinear;

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

	vec3 lin = (sceneTransfer == TRANSFER_SRGB_EXTENDED) ? SRGBDecode(stored.rgb) : stored.rgb;
	lin *= exposureLinear;

	vec3 outColor = PBRApplyDisplayTransform(lin, displayTransform);

	// alpha passes through so window captures keep today's alpha
	fragColor = vec4(outColor, stored.a);
}
