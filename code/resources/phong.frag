#version 330
		
in vec4			frag_Normal;
in vec4			frag_Pos;

out vec4		out_Color;

uniform mat4	mv_Mat; 


uniform vec4	_color_diffuse;
uniform vec4	_color_ambient;
uniform vec4	_color_specular;
uniform float	_specular_strength;

#define LIGHT_POINT_MAX 16

struct PointLight {
	bool enabled;
    vec4 position;
    vec4 color;
	float strength;
	vec4 getDiffuse(vec4 nor, vec4 pos, vec4 col)
	{
		vec4 frag_Normal_n = normalize(nor);
		vec4 light_direction = normalize(position - pos);
		float light_distance = distance(position, pos);
		vec4 diffuse = max(dot(frag_Normal_n, light_direction), 0)
			* col * color * strength
			/ (light_distance*light_distance);
		return diffuse;
	}
	vec4 getSpecular(vec4 nor, vec4 pos, mat4 modelView, vec4 col, float str)
	{
		vec4 frag_Normal_n = normalize(nor);
		vec4 light_direction = normalize(position - pos);
		float light_distance = distance(position, pos);
		vec4 _camera_position = inverse(modelView)[3];
		vec4 _camera_direction = normalize(_camera_position - pos);
		vec4 _ref_direction = reflect(-light_direction, frag_Normal_n);
		vec4 specular = pow(max(dot(_camera_direction, _ref_direction), 0.0), 32)
			* col * color * strength * str
			/ (light_distance*light_distance);
		return specular;
	}
};

uniform PointLight lights[LIGHT_POINT_MAX];



void main(){

	vec4 diffuse;
	vec4 specular;

	for	(int i = 0; i < LIGHT_POINT_MAX; i++){
		if(lights[i].enabled){
			diffuse += lights[i].getDiffuse(frag_Normal, frag_Pos, _color_diffuse);
			specular += lights[i].getSpecular(frag_Normal, frag_Pos, mv_Mat, _color_specular, _specular_strength);
		}
	}

//RESULT
	out_Color =
		+ _color_ambient
		+ diffuse
		+ specular;
}