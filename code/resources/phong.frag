#version 330
		
in vec4			frag_Normal;
in vec4			frag_Pos;
in vec2			frag_UVs;
in mat3			frag_TBN;

out vec4		out_Color;

uniform mat4	mv_Mat; 


uniform vec4	_color_diffuse;
uniform vec4	_color_ambient;
uniform vec4	_color_specular;
uniform vec4	_tilingOffset;
uniform float	_specular_strength;
uniform float	_normal_strength;
uniform float   _alphaCutout;
uniform bool	_alphaCutoutInvert;


uniform sampler2D _albedo;
uniform sampler2D _normal;
uniform sampler2D _specular;
uniform sampler2D _emissive;

#define LIGHT_POINT_MAX 16

struct DirectionalLight{
    vec4 direction;
    vec4 color;
	float strength;
};
	vec4 DirectionalLight_getDiffuse(DirectionalLight d_light, vec4 d_vertexNormal, vec4 d_vertexPosition, vec4 d_diffuseColor)
	{
		return max(dot(d_vertexNormal, d_light.direction), 0)
			* d_diffuseColor * d_light.color * d_light.strength;
	}
	vec4 DirectionalLight_getSpecular(DirectionalLight s_light, vec4 s_vertexNormal, vec4 s_vertexPosition, vec4 s_specularColor, mat4 s_modelView, float s_strength)
	{
		vec4 s_camera_position = inverse(s_modelView)[3];
		vec4 s_camera_direction = normalize(s_camera_position - s_vertexPosition);
		vec4 s_ref_direction = reflect(-s_light.direction, s_vertexNormal);
		return pow(max(dot(s_camera_direction, s_ref_direction), 0.0), 32)
			* s_specularColor * s_light.color * s_light.strength * s_strength;
	}

uniform DirectionalLight _mainLight;

struct PointLight {
	bool enabled;
    vec4 position;
    vec4 color;
	float strength;
};
	vec4 PointLight_getDiffuse(PointLight d_plight, vec4 d_vertexNormal, vec4 d_vertexPosition, vec4 d_diffuseColor)
	{
		vec4 d_light_direction = normalize(d_plight.position - d_vertexPosition);
		float d_light_distance = distance(d_plight.position, d_vertexPosition);
		return max(dot(d_vertexNormal, d_light_direction), 0)
			* d_diffuseColor * d_plight.color * d_plight.strength
			/ (d_light_distance*d_light_distance);
	}
	vec4 PointLight_getSpecular(PointLight s_plight, vec4 s_vertexNormal, vec4 s_vertexPosition, vec4 s_specularColor, mat4 s_modelView, float s_strength)
	{
		vec4 s_light_direction = normalize(s_plight.position - s_vertexPosition);
		float s_light_distance = distance(s_plight.position, s_vertexPosition);
		vec4 s_camera_position = inverse(s_modelView)[3];
		vec4 s_camera_direction = normalize(s_camera_position - s_vertexPosition);
		vec4 s_ref_direction = reflect(-s_light_direction, s_vertexNormal);
		return pow(max(dot(s_camera_direction, s_ref_direction), 0.0), 32)
			* s_specularColor * s_plight.color * s_plight.strength * s_strength
			/ (s_light_distance*s_light_distance);
	}

uniform PointLight lights[LIGHT_POINT_MAX];



void main(){

	vec4 diffuse;
	vec4 specular;
	vec2 texCoord = frag_UVs;
	texCoord.r = texCoord.r * _tilingOffset.r;
	texCoord.g = texCoord.g * _tilingOffset.g;
	texCoord.r = texCoord.r + _tilingOffset.b;
	texCoord.g = texCoord.g + _tilingOffset.w;
	vec4 albedoColor = texture(_albedo, texCoord);
	if(_alphaCutoutInvert){
		if(albedoColor.w > _alphaCutout){
			discard;
		}
	} else {
		if(albedoColor.w < _alphaCutout){
			discard;
		}
	}
	vec3 fixed_normal_tex = (texture(_normal, texCoord ).rgb * 0.5 + 0.5);
	vec4 frag_Normal_n = vec4(normalize(frag_TBN * fixed_normal_tex), 0) * _normal_strength;
	vec4 specular_tex = texture(_specular, texCoord);
	vec4 specular_tex_color = specular_tex;
	for	(int i = 0; i < LIGHT_POINT_MAX; i++){
		if(lights[i].enabled){
			diffuse += PointLight_getDiffuse(lights[i], frag_Normal_n, frag_Pos, albedoColor * _color_diffuse);
			specular += PointLight_getSpecular(lights[i], frag_Normal_n, frag_Pos, specular_tex_color + _color_specular, mv_Mat, specular_tex.w + _specular_strength);
		}
	}
	diffuse += DirectionalLight_getDiffuse(_mainLight, frag_Normal_n, frag_Pos, albedoColor * _color_diffuse);
	specular += DirectionalLight_getSpecular(_mainLight, frag_Normal_n, frag_Pos, specular_tex_color + _color_specular, mv_Mat, specular_tex.w + _specular_strength);

//RESULT
vec4 finalColor = 
		+ texture(_emissive, texCoord)
		+ _color_ambient
		+ diffuse
		+ specular;
	finalColor.w = albedoColor.w * _color_diffuse.w;
out_Color = finalColor;
}