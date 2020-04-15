#version 330 
in vec3 aPos;
in vec2 in_UVs;

out vec2 out_UVs;
out vec2 textureCoords;

void main(){
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);  // fix w = 1.0
	out_UVs = out_UVs;
	textureCoords = vec2(aPos.x + 0.5, aPos.y + 0.5);
}