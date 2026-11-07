// Converted from ARB/waterDynWave3.vp
// waveCopyHeight pass: copies wave height from float texture to smaller render target

#version 130

out vec2 texCoord;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	texCoord = gl_MultiTexCoord0.xy;
}
