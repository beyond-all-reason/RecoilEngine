// Converted from ARB/waterDyn.vp  (merges NV_vertex_program3 and NT fallback)
// Main water surface vertex shader: MVP transform, wave displacement, shadow coords

#version 130

// Wave height lookup: scale+offset to map world XZ into wave-map UV
uniform vec4 waveHeightScale;  // {1/(W_SIZE*256), 1/(W_SIZE*256), 0, 0}  (ARB VP env[10])
uniform vec4 waveHeightOfs;    // {-camPosX/256+0.5, -camPosZ/256+0.5, 0, 0}  (ARB VP env[11])
// World-to-wave-map UVs for refraction/reflection lookups
uniform vec4 waveNormalScale;  // {1/WF_SIZE, 1/WF_SIZE, 0, 0}  (ARB VP env[12])
uniform vec4 waveNormalOfs;    // {-(camPosBig.x-WH_SIZE)/WF_SIZE, ..., 0, 0}  (ARB VP env[13])
// Map-space UV scale
uniform vec4 mapUVScale;       // {1/(mapxp1*SQ), 1/(mapyp1*SQ), 0, 0}  (ARB VP env[14])

// Shadow matrix (replaces state.matrix.program[0])
uniform mat4 shadowMatrix;
// Shadow PCF params (replaces env[16..18]); if all zero, no soft PCF
uniform vec4 env16;  // {0.5, 0.5, 0, 0} bias
uniform vec4 env17;  // shadow softness A
uniform vec4 env18;  // shadow softness B

// Wave height texture (sampled in VS for displacement)
uniform sampler2D waveHeightTex;  // texture[2] from DynWater::Draw

out vec4 worldPos;      // texcoord[0] in ARB = world position passed to FS
out vec4 shadowCoord;   // texcoord[1] = shadow texture coordinate
out vec2 waveMapCoord;  // texcoord[2] = wave normal map UV
out vec2 foamCoord;     // texcoord[3] = {0.004,0.004} scale for waveHeight32
out vec2 mapCoord;      // texcoord[4] = map shading texture UV

void main() {
	vec4 pos = gl_Vertex;

	// Sample wave height texture to displace vertex vertically
	vec2 waveTC = pos.xz * waveHeightScale.xy + waveHeightOfs.xy;
	float waveHeight = texture2D(waveHeightTex, waveTC).r;
	pos.y += waveHeight;

	// Wave normal map UV
	waveMapCoord = pos.xz * waveNormalScale.xy + waveNormalOfs.xy;
	// {0.004, 0.004} foamCoord
	foamCoord  = pos.xz * vec2(0.004, 0.004);
	// Map shading UV
	mapCoord   = pos.xz * mapUVScale.xy;

	worldPos = pos;

	// Shadow texture coordinate (soft PCF variant from ARB shader)
	vec4 shadowPos;
	shadowPos.x = dot(pos, shadowMatrix[0]);
	shadowPos.y = dot(pos, shadowMatrix[1]);
	shadowPos.z = dot(pos, shadowMatrix[2]);
	shadowPos.w = dot(pos, shadowMatrix[3]);

	vec2 st = shadowPos.xy;
	vec2 absST = abs(st) + env17.xy;
	absST = inversesqrt(absST);
	absST = absST + env18.xy;
	shadowCoord.xy = st * absST + env16.xy;
	shadowCoord.z  = shadowPos.z;
	shadowCoord.w  = shadowPos.w;

	// Fog depth
	gl_FogFragCoord = (gl_ModelViewProjectionMatrix * pos).z;

	gl_Position = gl_ModelViewProjectionMatrix * pos;
}
