// Vertex shader for AdvWater / RefractWater
// Replaces fixed-function pipeline + GL_EYE_LINEAR texgen for bump coords

#version 130

// Object-linear scale for water depth tex (RefractWater only, ignored by AdvWater)
// {1/(mapxp1*SQUARE_SIZE), 1/(mapyp1*SQUARE_SIZE)}
uniform vec2 waterDepthScale;

out vec2 reflectCoord;   // texcoord[0]: manually-computed reflection UV
out vec2 bumpCoord;      // texcoord[1]: eye-linear (replicates GL_EYE_LINEAR with planes {0.02,0,0,0}/{0,0,0.02,0})
out vec2 waterDepthCoord;// texcoord[3]: object-linear for map shading / water depth (RefractWater)
out vec4 vertColor;

void main() {
	gl_Position  = gl_ModelViewProjectionMatrix * gl_Vertex;
	reflectCoord = gl_MultiTexCoord0.xy;

	// Eye-linear texgen for bump texture:
	//   S plane = {0.02, 0, 0, 0}  →  s = eyePos.x * 0.02
	//   T plane = {0,    0, 0.02, 0} →  t = eyePos.z * 0.02
	vec4 eyePos = gl_ModelViewMatrix * gl_Vertex;
	bumpCoord   = vec2(eyePos.x * 0.02, eyePos.z * 0.02);

	// Object-linear texgen for water depth (replaces GL_OBJECT_LINEAR on tex unit 3)
	waterDepthCoord = vec2(gl_Vertex.x * waterDepthScale.x, gl_Vertex.z * waterDepthScale.y);

	vertColor = gl_Color;
}
