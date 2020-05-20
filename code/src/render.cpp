#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#include <map>


#pragma warning(disable:4996)

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
	void setupAxis();
	void cleanupAxis();
	void drawAxis(glm::vec3 position_ = {0,0,0}, float scale_ = 1);
}
////////////////

namespace RenderVars {
	const float Initial_FOV = glm::radians(65.f);
	float FOV = glm::radians(65.f);
	const float zNear = 0.001f;
	const float zFar = 1000.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;
	glm::mat4 _view;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	const float initial_panv[3] = { 0.f, -2.5f, -5.f };
	const float initial_rota[2] = { 0.f, 0.f };

	float panv[3] = { initial_panv[0],initial_panv[1], initial_panv[2] };
	float rota[2] = { 0.f, 0.f };

	void UpdateProjection() {
		GLint m_viewport[4];
		glGetIntegerv(GL_VIEWPORT, m_viewport);

		_projection = glm::perspective(FOV, (float)m_viewport[2] / (float)m_viewport[3], zNear, zFar);
	}
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

#pragma region Loading

namespace FileLoader {
	std::string LoadString(const std::string& path_) {
		std::ifstream f(path_); //taking file as inputstream
		std::string str;
		if (f) {
			std::ostringstream ss;
			ss << f.rdbuf(); // reading data
			str = ss.str();
		}
		return str;
	}


	bool LoadOBJ(const char* path,
		std::vector < glm::vec3 >& out_vertices,
		std::vector < glm::vec2 >& out_uvs,
		std::vector < glm::vec3 >& out_normals) {

		std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
		std::vector< glm::vec3 > temp_vertices;
		std::vector< glm::vec2 > temp_uvs;
		std::vector< glm::vec3 > temp_normals;

		FILE* file = fopen(path, "r");
		if (file == NULL) {
			printf("Impossible to open the file !\n");
			return false;
		}

		while (1) {

			char lineHeader[128];
			// read the first word of the line
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF)
				break; // EOF = End Of File. Quit the loop.

			// else : parse lineHeader
			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				temp_uvs.push_back(uv);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				glm::vec3 normal;
				fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				temp_normals.push_back(normal);
			}
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					printf("File can't be read by our simple parser : ( Try exporting with other options\n");
					return false;
				}
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
		}

		// For each vertex of each triangle
		for (unsigned int i = 0; i < vertexIndices.size(); i++) {
			unsigned int vertexIndex = vertexIndices[i];
			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			out_vertices.push_back(vertex);

		}
		for (unsigned int i = 0; i < uvIndices.size(); i++) {
			unsigned int uvIndex = uvIndices[i];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			uv.y *= -1;
			out_uvs.push_back(uv);

		}
		for (unsigned int i = 0; i < normalIndices.size(); i++) {
			unsigned int normalIndex = normalIndices[i];
			glm::vec3 normal = temp_normals[normalIndex - 1];
			out_normals.push_back(normal);

		}
		return true;

	}
}

GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint compileShaderFromFile(const char* shaderPath, GLenum shaderType, const char* name = "") {
	return compileShader(FileLoader::LoadString(shaderPath).c_str(), shaderType, name);
}

void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

#pragma endregion

////////////////////////////////////////////////// AXIS
namespace Axis {
	GLuint AxisVao;
	GLuint AxisVbo[3];
	GLuint AxisShader[2];
	GLuint AxisProgram;

	float AxisVerts[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0
	};
	float AxisColors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};
	GLubyte AxisIdx[] = {
		0, 1,
		2, 3,
		4, 5
	};

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		AxisShader[0] = compileShader(FileLoader::LoadString("resources/axis.vert").c_str(), GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShader(FileLoader::LoadString("resources/axis.frag").c_str(), GL_FRAGMENT_SHADER, "AxisFrag");

		AxisProgram = glCreateProgram();
		glAttachShader(AxisProgram, AxisShader[0]);
		glAttachShader(AxisProgram, AxisShader[1]);
		glBindAttribLocation(AxisProgram, 0, "in_Position");
		glBindAttribLocation(AxisProgram, 1, "in_Color");
		linkProgram(AxisProgram);
	}
	void cleanupAxis() {
		glDeleteBuffers(3, AxisVbo);
		glDeleteVertexArrays(1, &AxisVao);

		glDeleteProgram(AxisProgram);
		glDeleteShader(AxisShader[0]);
		glDeleteShader(AxisShader[1]);
	}
	void drawAxis(glm::vec3 position_, float scale_) {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glUniform3f(glGetUniformLocation(AxisProgram, "_position"), position_.x, position_.y, position_.z);
		glUniform1f(glGetUniformLocation(AxisProgram, "_scale"), scale_);
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}


