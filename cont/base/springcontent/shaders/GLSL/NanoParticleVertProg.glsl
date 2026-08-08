/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#version 150 compatibility

in vec3 pos;
in vec3 uvw;
in vec4 uvInfo;
in vec3 aparams;
in vec4 color;

uniform float animationFrame;

out vec3 v_velocity;
out vec2 v_lifetime;
out vec4 v_color;

void main()
{
	float ballisticAge = max(animationFrame - uvInfo.x, 0.0) * aparams.x;
	gl_Position = vec4(pos + uvw * ballisticAge, 1.0);
	v_velocity = uvw;
	v_lifetime = uvInfo.xy;
	v_color = color;
}