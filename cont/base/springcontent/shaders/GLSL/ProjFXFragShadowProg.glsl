#version 130

#ifdef USE_TEXTURE_ARRAY
	uniform sampler2DArray atlasTex;
#else
	uniform sampler2D      atlasTex;
#endif

uniform vec4 alphaCtrl = vec4(0.0, 1.0, 0.0, 0.0);  //always pass
uniform float shadowColorMode = 1.0;

in vec4 vCol;
in vec4 vUV;
in float vLayer; //for sampler2Darray (future)
in float vBF;

out vec3 fragColor;

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

const vec3 LUMA = vec3(0.299, 0.587, 0.114);

void main() {
	#ifdef USE_TEXTURE_ARRAY
		vec4 c0 = texture(atlasTex, vec3(vUV.xy, vLayer));
		vec4 c1 = texture(atlasTex, vec3(vUV.zw, vLayer));
	#else
		vec4 c0 = texture(atlasTex, vUV.xy);
		vec4 c1 = texture(atlasTex, vUV.zw);
	#endif

	vec4 color = vec4(mix(c0, c1, vBF));
	color *= vCol;
	
	color.rgb = mix(vec3(dot(color.rgb, LUMA)), color.rgb, shadowColorMode);

	if (AlphaDiscard(color.a))
		discard;

	// use "multiply" blending R = S * D, but it needs to account for alpha too
	// so with alpha, this formula seems to make some sense: R = D * (1 - Sa) + S * D * Sa ==>
	// ==> R = D * (1 - Sa + S * Sa)
	// with glBlendFunc(GL_ZERO, GL_SRC_COLOR)
	// GL_SRC_COLOR becomes (1 - Sa + S * Sa), so write this part to the RGB buffer
	fragColor = vec3(1.0) - color.aaa + color.rgb * color.aaa;
}