// Converted from ARB/waterDynWave.vp
// Wave height update pass: computes offset texcoords for 5-tap stencil

#version 130

// Neighbour offsets (1/1024 in each diagonal direction)
uniform vec4 env8;   // {-1/1024,  1/1024, 0, 0}
uniform vec4 env9;   // {  0,      1/1024, 0, 0}
uniform vec4 env10;  // { 1/1024,  1/1024, 0, 0}
uniform vec4 env11;  // { 1/1024,  0,      0, 0}
// Camera scroll delta in wave-map space
uniform vec4 env14;  // {dx/WF_SIZE, dy/WF_SIZE, 0, 0}
// Background wave offset
uniform vec4 env15;  // {(camPosBig.x-WH_SIZE)/WF_SIZE*4, same, 0, 0}

out vec2 texCoord0;
out vec2 texCoord1;
out vec2 texCoord2;
out vec2 texCoord3;
out vec2 texCoord4;
out vec2 texCoord5;
out vec2 texCoord6;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec2 tc = gl_MultiTexCoord0.xy;

	texCoord0 = tc + env14.xy;
	texCoord1 = tc;
	texCoord2 = tc + env8.xy;
	texCoord3 = tc + env9.xy;
	texCoord4 = tc + env10.xy;
	texCoord5 = tc + env11.xy;
	texCoord6 = tc * 4.0 + env15.xy;
}
