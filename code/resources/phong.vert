#version 330
in vec3			in_Position; 
in vec3			in_Normal; 
uniform mat4	objMat; 
uniform mat4	mvpMat; 
out vec4		frag_Normal;
out vec4		frag_Pos;
		
void main(){
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);;
	frag_Normal = objMat * vec4(in_Normal, 0.0);
	frag_Pos = objMat * vec4(in_Position, 1.0);
}