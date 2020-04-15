#version 330
		
in vec2 textureCoords;
uniform sampler2D diffuseTexture;
out vec4 color;

void main(){
	color = texture(diffuseTexture, textureCoords);
}