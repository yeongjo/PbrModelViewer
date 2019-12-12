#pragma once
#include "stdafx.h"

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "inc/tiny_obj_loader.h"
#endif

using namespace std;
using namespace glm;

#define print(fmt, ...) printf("[%s:%d:%s]" fmt "\n",__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__);
#define debug(fmt, ...) printf("Debug: " fmt "\n", ##__VA_ARGS__);
#define De2Ra(x) x*0.017453
#define EPSILON 0.0000001

float rc() {
	return rand() % 255 / 255.f;
}

char* readTxt(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fopen_s(&fptr, file, "rb"); // Open file for reading
	if (!fptr) // Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file
	length = ftell(fptr); // Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
	fclose(fptr); // Close the file
	buf[length] = 0; // Null terminator
	return buf; // Return the buffer
}

class VO;

VO* readObj(const char* file_path);


GLchar glErrorLog[512];
GLuint compliePartShader(char* ch, GLuint type, const char* errorName) {
	GLuint vs = glCreateShader(type);
	glShaderSource(vs, 1, &ch, NULL);
	glCompileShader(vs);

	GLint result;

	glGetShaderiv(vs, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		glGetShaderInfoLog(vs, 512, NULL, glErrorLog);
		std::cerr << errorName << glErrorLog << std::endl;
		return -1;
	}
	return vs;
}

GLuint complie(const char* vsPath, const char* fsPath, GLuint ShaderProgramID=0) {
	std::stringstream s_vs;
	s_vs << "shader/" << vsPath << ".vs";
	char* t_vs = readTxt(s_vs.str().c_str());

	std::stringstream s_fs;
	s_fs << "shader/" << fsPath << ".fs";
	char* t_fs = readTxt(s_fs.str().c_str());

	if (!t_vs) {
		printf("ERROR: [%s].vs 경로에 없음 컴파일 실패\n", vsPath);
	}
	if (!t_fs) {
		printf("ERROR: [%s].fs 경로에 없음 컴파일 실패\n", fsPath);
	}
	if (!t_vs || !t_fs) {
		return 0;
	}

	std::stringstream ss;
	ss << "ERROR: [" << vsPath << "] vertex shader 컴파일 실패 :\n";
	GLuint vs = compliePartShader(t_vs, GL_VERTEX_SHADER, ss.str().c_str());
	ss.clear(); ss.str("");
	ss << "ERROR: [" << vsPath << "] fragment shader 컴파일 실패 :\n";
	GLuint fs = compliePartShader(t_fs, GL_FRAGMENT_SHADER, ss.str().c_str());

	free(t_vs);
	free(t_fs);

	if(!ShaderProgramID)
		ShaderProgramID = glCreateProgram(); //실패시 0반환
	else {
		//glLinkProgram
	}
	glAttachShader(ShaderProgramID, vs); // 세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(ShaderProgramID, fs); // 세이더 프로그램에 프래그먼트 세이더 붙이기
	glLinkProgram(ShaderProgramID); // 세이더 프로그램 링크하기
	glDeleteShader(vs); // 세이더 프로그램에 링크하여 세이더 객체 자체는 삭제 가능
	glDeleteShader(fs);
	GLint result;
	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &result); // 세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(ShaderProgramID, 512, NULL, glErrorLog);
		std::cerr << "ERROR: [" << vsPath << "]shader program 연결 실패\n" << glErrorLog << std::endl;
		return false;
	}
	// 여러 개의 프로그램 만들 수 있고, 특정 프로그램을 사용하려면
	// glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다.
	// 사용하기 직전에 호출할 수 있다.
	// Always detach shaders after a successful link.
	glDetachShader(ShaderProgramID, vs);
	glDetachShader(ShaderProgramID, fs);

	return ShaderProgramID;
}

// 사용전에 bind를 반드시 호출해줘야한다.
class VO {
public:
	GLuint VAO = 0, VBO = 0, elementbuffer = 0;
	vector<vec3> vertex, color, normal;
	vector<vec2> uv;
	vector<uint> vertexIndices;

	GLshort drawStyle = GL_TRIANGLES;

	int verIdx = -1;

	bool isBind = false;

	//GLbyte mode = 0;
	//static const GLbyte color	= 0b0000'0001;//1
	//static const GLbyte normal	= 0b0000'0010;//2
	//static const GLbyte uv		= 0b0000'0100;//3

	static map<string, VO*> loadedObjs;
	
	static VO* loadObj(const char *path) {
		auto pr = loadedObjs.insert(std::pair<string, VO*>(path, NULL));
		if (pr.second == true) {
			pr.first->second = readObj(path);
		}
		return pr.first->second;
	}

	void remove() {
		if(VAO)
			glDeleteVertexArrays(1, &VAO);
		if(VBO)
			glDeleteBuffers(1, &VBO);
	}

