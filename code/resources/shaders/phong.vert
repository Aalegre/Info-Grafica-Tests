#version 330
in vec3			in_Position; 
in vec3			in_Normal; 
in vec2			in_UVs; 
in vec3			in_Tangent; 
in vec3			in_BiTangent; 
uniform mat4	objMat; 
uniform mat4	mvpMat; 
uniform mat4	mv_Mat; 
out vec4		frag_Normal;
out vec4		frag_Pos;
out vec2		frag_UVs;
out vec4		frag_Tangent;
out vec4		frag_BiTangent;
out mat3		frag_TBN;
		
void main(){
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
	frag_Normal = objMat * vec4(in_Normal, 0.0);
	frag_Pos = objMat * vec4(in_Position, 1.0);
	frag_UVs = in_UVs;
	frag_Tangent = objMat * vec4(in_Tangent, 0.0);
	frag_BiTangent = objMat * vec4(in_BiTangent, 0.0);
	frag_TBN = transpose(mat3(
        frag_Tangent,
        frag_BiTangent,
        frag_Normal
    ));
}