class Skybox {
public:
	float vertices[3];
	float exposureMin = 0;
	float exposureMax = 1;
	float rotation = 0;
	glm::vec3 tint;

	GLuint program;
	GLuint VAO, VBO;

	GLuint cubemapID;

	~Skybox() {
		cleanup();
	}

	void init() {
		GLuint vertex_shader = compileShaderFromFile("resources/skybox.vert", GL_VERTEX_SHADER, "skyboxVert");
		GLuint geometry_shader = compileShaderFromFile("resources/skybox.geom", GL_GEOMETRY_SHADER, "skyboxGeom");
		GLuint fragment_shader = compileShaderFromFile("resources/skybox.frag", GL_FRAGMENT_SHADER, "skyboxFrag");
		program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, geometry_shader);
		glAttachShader(program, fragment_shader);
		linkProgram(program);
		glDeleteShader(vertex_shader);
		glDeleteShader(geometry_shader);
		glDeleteShader(fragment_shader);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		vertices[0] = 0;
		vertices[1] = 0;
		vertices[2] = 0;
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(0);

		glGenTextures(1, &cubemapID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

		int width, height, nchannels;
		unsigned char* data;
		/*
		std::vector<std::string> textures_faces = {
			"cubemap/right.png",
			"cubemap/left.png",
			"cubemap/up.png",
			"cubemap/down.png",
			"cubemap/back.png",
			"cubemap/front.png",
		};
		*/
		std::vector<std::string> textures_faces = {

			"resources/textures/River_Skybox/right.jpg",
			"resources/textures/River_Skybox/left.jpg",
			"resources/textures/River_Skybox/top.jpg",
			"resources/textures/River_Skybox/bottom.jpg",
			"resources/textures/River_Skybox/front.jpg",
			"resources/textures/River_Skybox/back.jpg",
		};
		for (GLuint i = 0; i < textures_faces.size(); i++) {
			data = stbi_load(textures_faces[i].c_str(), &width, &height, &nchannels, 0);
			if (data == NULL) {
				fprintf(stderr, "Error loading image %s", textures_faces[i].c_str());
				exit(1);
			}
			printf("%d %d %d\n", width, height, nchannels);
			unsigned int mode = nchannels == 3 ? GL_RGB : GL_RGBA;
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, mode, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindVertexArray(0);
	}

	void cleanup() {
		// Deallocate the resources
		glDeleteProgram(program);
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	void render() {
		glUseProgram(program);
		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

		glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(RV::_projection));
		glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(RV::_view));
		glUniform1f(glGetUniformLocation(program, "_rotation"), rotation);
		glUniform1f(glGetUniformLocation(program, "_exposureMin"), exposureMin);
		glUniform1f(glGetUniformLocation(program, "_exposureMax"), exposureMax);
		glUniform4f(glGetUniformLocation(program, "_tint"), tint.x, tint.y, tint.z, 1);

		glDrawArrays(GL_POINTS, 0, 1);
	}
};

#pragma region PHONG

struct Texture {
	int width = 1024, height = 1024, nrChannels = 4;
	std::string path;
	unsigned char* data;
	unsigned int texture;
	void Load() {
		std::cout << "Loading texture: " << path << std::endl;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		std::cout << "		size:" << std::to_string(width) << "x" << std::to_string(height) << std::endl;
		std::cout << "		channels: " << std::to_string(nrChannels) << std::endl;
		if (data)
		{
			switch (nrChannels) {
			case 1:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
				//glGenerateMipmap(GL_TEXTURE_2D);
				break;
			case 2:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, data);
				//glGenerateMipmap(GL_TEXTURE_2D);
				break;
			case 3:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				//glGenerateMipmap(GL_TEXTURE_2D);
				break;
			case 4:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				//glGenerateMipmap(GL_TEXTURE_2D);
				break;
			}
		}
		else
		{
			std::cout << "		Failed to load texture" << std::endl;
		}
		stbi_image_free(data);
		std::cout << "		Texture loaded" << std::endl;
	}
};

