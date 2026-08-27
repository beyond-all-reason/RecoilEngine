// Converted from ARB/waterDynNormal.vp
// Samples wave heights at 4 neighbour offsets for normal map computation

#version 130

uniform vec4 env9;   // {0, 1/1024, 0, 0} - vertical neighbour offset
uniform vec4 env11;  // {1/1024, 0, 0, 0} - horizontal neighbour offset

out vec2 texCoord0;
out vec2 texCoord1;
out vec2 texCoord2;
out vec2 texCoord3;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec2 tc = gl_MultiTexCoord0.xy;
	// MAD result.texcoord[N], program.env[M], +-1, vertex.texcoord
	// = env * (+-1) + tc
	texCoord0 = env9.xy  + tc;
	texCoord1 = -env9.xy + tc;
	texCoord2 = env11.xy + tc;
	texCoord3 = -env11.xy + tc;
}
