// Converted from ARB/waterDynWave3.fp
// waveCopyHeight pass: copies red channel of wave height texture

#version 130

uniform sampler2D tex0;

in vec2 texCoord;

void main() {
	float height = texture2D(tex0, texCoord).r;
	gl_FragColor = vec4(height);
}
