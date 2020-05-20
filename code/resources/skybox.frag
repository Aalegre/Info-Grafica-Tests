#version 330
out vec4 out_Color;
in vec3 textureDir;
uniform samplerCube _cubemap;
uniform float _exposureMin;
uniform float _exposureMax;
uniform vec4 _tint;

void main(){

	out_Color = _exposureMin + ((texture(_cubemap, textureDir) * _tint) - 0) * (_exposureMax - _exposureMin) / (1 - 0);
}