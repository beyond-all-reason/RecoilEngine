// Converted from ARB/waterDyn.fp
// Main water surface fragment shader: reflection, refraction, specular, foam

#version 130

uniform sampler2D      waterNormalTex;  // texture[0] = waveTex3 (normals + foam)
uniform sampler2D      reflectTex;      // texture[1] = reflectTexture
uniform sampler2D      waveHeight32;    // texture[2] (unused in FS, but was tex unit 2)
uniform sampler2D      refractTex;      // texture[3] = refractTexture
uniform sampler2D      shadingTex;      // texture[4] = map shading (alpha=ground height)
uniform sampler2D      foamTex;         // texture[5] = foamTex
uniform sampler2D      detailNormalTex; // texture[6] = detailNormalTex
uniform sampler2DShadow shadowTex;      // texture[7] = shadow depth

// Camera position
uniform vec4 env1;  // camera world position

// Reflection camera basis vectors
uniform vec4 env2;  // reflectRight
uniform vec4 env3;  // reflectUp
uniform vec4 env4;  // {0.5/dx, 0.5/dy, 1, 1} - reflection project scale
uniform vec4 env5;  // reflectForward

// Fresnel constants
uniform vec4 env6;  // {fresnelMin=0.05, fresnelRange=0.95, 0, 0}
uniform vec4 env7;  // {turbulenceScale=0.2, 0, 0, 0}

// Water color
uniform vec4 env8;  // {waterR=0.5, waterG=0.6, waterB=0.8, 0}

// Light direction
uniform vec4 env9;  // sun light direction (L)

// Lighting colors
uniform vec4 env10; // sun diffuse color
uniform vec4 env11; // sun ambient color

// Refraction camera basis vectors
uniform vec4 env12; // refractRight
uniform vec4 env13; // refractUp
uniform vec4 env14; // {0.5/dx, 0.5/dy, 1, 1} - refract project scale
uniform vec4 env15; // refractForward

in vec4 worldPos;
in vec4 shadowCoord;
in vec2 waveMapCoord;
in vec2 foamCoord;
in vec2 mapCoord;

void main() {
	// Shadow lookup
	float shadow = shadow2D(shadowTex, shadowCoord.xyz).r;

	// Camera direction
	vec3 camdir = worldPos.xyz - env1.xyz;
	float distSq = dot(camdir, camdir);
	float invDist = inversesqrt(distSq);
	camdir *= invDist;
	float camdist = 1.0 / invDist;

	// Sample water normal map
	vec4 waterNormal = texture2D(waterNormalTex, waveMapCoord);
	float foamIntensity = clamp(waterNormal.w * 5.0 - 1.0, 0.0, 1.0);

	// Ground height (encoded in shading texture alpha)
	float groundAlpha = texture2D(shadingTex, mapCoord).a;
	float groundHeight = groundAlpha * 0.4 - 0.4;

	// Foam texture modulates foam intensity
	float foamMask = texture2D(foamTex, foamCoord).r;
	foamIntensity *= foamMask;

	// Detail normal (additive to coarse normal)
	vec4 detailNormal = texture2D(detailNormalTex, foamCoord);
	waterNormal.xz += detailNormal.xy;
	waterNormal = normalize(waterNormal);

	// Reflect camera direction around water normal
	float cosAngle   = dot(camdir, waterNormal.xyz);
	vec3 reflectDir  = waterNormal.xyz * (-2.0 * cosAngle) + camdir;

	// Specular: two lobes (wide + tight)
	float specDot = clamp(dot(reflectDir, env9.xyz), 0.0, 1.0);
	float spec16  = pow(specDot, 16.0);
	float spec4k  = pow(specDot, 4096.0) * 10.0;
	spec4k *= shadow;

	float shadowLerp  = mix(shadow, 1.0, 0.3);
	float specular    = spec16 * shadowLerp * 0.5 + spec4k;

	// Perturb normal slightly by view distance for horizon smoothing
	vec3 waterNormal2 = normalize(waterNormal.xyz + vec3(0.0, camdist * 0.003, 0.0));
	float cosAngle2   = dot(camdir, waterNormal2);
	vec3 reflectDir2  = waterNormal2 * (-2.0 * cosAngle2) + camdir;
	reflectDir2.y = max(reflectDir2.y, 0.0);

	// Project into reflection texture space
	float invFwdR     = 1.0 / dot(reflectDir2, env5.xyz);
	reflectDir2      *= invFwdR;
	float reflU       = dot(reflectDir2, env2.xyz) * env4.x + 0.5;
	float reflV       = dot(reflectDir2, env3.xyz) * env4.y + 0.5;
	vec4 reflectColor = texture2D(reflectTex, vec2(reflU, reflV));
	reflectColor     += specular * env10;

	// Refraction
	vec3 refractDir  = waterNormal2 * groundHeight * vec3(1.0, 0.0, 1.0) + camdir;
	float invFwdRfr  = 1.0 / dot(refractDir, env15.xyz);
	refractDir      *= invFwdRfr;
	float refractU   = dot(refractDir, env12.xyz) * env14.x + 0.5;
	float refractV   = dot(refractDir, env13.xyz) * env14.y + 0.5;
	vec4 refractColor = texture2D(refractTex, vec2(refractU, refractV));

	// Fresnel blend: (1+cos)^5, matching ARB: temp.y = (1+cos)^4, temp.x *= temp.y
	float fresnel    = 1.0 + cosAngle;
	float fresnel4   = fresnel * fresnel;          // (1+cos)^2
	fresnel4         = fresnel4 * fresnel4;        // (1+cos)^4
	fresnel          = fresnel * fresnel4;         // (1+cos)^5
	fresnel          = fresnel * env6.y + env6.x; // scale to [fresnelMin, 1]
	vec4 clearColor  = mix(refractColor, reflectColor, fresnel);

	// Diffuse + ambient lighting
	float diffuse    = clamp(dot(waterNormal.xyz, env9.xyz), 0.0, 1.0) * shadow;
	vec4 lightColor  = diffuse * env10 + env11;
	vec4 waterColor  = lightColor * env8;

	// Turbulence blend
	vec4 blended     = mix(clearColor, waterColor, env7.x);

	// Foam
	gl_FragColor = mix(blended, lightColor, foamIntensity);
}