	void bind() {
		remove();
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		size_t bufferOff = 0;
		size_t totalBufferSize = ((vertex.size()+ color.size()+ normal.size())*3 + uv.size()*2) * sizeof(float);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, totalBufferSize, 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertex.size() * sizeof(vec3), &vertex[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
		glEnableVertexAttribArray(0);
		bufferOff += vertex.size() * sizeof(vec3);
		if (color.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff, color.size() * sizeof(vec3), &color[0]);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)bufferOff);
			glEnableVertexAttribArray(1);
			bufferOff += color.size() * sizeof(vec3);
		}
		if (normal.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff, normal.size() * sizeof(vec3), &normal[0]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)bufferOff);
			glEnableVertexAttribArray(2);
			bufferOff += normal.size() * sizeof(vec3);
		}
		if (uv.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff, uv.size() * sizeof(vec2), &uv[0]);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)bufferOff);
			glEnableVertexAttribArray(3);
			bufferOff += uv.size() * sizeof(vec2);
		}

		isBind = true;
	}

	void render() {
		if (vertex.size() == 0) return;
		assert(isBind && "not binded VO used.");
		glBindVertexArray(VAO);
		if (verIdx == -1)
			verIdx = (int)vertexIndices.size();

		if (verIdx >= 3)
			;// glDrawElements(GL_TRIANGLES, 0, vertex.size());
		else
			glDrawArrays(drawStyle, 0, (GLsizei)vertex.size());
	}
};

map<string, VO*> VO::loadedObjs;

glm::vec3 operator+ (glm::vec2& a, glm::vec3& b) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}

glm::vec3 operator+ (glm::vec3& b, glm::vec2& a) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}

void vec3ToFloat(glm::vec3& a, float* b) {
	for (int i = 0; i < 3; i++)
		b[i] = a[i];
}

VO* readObj(const char* file_path) {
	VO* vo = new VO();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path);
	if (!ret) {
		print("can't load %s", file_path);
		assert(ret && file_path);
	}

	uint shape_size = (uint)shapes.size();
	for (size_t s = 0; s < shape_size; s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;

		size_t vert_size = shapes[s].mesh.num_face_vertices.size();
		for (size_t f = 0; f < vert_size; f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			//memcpy((void*)& vo->vertex[0], (void*)& attrib.vertices[0], attrib.vertices.size() * sizeof(float));

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				// Optional: vertex colors
				//tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
				//tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
				//tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];

				vo->vertex.push_back(vec3(vx, vy, vz));
				vo->normal.push_back(vec3(nx, ny, nz));
				vo->uv.push_back(vec2(tx, ty));
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}
	/*shapes.shrink_to_fit();
	materials.shrink_to_fit();*/
	return vo;
}


float cross(const vec2& a, const vec2& b) {
	return a.x * b.y - a.y * b.x;
}


// 출처: https://bowbowbow.tistory.com/17 [멍멍멍]
bool operator<(const vec2& a, const vec2& b) {
	return a.x < b.x && a.y < b.y;
}

/*
- 점 a, b를 지나는 직선과 점 c, d를 지나는 직선의 교점을 x에 반환한다.
- 두 직선이 평행이면(겹치는 경우 포함) 거짓을, 아니면 참을 반환한다.
*/
bool lineIntersection(const vec2& a, const vec2& b, const vec2& c, const vec2& d, vec2& retX) {
	float det = cross((b - a),(d - c)); //두선이 평행인 경우
	if(fabs(det) < EPSILON) return false;
	retX = a+(b-a)*(cross((c-a),(d-c))/det);
	return true;
}

// - p가 두 점 a, b를 감싸면서 각 변이 x, y축에 평행한 최소사각형 내부에 있는지 확인한다.
// a, b, p는 일직선 상에 있다고 가정한다.
bool inBoundingRectangle(const vec2& p, vec2& a, vec2& b) {
	vec2 t1(glm::min(a.x, b.x), glm::min(a.y, b.y));
	vec2 t2(glm::max(a.x, b.x), glm::max(a.y, b.y));
	return p == t1 || p == t2 || (t1 < p && p < t2);
}

// - 두 점 a, b를 지나는 선분과 두 점 c, b를 지나는 선분을 p에 반환한다.
// - 교짐이 여러개일 경우 아무점이나 반환한다.
bool segmentIntersection(vec2& a, vec2& b, vec2& c, vec2& d, vec2& retP) {
	//두 직선이 평행인 경우를 우선 예외로 처리한다.
	if (lineIntersection(a, b, c, d, retP))
		//p가 두 선분에 포함되어 있는 경우에만 참을 반환한다.
		return inBoundingRectangle(retP, a, b) && inBoundingRectangle(retP, c, d);
	return false;
}