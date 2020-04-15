#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>
#include <vector>
#include <stdlib.h>
#include <time.h>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"
#include "../FileLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
	void setupAxis();
	void cleanupAxis();
	void drawAxis(glm::vec3 position_ = {0,0,0});
}
////////////////

namespace RenderVars {
	const float Initial_FOV = glm::radians(65.f);
	float FOV = glm::radians(65.f);
	const float zNear = 0.001f;
	const float zFar = 500.f;
	float autoDollyVel = 4.f;
	int DollyDirection = -1;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	const float initial_panv[3] = { 0.f, -2.5f, -5.f };
	const float MAX_panv[3] = { 0.f, -2.5f, -40.f };
	const float initial_rota[2] = { 0.f, 0.f };

	float panv[3] = { initial_panv[0],initial_panv[1], initial_panv[2] };
	float rota[2] = { 0.f, 0.f };
	float width;

	bool useDolly = false;
	bool lastDollyState = false;
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button && !RV::useDolly) {
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

//////////////////////////////////////////////////
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

GLuint compileShaderFromFile(const char* shaderPath, GLenum shaderType, const char* name = ""){
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

		AxisShader[0] = compileShaderFromFile("resources/shaders/axis.vert", GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShaderFromFile("resources/shaders/axis.frag", GL_FRAGMENT_SHADER, "AxisFrag");

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
	void drawAxis(glm::vec3 position_) {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glUniform3f(glGetUniformLocation(AxisProgram, "_position"), position_.x, position_.y, position_.z);
		glUniform1f(glGetUniformLocation(AxisProgram, "_scale"), 1);
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
	GLuint cubeVao;
	GLuint cubeVbo[3];
	GLuint cubeShaders[2];
	GLuint cubeProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	extern const float halfW = 0.5f;
	int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

						   //   4---------7
						   //  /|        /|
						   // / |       / |
						   //5---------6  |
						   //|  0------|--3
						   //| /       | /
						   //|/        |/
						   //1---------2

	glm::vec3 verts[] = {
		glm::vec3(-halfW, -halfW, -halfW),
		glm::vec3(-halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW, -halfW),
		glm::vec3(-halfW,  halfW, -halfW),
		glm::vec3(-halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW, -halfW)
	};
	glm::vec3 norms[] = {
		glm::vec3(0.f, -1.f,  0.f),
		glm::vec3(0.f,  1.f,  0.f),
		glm::vec3(-1.f,  0.f,  0.f),
		glm::vec3(1.f,  0.f,  0.f),
		glm::vec3(0.f,  0.f, -1.f),
		glm::vec3(0.f,  0.f,  1.f)
	};

	glm::vec3 cubeVerts[] = {
		verts[1], verts[0], verts[2], verts[3],
		verts[5], verts[6], verts[4], verts[7],
		verts[1], verts[5], verts[0], verts[4],
		verts[2], verts[3], verts[6], verts[7],
		verts[0], verts[4], verts[3], verts[7],
		verts[1], verts[2], verts[5], verts[6]
	};
	glm::vec3 cubeNorms[] = {
		norms[0], norms[0], norms[0], norms[0],
		norms[1], norms[1], norms[1], norms[1],
		norms[2], norms[2], norms[2], norms[2],
		norms[3], norms[3], norms[3], norms[3],
		norms[4], norms[4], norms[4], norms[4],
		norms[5], norms[5], norms[5], norms[5]
	};
	GLubyte cubeIdx[] = {
		0, 1, 2, 3, UCHAR_MAX,
		4, 5, 6, 7, UCHAR_MAX,
		8, 9, 10, 11, UCHAR_MAX,
		12, 13, 14, 15, UCHAR_MAX,
		16, 17, 18, 19, UCHAR_MAX,
		20, 21, 22, 23, UCHAR_MAX
	};

	void setupCube() {
		glGenVertexArrays(1, &cubeVao);
		glBindVertexArray(cubeVao);
		glGenBuffers(3, cubeVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glPrimitiveRestartIndex(UCHAR_MAX);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cubeShaders[0] = compileShaderFromFile("resources/shaders/cube_org.vert", GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileShaderFromFile("resources/shaders/cube_org.frag", GL_FRAGMENT_SHADER, "cubeFrag");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		glBindAttribLocation(cubeProgram, 0, "in_Position");
		glBindAttribLocation(cubeProgram, 1, "in_Normal");
		linkProgram(cubeProgram);
	}
	void cleanupCube() {
		glDeleteBuffers(3, cubeVbo);
		glDeleteVertexArrays(1, &cubeVao);

		glDeleteProgram(cubeProgram);
		glDeleteShader(cubeShaders[0]);
		glDeleteShader(cubeShaders[1]);
	}
	void updateCube(const glm::mat4& transform) {
		objMat = transform;
	}
	void drawCube() {
		glEnable(GL_PRIMITIVE_RESTART);

		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), .1f, 1.f, 1.f, 0.f);
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

////////////////////////////////////////////////// BILLBORD
namespace Billboard {
	float width, height;
	glm::mat4 BillboardMat;

	GLuint vao;
	GLuint vbo;
	GLuint shaders[3];
	GLuint program;
	GLuint textureID;

	float BillboardVert[] = {
		0.f, 1.f, 0.f
	};
	void setup() {

		shaders[0] = compileShaderFromFile("resources/shaders/billboard.vert", GL_VERTEX_SHADER, "billboardVert");
		shaders[1] = compileShaderFromFile("resources/shaders/billboard.geom", GL_GEOMETRY_SHADER, "billboardGeom");
		shaders[2] = compileShaderFromFile("resources/shaders/billboard.frag", GL_FRAGMENT_SHADER, "billboardFrag");
		
		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);
		glAttachShader(program, shaders[2]);
		linkProgram(program);

		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
		glDeleteShader(shaders[2]);

		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(
			"resources/textures/700x420_arbol-mundo.jpg",
			&width,
			&height,
			&channels,
			0
		);

		if (data == NULL)
		{
			fprintf(stderr, "cannot load texture");
		}
		else
		{
			//load image to CPU
			glGenTextures(1, &textureID); //create the handle of the texture
			glBindTexture(GL_TEXTURE_2D, textureID); //Bind it

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// USE MIPMAPS
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			// USE MIPMAPS chess
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glTexImage2D(
				GL_TEXTURE_2D, //Target
				0, // Level of midmap (0 = base)
				GL_RGB, //how to store the texture (GPU)
				width,
				height,
				0,	//Legacy stuff
				GL_RGB,	// format, datatype, and actual data
				GL_UNSIGNED_BYTE,
				data //where the data is stored
			);
			// GENERATE MIPMAPS
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		stbi_image_free(data);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glBufferData(GL_ARRAY_BUFFER, sizeof(BillboardVert), BillboardVert, GL_STATIC_DRAW);

		glVertexAttribPointer(
			0, // Set the location for this attribute to 0 (same that we specified in the shader)
			3, // Size of the vertex attribute. In this case, 3 coordinates x, y, z
			GL_FLOAT,
			GL_FALSE, // Don't normalize the data.
			3 * sizeof(float),  // stride (space between consecutive vertex attributes)
			(void*)0 // offset of where the position data begins in the buffer (in this case 0)
		);

		// Once the attribute is specified, we enable it. The parameter is the location of the attribute
		glEnableVertexAttribArray(0);


		// Not necessary, but recomendable as it ensures that we will not use the VAO accidentally.
		glBindVertexArray(0);
	}

	void cleanup() {
		// Deallocate the resources
		glDeleteProgram(program);
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}

	void Update(glm::mat4 matrix) {
		BillboardMat = matrix;
	}

	void render() {
		glUseProgram(program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Fill the triangle
		//glDrawArrays(GL_LINE_LOOP, 0, 3); // Don't fill the triangle
	}
	/*
	void draw() {
		glUseProgram(program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Fill the triangle

		GLfloat param = sin(GL_CURRENT_TIME_NV);
		glUniform1f(glGetUniformLocation(program, "param"), param);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}*/

};

/////////////////////////////////////////////////

struct Texture {
	int width = 1024, height = 1024, nrChannels = 4;
	std::string path;
	unsigned char* data;
	unsigned int texture;
	void Load() {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			switch (nrChannels) {
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
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);
	}
};

struct Light {
	glm::vec3 color = { 1,1,1 };
	glm::vec3 position = { 0,5,2 };
	glm::float32 strength = 10;
};

const int MAX_LIGHTS = 16;
std::vector<Light> lights;

class Object {
	//const char* path = "cube.3dobj";
	const std::string path;
	const std::string name;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec2> uvs;
	std::vector <glm::vec3> normals;
	std::vector <glm::vec3> tangents;
	std::vector <glm::vec3> bitangents;

	GLuint vao;
	GLuint vbo[5];
	GLuint shaders[2];
	GLuint program;

	glm::mat4 objMat;

public:
	glm::vec3 colorAmbient = { 0,0,0 };
	glm::vec3 colorDiffuse = { 1,1,1 };
	glm::vec3 colorSpecular = { 1,1,1 };
	glm::float32 specularStrength = 1;

	Texture albedo;
	Texture normal;
	Texture specular;

private:
	void setupObject() {
		bool res = FileLoader::LoadOBJ(path.c_str(), vertices, uvs, normals);
		computeTB();

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(5, vbo);

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
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), &tangents[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), &bitangents[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)4, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(4);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		shaders[0] = compileShaderFromFile("resources/shaders/phong.vert", GL_VERTEX_SHADER, "objectVertexShader");
		shaders[1] = compileShaderFromFile("resources/shaders/phong.frag", GL_FRAGMENT_SHADER, "objectFragmentShader");

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);

		albedo.path = "resources/textures/Metal_AlbedoTransparency.png";
		albedo.nrChannels = 3;
		albedo.Load();
		normal.path = "resources/textures/Metal_Normal.png";
		normal.nrChannels = 3;
		normal.Load();
		specular.path = "resources/textures/Metal_SpecularSmoothness.png";
		specular.Load();


		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_UVs");
		glBindAttribLocation(program, 3, "in_Tangent");
		glBindAttribLocation(program, 4, "in_BiTangent");
		linkProgram(program);

		objMat = glm::mat4(1.f);

	}
	void computeTB() {
		for (int i = 0; i < vertices.size(); i += 3) {

			// Shortcuts for vertices
			glm::vec3& v0 = vertices[i + 0];
			glm::vec3& v1 = vertices[i + 1];
			glm::vec3& v2 = vertices[i + 2];

			// Shortcuts for UVs
			glm::vec2& uv0 = uvs[i + 0];
			glm::vec2& uv1 = uvs[i + 1];
			glm::vec2& uv2 = uvs[i + 2];

			// Edges of the triangle : postion delta
			glm::vec3 deltaPos1 = v1 - v0;
			glm::vec3 deltaPos2 = v2 - v0;

			// UV delta
			glm::vec2 deltaUV1 = uv1 - uv0;
			glm::vec2 deltaUV2 = uv2 - uv0;

			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
			glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
			glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

			tangents.push_back(tangent);
			tangents.push_back(tangent);
			tangents.push_back(tangent);

			// Same thing for binormals
			bitangents.push_back(bitangent);
			bitangents.push_back(bitangent);
			bitangents.push_back(bitangent);

		}
	}

public:
	Object(const std::string & name_ = "cube", const std::string & path_ = "resources/cube.3dobj") : name(name_), path(path_) {
		setupObject();
	}
	~Object() {
		cleanupObject();
	}


	void updateObject(glm::mat4 matrix) {
		objMat = matrix;
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
		glUniform1f(glGetUniformLocation(program, "_specular_strength"), specularStrength);

		glUniform1i(glGetUniformLocation(program, "_albedo"), 0);
		glUniform1i(glGetUniformLocation(program, "_normal"), 1);
		glUniform1i(glGetUniformLocation(program, "_specular"), 2);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo.texture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normal.texture);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, specular.texture);
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

Object* cubeObj;




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
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	Axis::setupAxis();
	Cube::setupCube();

	Billboard::setup();

	cubeObj = new Object();

	lights.push_back(Light());

	RV::width = tanf(RV::FOV / 2) * (glm::abs(RV::panv[2]));

}

void GLcleanup() {
	Axis::cleanupAxis();
	delete cubeObj;
}

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (RV::lastDollyState != RV::useDolly) {
		ResetPanV();
		RV::lastDollyState = RV::useDolly;
	}

	RV::_modelView = glm::mat4(1.f);

	RV::_modelView = glm::translate(RV::_modelView,
		glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1],
		glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0],
		glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	if (RV::useDolly) {
		//actualizar FOV
		GLint m_viewport[4];
		glGetIntegerv(GL_VIEWPORT, m_viewport);

		RV::FOV = tanf(RV::width / glm::abs(RV::panv[2])) * 2;
		RV::_projection = glm::perspective(RV::FOV, (float)m_viewport[2] / (float)m_viewport[3], RV::zNear, RV::zFar);

		float currentTime = ImGui::GetTime();

		if (RV::panv[2] >= RV::initial_panv[2])
		{
			RV::DollyDirection = -1;
			RV::autoDollyVel = 4;
		}
		else if(RV::panv[2] <= RV::MAX_panv[2] )
		{
			RV::DollyDirection = 1;
			RV::autoDollyVel = 4;
		}
		RV::panv[2] += RV::DollyDirection * RV::autoDollyVel * dt;
		RV::autoDollyVel += 1*dt;
	}

	Axis::drawAxis();
	for (size_t i = 0; i < lights.size(); i++)
	{
		Axis::drawAxis(lights.at(i).position);
	}

	glm::mat4 t = glm::translate(glm::mat4(), glm::vec3(0, 0, 0));
	glm::mat4 r = glm::mat4(1.f);
	float size = 1.f;
	glm::mat4 s = glm::scale(glm::mat4(), glm::vec3(size, size, size));
	cubeObj->updateObject(t * r * s);
	cubeObj->drawObject();
	t = glm::translate(glm::mat4(), glm::vec3(0, 10, 0));
	Billboard::Update(t * r * s);
	Billboard::render();

	//t = glm::translate(glm::mat4(), glm::vec3(-10, 0, -20));
	//r = glm::rotate(glm::mat4(), 25.f, glm::vec3(0, 1, 0));
	//cubeObj->updateObject(t * r * s);
	//cubeObj->drawObject();


	//t = glm::translate(glm::mat4(), glm::vec3(10, 0, -20));
	//r = glm::rotate(glm::mat4(), 25.f, glm::vec3(0, -1, 0));
	//cubeObj->updateObject(t * r * s);
	//cubeObj->drawObject();


	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("center of action\n\tx: %.3f\n\ty: %.3f\n\tz: %.3f", RV::panv[0], RV::panv[1], RV::panv[2]);
		ImGui::Text("center of rotation\n\tx: %.3f\n\ty: %.3f\n\tz: %.3f", RV::rota[0], RV::rota[1], RV::rota[2]);
		ImGui::Text("FOV = %.3f", glm::degrees(RV::FOV));
		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		// ...
		// ...
		// ...
		/////////////////////////////////////////////////////////
		//ImGui::DragFloat4("Color", &Object::color[0], .1f, 0, 0, "%.3f", .1f);
		//ImGui::DragFloat4("Light Pos", &Object::light[0], .1f, 0, 0, "%.3f", .1f);

		int previousLightSize = lights.size();
		int newLightsSize = lights.size();
		ImGui::SliderInt("Lights", &newLightsSize, 0, MAX_LIGHTS);
		for (size_t i = 0; i < lights.size(); i++)
		{
			ImGui::Spacing();
			std::string name = std::to_string(i);
			name += " position";
			ImGui::DragFloat3(name.c_str(), &lights.at(i).position[0], .01f);
			name = std::to_string(i);
			name += " color";
			ImGui::DragFloat3(name.c_str(), &lights.at(i).color[0], .01f);
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
					Light newLight;
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

		ImGui::NewLine();
		ImGui::SliderFloat("distance: ", &RV::panv[2], RV::initial_panv[2], RV::MAX_panv[2]);
		ImGui::SliderFloat("Dolly Velocity: ", &RV::autoDollyVel, 0.001f, 20.f);

		if (ImGui::Button("reset transform", ImVec2(140, 30))) {
			ResetPanV();
		}
		if (ImGui::Button("Invert Dolly", ImVec2(140, 30))) {
			RV::DollyDirection *= -1;
		}

		ImGui::Checkbox("Toogle Dolly", &RV::useDolly);
	}
	// .........................

	ImGui::End();

		cubeObj->drawGUI();
	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}