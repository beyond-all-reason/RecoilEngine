#version 330

// Depth copy into the window for when a blit cannot (d32fs8, or a window depth format the driver will not blit into). No stencil.

uniform sampler2D sceneDepth;

void main() {
	gl_FragDepth = texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r;
}
