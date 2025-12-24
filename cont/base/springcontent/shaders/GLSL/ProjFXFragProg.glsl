#version 130

#ifdef USE_TEXTURE_ARRAY
	uniform sampler2DArray atlasTex;
#else
	uniform sampler2D      atlasTex;
#endif

uniform sampler2D depthTex;
uniform float softenThreshold;
uniform vec2 softenExponent;
uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass
uniform vec3 fogColor;
uniform vec2 distUni; // time, uvOffsetMag

in vec4 vCol;
centroid in vec4 vUV;
in float vLayer;
in float vBF;
in float fragDist;
in float fogFactor;
in vec3 wsPos;
in vec4 vsPos;
in vec4 vsDistParam2;
in vec2 vsDistParam1;
noperspective in vec2 screenUV;

out vec4 fragColor;
out vec4 distVec;

#define projMatrix gl_ProjectionMatrix

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

// LLM generated derivative of
//  https://github.com/BrianSharpe/Wombat/blob/master/Perlin2D.glsl
vec2 Perlin2D2(in vec2 P)
{
    // Establish our grid cell and unit position
    vec2 Pi = floor(P);
    vec4 Pf_Pfmin1 = P.xyxy - vec4( Pi, Pi + 1.0 );

    // Calculate the hash
    // (This part is identical to the original - high quality, low cost hashing)
    vec4 Pt = vec4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += vec2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    vec4 hash_x = fract( Pt * ( 1.0 / 951.135664 ) );
    vec4 hash_y = fract( Pt * ( 1.0 / 642.949883 ) );

    // Calculate the gradient vectors
    vec4 grad_x = hash_x - 0.49999;
    vec4 grad_y = hash_y - 0.49999;

    // Compute the Normalization factor (reused for both components)
    // We scale things to a strict -1.0->1.0 range here ( *= 1.41421...)
    vec4 norm = inversesqrt( grad_x * grad_x + grad_y * grad_y ) * 1.414213562373095;

    // Cache the distance vectors
    vec4 px = Pf_Pfmin1.xzxz;
    vec4 py = Pf_Pfmin1.yyww;

    // Component 1: Standard Dot Product ( grad . dist )
    vec4 dot1 = norm * ( grad_x * px + grad_y * py );

    // Component 2: Orthogonal Dot Product ( rotate gradient 90 deg: -y, x )
    // This creates a second uncorrelated noise field with zero extra hashing.
    vec4 dot2 = norm * ( -grad_y * px + grad_x * py );

    // Calculate Interpolation Weights (Quintic / Perlin Curve)
    vec2 blend = Pf_Pfmin1.xy * Pf_Pfmin1.xy * Pf_Pfmin1.xy * (Pf_Pfmin1.xy * (Pf_Pfmin1.xy * 6.0 - 15.0) + 10.0);
    vec4 blend2 = vec4( blend, vec2( 1.0 - blend ) );

    // Calculate final weights for the 4 corners
    vec4 weights = blend2.zxzx * blend2.wwyy;

    // Return the two independent noise values
    return vec2( dot( dot1, weights ), dot( dot2, weights ) );
}

const vec3 LUMA = vec3(0.299, 0.587, 0.114);
const vec2 distCamDist = vec2(10, 3500);

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

	fragColor = color;
	fragColor.rgb = mix(fragColor.rgb, fogColor * fragColor.a, (1.0 - fogFactor));

	#ifdef SMOOTH_PARTICLES
	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);

	if (softenThreshold > 0.0) {
		float edgeSmoothness = smoothstep(0.0, softenThreshold, vsPos.z - depthVS); // soften edges
		fragColor *= pow(edgeSmoothness, softenExponent.x);
	} else {
		float edgeSmoothness = smoothstep(softenThreshold, 0.0, vsPos.z - depthVS); // follow the surface up
		fragColor *= pow(edgeSmoothness, softenExponent.y);
	}
	#endif

	//vec2 uvMag = vsDistParam1 * distUni.y;
	vec2 uvMag = vec2(1.0) * distUni.y;
	if (dot(uvMag, uvMag) > 0.0) {
		float distFactor = clamp((fragDist - distCamDist.x) / (distCamDist.y - distCamDist.x), 0.0, 1.0);
		distFactor = pow(distFactor, 0.1);
		distFactor = 1.0 - distFactor;
		float distTexIntensity = dot(color.rgb, LUMA);
		distTexIntensity = fragColor.a;
		//distVec = vec4(Perlin2D2(10.0 * vsDistParam2.xy * vUV.xy + vsDistParam2.zw * distUni.x) * uvMag * fragColor.a, 0.0, fragColor.a);
		distVec = vec4(Perlin2D2(vec2(0.08, 0.18) * wsPos.xy  + vec2(1.0, 1.0) * vec2(distUni.x)) * uvMag * pow(distTexIntensity, 0.75), 0.0, distTexIntensity);
		distVec *= distFactor;
		//distVec *= 1.0 - smoothstep(1000.0, 4000.0, fogDist);
		//distVec = vec4(fragColor.rg, 0.0, fragColor.a);
		//distVec.xy = vec2(distTexIntensity);
	} else {
		distVec = vec4(0.0);
	}
	//distVec.xy = vec2(0.0);
	//distVec = 10.0 * fragColor.aaaa;

	if (AlphaDiscard(fragColor.a))
		discard;
}