struct PointLight {
	glm::vec3 color = { 1,1,1 };
	glm::vec3 position = { 0,2,-3 };
	glm::float32 strength = 10;
};
struct DirectionalLight {
	glm::vec3 color = { 1, 1, 1 };
	glm::vec3 direction = { 0,1,0 };
	glm::float32 strength = 0.5f;
};

const int MAX_LIGHTS = 16;
std::vector<PointLight> lights;
DirectionalLight mainLight;


struct Location {
	glm::vec3 position = { 0,0,0 };
	glm::vec3 rotation = { 0,0,0 };
	glm::vec3 scale = { 1,1,1 };
	Location(glm::vec3 position_ = { 0,0,0 }, glm::vec3 rotation_ = { 0,0,0 }, glm::vec3 scale_ = { 1,1,1 }) : position(position_), rotation(rotation_), scale(scale_){

	}
};
class Object {
	//const char* path = "cube.3dobj";
	std::string path;
	std::string name;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec2> uvs;
	std::vector <glm::vec3> normals;
	std::vector <glm::vec3> tangents;

	GLuint vao;
	GLuint vbo[4];
	GLuint shaders[2];
	GLuint program;

	glm::mat4 objMat;

public:
	float preScaler = 1;
	std::vector<Location> locations;

	glm::vec3 colorAmbient = { 0,0,0 };
	glm::vec3 colorDiffuse = { 1,1,1 };
	glm::vec3 colorSpecular = { 1,1,1 };
	glm::vec4 tilingOffset = { 1,1,0,0 };
	glm::float32 specularStrength = 1;
	glm::float32 normalStrength = 1;
	glm::float32 alphaCutout = 0;



	Texture albedo;
	Texture normal;
	Texture specular;
	Texture emissive;

	void setupObject() {
		bool res = FileLoader::LoadOBJ(path.c_str(), vertices, uvs, normals);
		computeTB();

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(4, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), &tangents[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		shaders[0] = compileShaderFromFile("resources/phong.vert", GL_VERTEX_SHADER, "objectVertexShader");
		shaders[1] = compileShaderFromFile("resources/phong.frag", GL_FRAGMENT_SHADER, "objectFragmentShader");

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);

		albedo.Load();
		normal.Load();
		specular.Load();
		emissive.Load();


		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_UVs");
		glBindAttribLocation(program, 3, "in_Tangent");
		linkProgram(program);

		objMat = glm::mat4(1.f);

	}
private:
	void computeTB() {
		for (int i = 0; i < vertices.size(); i += 3) {

			// Shortcuts for vertices
			glm::vec3 v0 = vertices[i + 0];
			glm::vec3 v1 = vertices[i + 1];
			glm::vec3 v2 = vertices[i + 2];

			// Shortcuts for UVs
			glm::vec2 uv0 = uvs[i + 0];
			glm::vec2 uv1 = uvs[i + 1];
			glm::vec2 uv2 = uvs[i + 2];

			// Edges of the triangle : postion delta
			glm::vec3 deltaPos1 = v1 - v0;
			glm::vec3 deltaPos2 = v2 - v0;

			// UV delta
			glm::vec2 deltaUV1 = uv1 - uv0;
			glm::vec2 deltaUV2 = uv2 - uv0;

			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
			glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;

			tangents.push_back(tangent);
			tangents.push_back(tangent);
			tangents.push_back(tangent);

		}
	}

public:
	Object(const std::string & name_ = "cube", const std::string & path_ = "resources/models/cube.3dobj") : name(name_), path(path_) {
	}
	~Object() {
		cleanupObject();
	}

	void updateObject(Location location_) {
		glm::mat4 t = glm::translate(glm::mat4(), location_.position);
		glm::mat4 r = glm::toMat4(glm::fquat(glm::radians(location_.rotation)));
		glm::mat4 s = glm::scale(glm::mat4(), location_.scale * preScaler);
		objMat = t * r * s;
	}

	void drawObjects() {
		for (size_t i = 0; i < locations.size(); i++)
		{
			updateObject(locations[i]);
			drawObject();
		}
	}

