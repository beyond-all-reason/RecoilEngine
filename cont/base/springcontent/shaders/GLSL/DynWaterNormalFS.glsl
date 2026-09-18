// Converted from ARB/waterDynNormal.fp
// Computes water surface normal from 4 height samples

#version 130

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;

// env0 = {0, 0, W_SIZE*2, 0} - cross vector 1 base (xz cross component)
// env1 = {W_SIZE*2, 0, 0, 0} - cross vector 2 base (xz cross component)
uniform vec4 env0;
uniform vec4 env1;

in vec2 texCoord0;
in vec2 texCoord1;
in vec2 texCoord2;
in vec2 texCoord3;

void main() {
	vec4 h1 = texture2D(tex0, texCoord0);
	vec4 h2 = texture2D(tex1, texCoord1);
	vec4 h3 = texture2D(tex2, texCoord2);
	vec4 h4 = texture2D(tex3, texCoord3);

	float foam = h1.y + h2.y;

	vec3 cross1 = vec3(env0.x, h1.x - h2.x, env0.z);
	vec3 cross2 = vec3(env1.x, h3.x - h4.x, env1.z);

	vec3 normal = cross(cross1, cross2);
	normal = normalize(normal);

	gl_FragColor = vec4(normal, foam * 0.5);
}
