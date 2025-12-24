#version 130

in vec2 uv;

uniform sampler2D screenCopyTex;
uniform sampler2D distortionTex;
uniform vec3 params; // uvOffMax, numberOfLODs, chromaticOberation

out vec4 fragColor;

float easeOutSaturate(float bmin, float bmax, float x) {
	// Normalize input to [0, 1] range
	float t = clamp((x - bmin) / (bmax - bmin), 0.0, 1.0);

	// Linear in lower/middle, saturate toward upper end
	float it = 1.0 - t;
	return 1.0 - it * it * it;
}

void main()
{
	vec2 uvOff = texture(distortionTex, uv).xy;
	float uvOffMag = length(uvOff);
	float uvOffMagMax = params.x;
	//uvOffMagMax = 0.01;
	
	float uvRatio = uvOffMag / uvOffMagMax;
	//
	//if (uvRatio > 0.0) {
	//	float adjFactor = easeOutSaturate(0.0, uvOffMagMax, uvOffMag) / uvRatio;
	//	uvOff *= adjFactor;
	//	uvOffMag *= adjFactor;
	//}

	float lodR = clamp(uvRatio, 0.0, 1.0);
	float lodA = lodR * params.y;
	//lodA = 0.0;
	
	fragColor = textureLod(screenCopyTex, uv + uvOff, lodA);
	if (params.z != 0.0 && uvOffMag > 0.0) {
		fragColor.r = textureLod(screenCopyTex, uv + (uvOff * (1.0 + 0.5 * params.z)), lodA).r;
		fragColor.b = textureLod(screenCopyTex, uv + (uvOff * (1.0 - 0.5 * params.z)), lodA).b;
	}
	//fragColor = vec4(100.0 * uvOff, 0.0, 1.0);
	//fragColor = textureLod(screenCopyTex, uv, 0.0);
	//fragColor = vec4(texture(distortionTex, uv).xy, 0.0, 1.0);
}