	void drawObject() {
		glBindVertexArray(vao);
		glUseProgram(program);

		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(program, "_color_ambient"), colorAmbient.x, colorAmbient.y, colorAmbient.z, 0);
		glUniform4f(glGetUniformLocation(program, "_color_diffuse"), colorDiffuse.x, colorDiffuse.y, colorDiffuse.z, 0);
		glUniform4f(glGetUniformLocation(program, "_color_specular"), colorSpecular.x, colorSpecular.y, colorSpecular.z, 0);
		glUniform4f(glGetUniformLocation(program, "_tilingOffset"), tilingOffset.x, tilingOffset.y, tilingOffset.z, tilingOffset.w);
		glUniform1f(glGetUniformLocation(program, "_specular_strength"), specularStrength);
		glUniform1f(glGetUniformLocation(program, "_normal_strength"), normalStrength);
		glUniform1f(glGetUniformLocation(program, "_alphaCutout"), alphaCutout);

		glUniform1i(glGetUniformLocation(program, "_albedo"), 0);
		glUniform1i(glGetUniformLocation(program, "_normal"), 1);
		glUniform1i(glGetUniformLocation(program, "_specular"), 2);
		glUniform1i(glGetUniformLocation(program, "_emissive"), 3);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo.texture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normal.texture);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, specular.texture);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, emissive.texture);

		for (size_t i = 0; i < MAX_LIGHTS; i++)
		{
			std::string name = "lights[";
			name += std::to_string(i);
			name += "].enabled";
			glUniform1i(glGetUniformLocation(program, name.c_str()), 0);
		}
		for (size_t i = 0; i < lights.size(); i++)
		{
			std::string name = "lights[";
			name += std::to_string(i);
			name += "].enabled";
			glUniform1i(glGetUniformLocation(program, name.c_str()), 1);
			name = "lights[";
			name += std::to_string(i);
			name += "].position";
			glUniform4f(glGetUniformLocation(program, name.c_str()), lights.at(i).position.x, lights.at(i).position.y, lights.at(i).position.z, 0);
			name = "lights[";
			name += std::to_string(i);
			name += "].color";
			glUniform4f(glGetUniformLocation(program, name.c_str()), lights.at(i).color.x, lights.at(i).color.y, lights.at(i).color.z, 0);
			name = "lights[";
			name += std::to_string(i);
			name += "].strength";
			glUniform1f(glGetUniformLocation(program, name.c_str()), lights.at(i).strength);
		}

		glUniform4f(glGetUniformLocation(program, "_mainLight.direction"), mainLight.direction.x, mainLight.direction.y, mainLight.direction.z, 0);
		glUniform4f(glGetUniformLocation(program, "_mainLight.color"), mainLight.color.x, mainLight.color.y, mainLight.color.z, 0);
		glUniform1f(glGetUniformLocation(program, "_mainLight.strength"), mainLight.strength);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}

	void drawGUI() {
		ImGui::Begin(name.c_str());

		ImGui::Text("Material:");
		ImGui::ColorEdit3("Ambient", &colorAmbient[0]);
		ImGui::ColorEdit3("Diffuse", &colorDiffuse[0]);
		ImGui::ColorEdit3("Specular", &colorSpecular[0]);
		ImGui::SliderFloat("Specular Strength", &specularStrength, 0, 2);
		ImGui::SliderFloat("Normal Strength", &normalStrength, 0.1f, 1.05f, "%.3f", .5f);
		ImGui::DragFloat2("Tiling", &tilingOffset.x, 0.01f);
		ImGui::DragFloat2("Offset", &tilingOffset.z, 0.01f);
		ImGui::SliderFloat("Alpha cutout", &alphaCutout, -0.001f, 1.001f, "%.3f", .5f);
		ImGui::Spacing();
		ImGui::Text("Transforms:");

		int locationsCount = locations.size();
		ImGui::DragInt("Instances count", &locationsCount, .1f);
		for (size_t i = 0; i < locations.size(); i++)
		{
			ImGui::Spacing();
			std::string tempString = std::to_string(i + 1);
			tempString += " Position";
			ImGui::DragFloat3(tempString.c_str(), &locations[i].position.x, 0.01f);
			tempString = std::to_string(i + 1);
			tempString += " Rotation";
			ImGui::DragFloat3(tempString.c_str(), &locations[i].rotation.x);
			tempString = std::to_string(i + 1);
			tempString += " Scale";
			ImGui::DragFloat3(tempString.c_str(), &locations[i].scale.x, 0.01f);
		}
		if (locationsCount < 0)
			locationsCount = 0;
		if (locationsCount != locations.size()) {
			while (locationsCount > locations.size()) {
				locations.push_back(Location());
			}
			while (locationsCount < locations.size()) {
				locations.pop_back();
			}
		}
		ImGui::End();
	}

	void cleanupObject() {
		glDeleteBuffers(3, vbo);
		glDeleteVertexArrays(1, &vao);

		glDeleteProgram(program);
		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
	}
};

