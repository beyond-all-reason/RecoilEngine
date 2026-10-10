// Converted from ARB/dwAddSplash.fp
// Modulates splash/wake texture by wake intensity data

#version 130

uniform sampler2D tex0;  // boatShape / hoverShape / splashTex

in vec2 texCoord0;
in vec4 wakeData;

void main() {
	vec4 texColor = texture2D(tex0, texCoord0);
	gl_FragColor  = texColor * wakeData;
}
