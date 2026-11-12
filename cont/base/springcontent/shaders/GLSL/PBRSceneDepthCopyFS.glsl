#version 330

// Copies the PBR scene target's resolved depth into the window's depth buffer
// when a blit cannot (PBRSceneDepthFormat=d32fs8, or a window depth format the
// driver will not blit into). Stencil is not copied by this path.

uniform sampler2D sceneDepth;

void main() {
	gl_FragDepth = texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r;
}
