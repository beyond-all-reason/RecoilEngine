#version 330

// One triangle covering the viewport, no vertex attributes (draw 3 vertices).
void main() {
	vec2 uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