#pragma endregion


Skybox skybox;
std::map<std::string, Object> objects;

void ResetPanV() {
	RV::panv[0] = RV::initial_panv[0];
	RV::panv[1] = RV::initial_panv[1];
	RV::panv[2] = RV::initial_panv[2];

	RV::rota[0] = RV::initial_rota[0];
	RV::rota[1] = RV::initial_rota[1];
	RV::FOV = RV::Initial_FOV;
}

void GLinit(int width, int height) {
	srand(time(NULL));
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	Axis::setupAxis();
	//Cube::setupCube();

	/////////////////////////////////////////////////////TODO
	// Do your init code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
	{
		objects["Camaro"] = Object("Camaro", "resources/models/Camaro.obj");
		objects["Camaro"].albedo.path = "resources/textures/Camaro/Camaro_AlbedoTransparency.png";
		objects["Camaro"].alphaCutout = .9f;
		objects["Camaro"].normal.path = "resources/textures/Camaro/Camaro_Normal_xs.png";
		objects["Camaro"].specular.path = "resources/textures/Camaro/Camaro_SpecularGlossiness.png";
		objects["Camaro"].emissive.path = "resources/textures/Camaro/Camaro_Emissive_md.png";
		objects["Camaro"].preScaler = 0.02f;
		objects["Camaro"].locations.push_back(Location());
		objects["Camaro"].setupObject();
	}
	{
		objects["Bush"] = Object("Bush", "resources/models/Bush.3dobj");
		objects["Bush"].albedo.path = "resources/textures/Bush/Bush_Diffuse.png";
		objects["Bush"].alphaCutout = .001f;
		objects["Bush"].normal.path = "resources/textures/Bush/Bush_Normal.png";
		objects["Bush"].specular.path = "resources/textures/Bush/Bush_SpecularGlossines.png";
		objects["Bush"].preScaler = 0.1f;
		objects["Bush"].locations.push_back(Location());
		objects["Bush"].setupObject();
	}
	{
		objects["Floor"] = Object("Floor", "resources/models/Floor.3dobj");
		objects["Floor"].albedo.path = "resources/textures/Floor/Floor_BaseColor.png";
		objects["Floor"].normal.path = "resources/textures/Floor/Floor_Normal.png";
		objects["Floor"].specular.path = "resources/textures/Floor/Floor_SpecularSmoothness.png";
		objects["Floor"].preScaler = 0.05f;
		const int floorWidth = 20;
		const int floorLength = 10;
		float posX = -(floorWidth / 2.f) * 5;
		float posY = -(floorLength / 2.f) * 5;
		for (size_t i = 0; i < floorWidth; i++)
		{
			posY = -(floorLength / 2.f);
			posX += 5;
			for (size_t j = 0; j < floorLength; j++)
			{
				objects["Floor"].locations.push_back(Location({ posX, 0, posY }));
				posY += 5;
			}
		}
		objects["Floor"].setupObject();
	}
	//parterre = new Object("Parterre", "resources/models/Parterre.3dobj");
	//parterre->albedo.path = "resources/textures/Parterre/Bush_Diffuse.png";
	//parterre->normal.path = "resources/textures/Parterre/Bush_Normal.png";
	//parterre->specular.path = "resources/textures/Parterre/Bush_SpecularGlossines.png";
	//parterre->emissive.path = "resources/textures/Parterre/Bush_SpecularGlossines.png";
	//parterre->scale = { .01f,.01f,.01f };
	//parterre->setupObject();


	lights.push_back(PointLight());

	skybox.init();
	skybox.exposureMin = -.15f;
	skybox.exposureMax = .3f;
	skybox.tint = { 1, .8f, .7f };

	mainLight.color = { 1, .8f, .5f };
	mainLight.direction = { 0,0.5f,-0.866f };


}

