// Converted from ARB/dwDetailNormal.fp
// Blends 8 wave normal samples with per-wave weights to produce detail normals

#version 130

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;

// Per-wave blend weights (all rawBumpTexture[0] samples)
uniform vec4 env0;
uniform vec4 env1;
uniform vec4 env2;
uniform vec4 env3;
uniform vec4 env4;
uniform vec4 env5;
uniform vec4 env6;
uniform vec4 env7;

in vec2 texCoord0;
in vec2 texCoord1;
in vec2 texCoord2;
in vec2 texCoord3;
in vec2 texCoord4;
in vec2 texCoord5;
in vec2 texCoord6;
in vec2 texCoord7;

void main() {
	vec4 result = texture2D(tex0, texCoord0) * env0;
	result     += texture2D(tex1, texCoord1) * env1;
	result     += texture2D(tex2, texCoord2) * env2;
	result     += texture2D(tex3, texCoord3) * env3;
	result     += texture2D(tex4, texCoord4) * env4;
	result     += texture2D(tex5, texCoord5) * env5;
	result     += texture2D(tex6, texCoord6) * env6;
	result     += texture2D(tex7, texCoord7) * env7;

	gl_FragColor = result;
}
