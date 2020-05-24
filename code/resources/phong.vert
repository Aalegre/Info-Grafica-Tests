#version 330
layout (location = 0)	in vec3		in_Position; 
layout (location = 1)	in vec3		in_Normal; 
layout (location = 2)	in vec2		in_UVs; 
layout (location = 3)	in vec3		in_Tangent; 
layout (location = 4)	in mat4		objMat; 
uniform mat4	mvpMat; 
uniform mat4	mv_Mat; 
out vec4		frag_Normal;
out vec4		frag_Pos;
out vec2		frag_UVs;
out mat3		frag_TBN;
	


void main(){
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
	frag_Normal = objMat * vec4(in_Normal, 0.0);
	frag_Pos = objMat * vec4(in_Position, 1.0);
	frag_UVs = in_UVs;
	vec3 T = normalize(vec3(objMat * vec4(in_Tangent,   0.0)));
	vec3 B = cross(vec3(frag_Normal), T);
	frag_TBN = mat3(vec3(0), vec3(0), vec3(frag_Normal));
}