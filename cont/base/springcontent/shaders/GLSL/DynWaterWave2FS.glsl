// Converted from ARB/waterDynWave2.fp
// Wave flow update: updates per-pixel flow vectors using height differences

#version 130

uniform sampler2D tex0;  // waveTex2 (old flow state)
uniform sampler2D tex1;  // waveTex1 (centre height)
uniform sampler2D tex2;  // waveTex1 at -diag offset (neighbour heights)
uniform sampler2D tex3;  // waveTex1 at -vert offset
uniform sampler2D tex4;  // waveTex1 at +diag offset
uniform sampler2D tex5;  // waveTex1 at +horiz offset
uniform sampler2D tex6;  // map shading texture (ground height in alpha)

in vec2 texCoord0;
in vec2 texCoord1;
in vec2 texCoord2;
in vec2 texCoord3;
in vec2 texCoord4;
in vec2 texCoord5;

void main() {
	vec4 old       = texture2D(tex0, texCoord0);
	vec4 midHeight = texture2D(tex1, texCoord0);
	vec4 height1   = texture2D(tex2, texCoord1);
	vec4 height2   = texture2D(tex3, texCoord2);
	vec4 height3   = texture2D(tex4, texCoord3);
	vec4 height4   = texture2D(tex5, texCoord4);
	vec4 gheight   = texture2D(tex6, texCoord5);

	// Accelerate each flow component toward its neighbour's height
	old.x += (midHeight.x - height1.x) * 0.02;
	old.y += (midHeight.x - height2.x) * 0.02;
	old.z += (midHeight.x - height3.x) * 0.02;
	old.w += (midHeight.x - height4.x) * 0.02;

	// Damping: ground height (in alpha) controls whether water flows freely
	float damping = mix(0.998, 0.008, gheight.w);
	old *= damping;

	gl_FragColor = old;
}
