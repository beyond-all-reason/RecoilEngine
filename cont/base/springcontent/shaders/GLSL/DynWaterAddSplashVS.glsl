// Converted from ARB/dwAddSplash.vp
// Transforms ship/explosion splash quads into wave-map space

#version 130

// Scale: {1/WF_SIZE, 1/WF_SIZE, 0, 1}
uniform vec4 env10;
// Offset: {-(oldCamPosBig.x-WH_SIZE)/WF_SIZE, -(oldCamPosBig.z-WH_SIZE)/WF_SIZE, 0, 0}
uniform vec4 env11;

out vec2 texCoord0;
out vec4 wakeData;  // carries normal/intensity (was texcoord[1])

void main() {
	// ARB: MAD pos, ipos.xzyw, env10, env11
	// ipos.xzyw = swizzle so world XZ → wave-map XY, world Y → wave-map Z
	vec4 ipos = gl_Vertex;
	vec4 swiz = vec4(ipos.x, ipos.z, ipos.y, ipos.w);
	vec4 pos  = swiz * env10 + env11;

	gl_Position = gl_ModelViewProjectionMatrix * pos;

	texCoord0 = gl_MultiTexCoord0.xy;

	// Normal packed into texcoord[1]; .w gets normal.z
	vec3 n  = gl_Normal;
	wakeData = vec4(n.x, n.y, n.z, n.z);
}
