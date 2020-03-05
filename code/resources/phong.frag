#version 330
		
in vec4			frag_Normal;
in vec4			frag_Pos;

out vec4		out_Color;

uniform mat4	mv_Mat; 


uniform vec4	_color_diffuse;
uniform vec4	_color_ambient;
uniform vec4	_color_specular;
uniform float	_specular_strength;

uniform vec4	_light_color;
uniform vec4	_light_position;
uniform float	_light_strength;


void main(){

//GLOBAL
	vec4 frag_Normal_n = normalize(frag_Normal);
	vec4 light_direction = normalize(_light_position - frag_Pos);
	float light_distance = distance(_light_position, frag_Pos);

//DIFFUSE
	vec4 diffuse = max(dot(frag_Normal_n, light_direction), 0)
		* _color_diffuse * _light_color * _light_strength
		/ (light_distance*light_distance);

//SPECULAR
	vec4 _camera_position = inverse(mv_Mat)[3];
	vec4 _camera_direction = normalize(_camera_position - frag_Pos);
	vec4 _ref_direction = reflect(-light_direction, frag_Normal_n);

	vec4 specular = pow(max(dot(_camera_direction, _ref_direction), 0.0), 32)
		* _color_specular * _light_color * _light_strength * _specular_strength
		/ (light_distance*light_distance);

//RESULT
	out_Color =
		+ _color_ambient
		+ diffuse
		+ specular;
}