void GLcleanup() {
	Axis::cleanupAxis();
	skybox.cleanup();
	objects.clear();
}


void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::UpdateProjection();

	RV::_modelView = glm::mat4(1.f);

	RV::_modelView = glm::translate(RV::_modelView,
		glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1],
		glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0],
		glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	RV::_view = glm::mat4(glm::mat3(RV::_modelView));

	glDepthMask(GL_FALSE);
	skybox.render();
	glDepthMask(GL_TRUE);



	for (size_t i = 0; i < lights.size(); i++)
	{
		Axis::drawAxis(lights.at(i).position, 0.01f);
	}
	{
		for (std::map<std::string, Object>::iterator it = objects.begin(); it != objects.end(); ++it)
		{
			it->second.drawObjects();
		}

	}



	ImGui::Render();
}


void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("center of action\n\tx: %.3f\n\ty: %.3f\n\tz: %.3f", RV::panv[0], RV::panv[1], RV::panv[2]);
		ImGui::Text("center of rotation\n\tx: %.3f\n\ty: %.3f\n\tz: %.3f", RV::rota[0], RV::rota[1], RV::rota[2]);

		{
			ImGui::Text("Camera:");

			ImGui::Text("FOV = %.3f", glm::degrees(RV::FOV));
			float currentFov = glm::degrees(RV::FOV);
			ImGui::SliderFloat("FOV", &currentFov, 1, 179);
			if (currentFov != glm::degrees(RV::FOV)) {
				RV::FOV = glm::radians(currentFov);
			}
			if (ImGui::Button("reset transform")) {
				ResetPanV();
			}
		}
		{
			ImGui::NewLine();
			ImGui::Text("Enviroment");
			ImGui::DragFloat("Exposure min", &skybox.exposureMin, 0.01f);
			ImGui::DragFloat("Exposure max", &skybox.exposureMax, 0.01f);
			ImGui::ColorEdit3("Tint", &skybox.tint[0], 0.01f);
			ImGui::Spacing();
			ImGui::Text("Main light");
			ImGui::DragFloat3("Direction", &mainLight.direction[0], .01f);
			ImGui::ColorEdit3("Color", &mainLight.color[0], .01f);
			ImGui::DragFloat("Strength", &mainLight.strength, .01f);
			mainLight.direction = glm::normalize(mainLight.direction);
			ImGui::Spacing();
			ImGui::Text("Point lights");
			int previousLightSize = lights.size();
			int newLightsSize = lights.size();
			ImGui::SliderInt("Count", &newLightsSize, 0, MAX_LIGHTS);
			for (size_t i = 0; i < lights.size(); i++)
			{
				ImGui::Spacing();
				std::string name = std::to_string(i);
				name += " position";
				ImGui::DragFloat3(name.c_str(), &lights.at(i).position[0], .01f);
				name = std::to_string(i);
				name += " color";
				ImGui::ColorEdit3(name.c_str(), &lights.at(i).color[0], .01f);
				name = std::to_string(i);
				name += " strength";
				ImGui::DragFloat(name.c_str(), &lights.at(i).strength, .01f);
			}
			if (previousLightSize != newLightsSize) {
				if (previousLightSize > newLightsSize) {
					while (previousLightSize != newLightsSize) {
						lights.pop_back();
						newLightsSize++;
					}
				}
				else {
					while (previousLightSize != newLightsSize) {
						PointLight newLight;
						newLight.strength = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 10;
						newLight.color.x = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
						newLight.color.y = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
						newLight.color.z = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
						newLight.position.x = ((static_cast <float> (rand()) * 2 / static_cast <float> (RAND_MAX)) - 1) * 2;
						newLight.position.y = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX))) * 2 + 1;
						newLight.position.z = ((static_cast <float> (rand()) * 2 / static_cast <float> (RAND_MAX)) - 1) * 2;
						lights.push_back(newLight);
						newLightsSize--;
					}
				}
			}
		}

	}
	// .........................

	ImGui::End();

	for (std::map<std::string, Object>::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		it->second.drawGUI();
	}
	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}