// Converted from ARB/dwDetailNormal.vp
// Generates 8 scrolling wave texcoords for detail normal map blending

#version 130

// Current frame number (for wave animation)
uniform float frame;  // env9.x = gs->frameNum

// Wave direction/speed params: xyz = direction (used for dot), w = speed
uniform vec4 env10;
uniform vec4 env11;
uniform vec4 env12;
uniform vec4 env13;
uniform vec4 env14;
uniform vec4 env15;
uniform vec4 env16;
uniform vec4 env17;

out vec2 texCoord0;
out vec2 texCoord1;
out vec2 texCoord2;
out vec2 texCoord3;
out vec2 texCoord4;
out vec2 texCoord5;
out vec2 texCoord6;
out vec2 texCoord7;

// Computes wave phase: wave_speed * time + dot(position, wave_dir)
// ARB: DP3 temp, vertex.texcoord, env  → dot3
//      MAD result.texcoord[i], env.w, frame, temp  → env.w * frame + temp
float wavePhase(vec4 waveParam, vec2 pos) {
	float proj = dot(pos, waveParam.xy);
	return waveParam.w * frame + proj;
}

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec2 tc = gl_MultiTexCoord0.xy;

	float p0 = wavePhase(env10, tc);
	float p1 = wavePhase(env11, tc);
	float p2 = wavePhase(env12, tc);
	float p3 = wavePhase(env13, tc);
	float p4 = wavePhase(env14, tc);
	float p5 = wavePhase(env15, tc);
	float p6 = wavePhase(env16, tc);
	float p7 = wavePhase(env17, tc);

	// The ARB shader stores the scalar phase in all 4 components of texcoord,
	// and the 2D TEX instruction uses only .xy, so (phase, phase) is the 2D coord.
	texCoord0 = vec2(p0, p0);
	texCoord1 = vec2(p1, p1);
	texCoord2 = vec2(p2, p2);
	texCoord3 = vec2(p3, p3);
	texCoord4 = vec2(p4, p4);
	texCoord5 = vec2(p5, p5);
	texCoord6 = vec2(p6, p6);
	texCoord7 = vec2(p7, p7);
}
