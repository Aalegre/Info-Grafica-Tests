#version 330
		
in vec4 vert_Normal;
out vec4 out_Color;
uniform mat4 mv_Mat;
uniform vec4 color;
uniform vec4 position;
void main(){
	out_Color = vec4(color.xyz
	* dot(vert_Normal, mv_Mat * position)
	+ color.xyz * 0.3, 1.0 );
}