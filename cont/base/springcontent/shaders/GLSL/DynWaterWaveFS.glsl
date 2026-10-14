// Converted from ARB/waterDynWave.fp
// Wave height update: propagates water height using a wave equation stencil

#version 130

uniform sampler2D tex0;  // waveTex3 (old height state)
uniform sampler2D tex1;  // waveTex1 (flow/velocity)
uniform sampler2D tex2;  // waveTex1 (neighbour -diag)
uniform sampler2D tex3;  // waveTex1 (neighbour +vert)
uniform sampler2D tex4;  // waveTex1 (neighbour +diag)
uniform sampler2D tex5;  // waveTex1 (neighbour +horiz)
uniform sampler2D tex6;  // detailNormalTex (background wave)

in vec2 texCoord0;
in vec2 texCoord1;
in vec2 texCoord2;
in vec2 texCoord3;
in vec2 texCoord4;
in vec2 texCoord5;
in vec2 texCoord6;

void main() {
	vec4 old        = texture2D(tex0, texCoord0);
	vec4 midFlow    = texture2D(tex1, texCoord1);
	vec4 flow1      = texture2D(tex2, texCoord2);
	vec4 flow2      = texture2D(tex3, texCoord3);
	vec4 flow3      = texture2D(tex4, texCoord4);
	vec4 flow4      = texture2D(tex5, texCoord5);
	vec4 bgWave     = texture2D(tex6, texCoord6);

	// Height component stores (height - prevHeight) packed as (delta, _, _, _)
	old.x -= old.z;

	float sumMid = dot(midFlow, vec4(1.0));
	old.x -= sumMid;

	// Gather flow contributions from each neighbour's respective channel
	float flow = flow1.x + flow2.y + flow3.z + flow4.w;
	old.x += flow;

	// Turbulence energy from flow divergence
	vec4 diffFlow = midFlow - vec4(flow1.x, flow2.y, flow3.z, flow4.w);
	float turbulence = dot(diffFlow, diffFlow);
	old.y = clamp((old.y + turbulence * 0.2) * 0.99, 0.0, 1.0);

	// Foam: blend with background wave at extremes
	float foamFactor = min(old.w, 2.0) * 0.2;
	foamFactor = clamp(foamFactor, 0.0, 1.0);

	old.x = mix(old.x, bgWave.z, 0.005);
	old.x = mix(old.x, -old.w, foamFactor);

	gl_FragColor = old * vec4(1.0, 1.0, 0.0, 0.0);
}
