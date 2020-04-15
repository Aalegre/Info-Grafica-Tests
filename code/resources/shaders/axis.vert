#version 330
in vec3 in_Position;
in vec4 in_Color;
out vec4 vert_color;
uniform mat4 mvpMat;
uniform vec3 _position;
void main() {
	vert_color = in_Color;
	gl_Position = mvpMat * vec4((in_Position + _position), 1.0);
}