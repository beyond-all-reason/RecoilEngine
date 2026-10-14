// Converted from ARB/waterDynWave2.vp
// Wave flow update pass: computes offset texcoords for 5-tap flow stencil

#version 130

// Neighbour offsets (same as wave height pass but subtracted from scrolled tc)
uniform vec4 env8;   // {-1/1024,  1/1024, 0, 0}
uniform vec4 env9;   // {  0,      1/1024, 0, 0}
uniform vec4 env10;  // { 1/1024,  1/1024, 0, 0}
uniform vec4 env11;  // { 1/1024,  0,      0, 0}
// Ground/map coordinate scale for gheight lookup
uniform vec4 env12;  // {WF_SIZE/(mapx*SQ), WF_SIZE/(mapy*SQ), 0, 0}
uniform vec4 env13;  // {(camPosBig.x-WH_SIZE)/(pwr2mapx*SQ), ..., 0, 0}
// Camera scroll delta
uniform vec4 env14;  // {dx/WF_SIZE, dy/WF_SIZE, 0, 0}

out vec2 texCoord0;
out vec2 texCoord1;
out vec2 texCoord2;
out vec2 texCoord3;
out vec2 texCoord4;
out vec2 texCoord5;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec2 tc = gl_MultiTexCoord0.xy;
	vec2 scrolled = tc + env14.xy;

	texCoord0 = scrolled;
	texCoord1 = scrolled - env8.xy;
	texCoord2 = scrolled - env9.xy;
	texCoord3 = scrolled - env10.xy;
	texCoord4 = scrolled - env11.xy;
	texCoord5 = tc * env12.xy + env13.xy;
}
