#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "mGlHeader.h"
#include "../glm/gtx/string_cast.hpp"



GLclampf f() {
	return rand() % 255 / 255.0f;
}

vec3 randColor() {
	return vec3(f(), f(), f());
}



class Window {
	static Window* active;
public:
	uint w, h;
	uint halfWidth, halfHeight;
	float ratio;

	Window() {
		active = this;
	}

	static Window& get() {
		return *active;
	}

	void init(int w, int h) {
		this->w = w;
		this->h = h;
		ratio = w / (float)h;
		halfWidth = w / 2, halfHeight = h / 2;
	}
};


Window* Window::active = nullptr;


class TickObj;
class Scene {
public:
	set<TickObj*> objs;

	void tick(float dt);

	void render();

	void active() {
		activeScene = this;
	}

	static Scene* activeScene;

	static Scene& self() {
		if (activeScene == nullptr)
			activeScene = new Scene();
		return *activeScene;
	}

	static void addObj(TickObj* obj) {
		Scene::self().objs.insert(obj);
	}

	queue<TickObj*> removeObjs;
	static void removeObj(TickObj* obj) {
		Scene::self().removeObjs.push(obj);
	}
};


Scene* Scene::activeScene = nullptr;


class TickObj {
public:
	TickObj() { Scene::addObj(this); }
	virtual ~TickObj() {  }
	virtual void tick(float dt) = 0;
	virtual void render() = 0;
	virtual void initName(){}
};


void Scene::tick(float dt) {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end();++it) {
		(*it)->tick(dt);
	}
	while(Scene::self().removeObjs.size()) {
		auto t = removeObjs.front();
		objs.erase(t);
		delete t;
		removeObjs.pop();
	}
}

void Scene::render() {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); ++it) {
		(*it)->render();
	}
}


class Camera;

class CameraShaderUniformBuffer {
public:
	unsigned int UBO;
	mat4* p, * v, * vp;
	vec3* pos;

	void create() {
		glGenBuffers(1, &UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		//https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(mat4) * 3 + sizeof(vec4),
			NULL, GL_STATIC_DRAW); // mat4 3개, vec3 1개 할당
	}

	void setData(Camera& cam, Window& win);

	// 값변경될때마다 한번만 호출하면되나
	void bindBuffer() {
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), p);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), v);
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), vp);
		glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::vec3), pos); // 카메라 위치
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
	}
};

struct TextureBindInfo {
	unsigned int id;
	unsigned int activeIdx;
};

class IShaderUniform {
public:
	virtual void setName(unsigned shaderIdx, const char* name) = 0;
	virtual void setData(void* ptr) = 0;
	virtual void* getData() = 0;
	virtual void applyUniform() = 0;
	virtual void log() = 0;
};


template<class T>
class ShaderUniform : public IShaderUniform {
public:
	int uniformLocation = -2;
	string uniformName;
	T* valuePtr = nullptr; // 보낼 값이 있는 포인터
	bool isRef; // 다른걸 참조하는지: 한다면 지우면안되고 아니면 지워야함

	ShaderUniform(bool isRef) {
		this->isRef = isRef;
	}

	virtual ~ShaderUniform() {
		if (!isRef) delete valuePtr;
	}

	void setName(unsigned shaderIdx, const char* name) {
		uniformName = name;
		uniformLocation = glGetUniformLocation(shaderIdx, name);
	}
	
	// 값 설정
	void setData(void* ptr) {
		T* t = (T*)ptr;
		if (!isRef&&!valuePtr)
			t = new T(*t);
		setData(t);
	}

	void* getData() {
		return valuePtr;
	}

	// 쉐이더에 값 적용
	void applyUniform() {
		if (uniformLocation != -1) {
			applyUniformByType();
		}
	}

	void log() {
		debug("[id:%d]%s: %s", uniformLocation, uniformName.c_str(), glm::to_string(*valuePtr).c_str());
	}
private:
	void setData(T* ptr) {
		valuePtr = ptr;
	}

	void applyUniformByType();
};

template<>
void ShaderUniform<TextureBindInfo>::setData(void* ptr) {
	TextureBindInfo* t = (TextureBindInfo*)ptr;

	if (!isRef && !valuePtr)
		valuePtr = new TextureBindInfo(*t);
	valuePtr->id = t->id;
	valuePtr->activeIdx = t->activeIdx;

	//setData(t);
	glUniform1i(uniformLocation, t->activeIdx);
}

template<>
void ShaderUniform<TextureBindInfo>::log() {
	debug("[id:%d]%s: id: %d, bind: %d", uniformLocation, uniformName.c_str(), valuePtr->id, valuePtr->activeIdx - GL_TEXTURE0);
}
template<>
void ShaderUniform<int>::log() {
	debug("[id:%d]%s: %d", uniformLocation, uniformName.c_str(), valuePtr);
}

template<>
void ShaderUniform<mat4>::applyUniformByType() {
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (const float*)valuePtr);
}

template<>
void ShaderUniform<vec3>::applyUniformByType() {
	glUniform3fv(uniformLocation, 1, (GLfloat*)valuePtr);
}

template<>
void ShaderUniform<vec4>::applyUniformByType() {
	glUniform4fv(uniformLocation, 1, (GLfloat*)valuePtr);
}

template<>
void ShaderUniform<int>::applyUniformByType() {
	glUniform1i(uniformLocation, *(GLint*)valuePtr);
}

template<>
void ShaderUniform<TextureBindInfo>::applyUniformByType() {
	
	glActiveTexture(valuePtr->activeIdx);
	glBindTexture(GL_TEXTURE_2D, valuePtr->id);
}


//template<>
//void ShaderUniform<mat4>::log() {
//	debug("[%d]%s: %s", uniformLocation, uniformName, glm::to_string(*valuePtr));
//}
//
//template<>
//void ShaderUniform<vec3>::log() {
//	debug("[%d]%s: %s", uniformLocation, uniformName, glm::to_string(*valuePtr));
//}

class Texture {
public:
	int width, height, nrChannels;
	unsigned int id;
	void load(const char* path) {
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
		// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
		unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			assert("Failed to load texture: " && path && 0);
		}
		stbi_image_free(data);
	}

	//void setUniformToShader(Shader& shader, const char* name) {
	//	glUniform1i(glGetUniformLocation(shader.id, name), 0);
	//	// or set it via the texture class
	//	shader.addUniform(name, 1);
	//}
};

class Shader {
	

	unsigned int textureIdx = GL_TEXTURE0;
public:
	unsigned int id;
	map<string, IShaderUniform*> shaderUniforms;
	string shaderPath;

	static map<string, Shader*> shaders;

	void complieShader(const char* shaderPath) {
		auto find = shaders.insert(pair<string, Shader*>(shaderPath, this));
		if (!find.second) {
			assert(0 && shaderPath); //에 경로가 없음
		}
		id = ::complieShader(shaderPath);
		this->shaderPath = shaderPath;
		shaders.insert(pair<string, Shader*>(shaderPath, this));
	}

	void addUniform(const char* name, int ptr) {
		print("%s int add uniform", name);
		insertUniform(name, &ptr, false);
	}

	void addUniform(const char* name, mat4* ptr) {
		print("%s mat4 add Referentce uniform", name);
		insertUniform(name, ptr);
	}

	void addUniform(const char* name, vec3* ptr) {
		print("%s vec3 add Referentce uniform", name);
		insertUniform(name, ptr);
	}

	void addUniform(const char* name, vec4* ptr) {
		print("%s vec3 add Referentce uniform", name);
		insertUniform(name, ptr);
	}

	void addUniform(const char* name, mat4& ptr) {
		print("%s mat4 add uniform", name);
		insertUniform(name, &ptr, false);
	}

	void addUniform(const char* name, Texture& tex) {
		print("%s Texture add uniform", name);
		auto a = TextureBindInfo{ tex.id, textureIdx };
		insertUniform(name, &a, false);
		++textureIdx;
	}

	void removeUniform() {
		//TODO 나중에만들거
	}

	template<class T>
	void changeUniformValue(const char* name, T* ptr) {
		auto uniform = getUniform(name);
		if (uniform)
			uniform->setData(ptr);
	}

	void use() {
		glUseProgram(id);
		if (!shaderUniforms.empty())
			for (auto it = shaderUniforms.begin(); it != shaderUniforms.end(); it++) {
				it->second->applyUniform();
			}
	}

	// log용
	void logAllUniforms() {
		if (!shaderUniforms.empty())
			for (auto it = shaderUniforms.begin(); it != shaderUniforms.end(); it++) {
				it->second->log();
			}
		else
			debug("%s has no uniforms", shaderPath);
	}

	void logUniform(const char* name) {
		if (!shaderUniforms.empty()) {
			auto temp = getUniform(name);
			if (temp) { temp->log(); return; }
		}
		debug("%s has no uniforms", shaderPath);
	}

	// 이름으로 유니폼가져옴 없을시 null반환
	IShaderUniform* getUniform(const char* name) {
		auto findKey = shaderUniforms.find(name);
		if (findKey == shaderUniforms.end()) {
			return nullptr;
		}
		return findKey->second;
	}

private:
	template<class T>
	void insertUniform(const char* name, T* ptr, bool isRef = true) {
		auto uniform = getUniform(name);
		if (uniform) {
			//assert("넣으려는 유니폼이 이미 있음" && name && 0);
		}else {
			uniform = new ShaderUniform<T>(isRef);
		}
		uniform->setName(id, name);
		uniform->setData(ptr);
		shaderUniforms.insert(make_pair(name, uniform));
	}
};
map<string, Shader*> Shader::shaders;

template<>
void Shader::changeUniformValue<Texture>(const char* name, Texture* ptr) {
	auto uniform = getUniform(name);
	if (uniform) {
		auto a = TextureBindInfo{ ptr->id, ((TextureBindInfo*)uniform->getData())->activeIdx};
		uniform->setData(&a);
	}
}


class Obj : public TickObj {
protected:
	Obj* parentObj = nullptr;
public:
	VO* vo;

	Shader* shader = nullptr;

	glm::mat4 trans = glm::mat4(1.0f);
	vec4 color = vec4(1);

	string name;

	virtual void initName() {
		name = "Obj";
	}

	// call after this have verteies
	void loadObj(const char* path) {
		vo = VO::loadObj(path);
		vo->bind();
	}

	void destroy() {
		Scene::removeObj(this);
	}

	vec3& getPos(){
		return pos;
	}

	vec3& getRotation(){
		return rot;
	}

	vec3& getScale(){
		return scale;
	}

	void setPos(const vec3& p) {
		pos = p; bIsGetTransInThisTick = false;
	}
	void setRotation(const vec3& r) {
		rot = r; bIsGetTransInThisTick = false;
	}
	void setScale(const vec3& s) {
		scale = s; bIsGetTransInThisTick = false;
	}

	void setParent(Obj* parent) {
		assert(parent != this);
		parentObj = parent;
	}
	Obj* GetParent() {
		return parentObj;
	}

	void setShader(Shader* shader) {
		this->shader = shader;
		shader->addUniform("trans", &getTrans()); // TODO 낭비됨 계속 똑같은거 추가함
	}

	void rotateX(float x) {
		bIsGetTransInThisTick = false; rot.x += x;
	}
	void rotateY(float y) {
		bIsGetTransInThisTick = false; rot.y += y;
	}
	void rotateZ(float z) {
		bIsGetTransInThisTick = false; rot.z += z;
	}

	void updateTransform() {
		bIsGetTransInThisTick = false;
	}

	vec3 getForward() {
		vec3 forward;
		forward.x = sin(glm::radians(rot.y));
		forward.y = -tan(glm::radians(rot.x));
		forward.z = cos(glm::radians(rot.y));
		return forward;
	}

	glm::mat4& getTrans() {
		if (bIsGetTransInThisTick) return trans;
		trans = glm::mat4(1.0f);
		trans = glm::translate(trans, pos);
		trans = glm::scale(trans, scale);
		trans = glm::rotate(trans, glm::radians(rot.y), glm::vec3(0.0, 1, 0));
		trans = glm::rotate(trans, glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
		trans = glm::rotate(trans, glm::radians(rot.x), glm::vec3(1, 0.0, 0));
		if (parentObj)
			trans = parentObj->getTrans() * trans;
		bIsGetTransInThisTick = true;
		return trans;
	}

	virtual void tick(float dt) {
		updateTransform();
	}

	void applyColor(GLuint shaderId) {
		unsigned int loc = glGetUniformLocation(shaderId, "vcolor");
		glUniform4f(loc, color.x, color.y, color.z, color.w);
	}

	virtual void render() {
		if (shader) {
			shader->changeUniformValue("trans", &getTrans());
			shader->use();
			applyColor(shader->id);
		} else {
			assert(0 && name.c_str()); // shader 없음
		}
		vo->render();
	}

private:
	bool bIsGetTransInThisTick = false;
protected:
	vec3 pos = vec3(0);
	vec3 scale = vec3(1);
	vec3 rot = vec3(0);
};

class Camera : public Obj {
public:
	glm::vec3 armVector = glm::vec3(0, 0, -10);
	glm::vec3 target = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);

	float fov = 45;
	bool isPerspective = true;

	glm::mat4 v = glm::mat4(1.0f), p = glm::mat4(1.0f);
	glm::mat4 vp = glm::mat4(1.0f);

	static CameraShaderUniformBuffer* UBO;

	Camera() : Obj() {
		if (!UBO) {
			UBO = new CameraShaderUniformBuffer();
			UBO->create();
		}
		pos.z = -2;
	}

	virtual void tick(float dt) {
		bIsGetTransInThisTick = 0;
		bIsGetVPInThisTick = 0;
	}

	virtual void bind(Window& win) {
		UBO->setData(*this, win);
		UBO->bindBuffer();
	}

	glm::mat4& getTrans(Window& win) {
		if (bIsGetTransInThisTick) return vp;

		if (~bIsGetVPInThisTick & 0b10) {
			v = glm::lookAt(
				armVector, // 월드 공간에서 당신의 카메라 좌표
				target,   // 월드 스페이스에서 당신의 카메라가 볼 곳
				up        // glm::vec(0,1,0) 가 적절하나, (0,-1,0)으로 화면을 뒤집을 수 있습니다. 그래도 멋지겠죠
			);
			v = glm::rotate(v, glm::radians(rot.x), vec3(1, 0, 0));
			v = glm::rotate(v, glm::radians(rot.y), vec3(0, -1, 0));
			if (parentObj)
				v *= glm::inverse(parentObj->getTrans());
			v = glm::translate(v, -pos);
			bIsGetVPInThisTick |= 0b10;
		}
		if (~bIsGetVPInThisTick & 0b1) {
			if (isPerspective)
				p = glm::perspective(glm::radians(fov), win.ratio, 0.1f, 1000.f);
			else {
				float size = 5;
				p = glm::ortho<float>(-size * win.ratio, size * win.ratio, -size, size, 0.1, 50.f);
			}
			bIsGetVPInThisTick |= 0b1;
		}

		vp = p * v;
		bIsGetTransInThisTick = 1;
		return vp;
	}

	void translate(vec3 off) {
		vec3 right = glm::normalize(glm::cross(target - pos, up));
		vec3 forward = glm::normalize(glm::cross(up, right));
		vec3 moveOff = forward * off.z + right * off.x;
		pos += moveOff;
		target += moveOff;
	}

	void render() {}
	virtual void initName() {
		name = "Camera";
	}

private:
	uint bIsGetTransInThisTick = 0;
	uint bIsGetVPInThisTick = 0;
};


CameraShaderUniformBuffer* Camera::UBO = nullptr;


void CameraShaderUniformBuffer::setData(Camera& cam, Window& win) {
	this->p = &cam.p;
	this->v = &cam.v;
	this->vp = &cam.getTrans(win);
	this->pos = &cam.getPos();
}




enum EMouse {
	NONE,
	MOUSE_X, MOUSE_Y,
	MOUSE_OFF_X, MOUSE_OFF_Y,
	MOUSE_NORMALIZE_X, MOUSE_NORMALIZE_Y,
	MOUSE_NORMALIZE_OFF_X, MOUSE_NORMALIZE_OFF_Y, // 안만듬
	MOUSE_L_BUTTON,MOUSE_R_BUTTON,MOUSE_M_BUTTON,
	MOUSE_WHEEL,
	COUNT
};

void setMousePos(int x, int y);
class Input {
public:
	static char key[512];
	static int mouse[EMouse::COUNT];
	static bool isFirstMousePos;
	static vec2 moveVec;

	static POINT mousePos;

	static void getMousePos() {
		GetCursorPos(&mousePos);
		int x = mousePos.x, y = mousePos.y;
		if (Input::isFirstMousePos) {
			Input::mouse[EMouse::MOUSE_OFF_X] = 0;
			Input::mouse[EMouse::MOUSE_OFF_Y] = 0;
			Input::isFirstMousePos = false;
		} else {
			Input::mouse[EMouse::MOUSE_OFF_X] = Input::mouse[EMouse::MOUSE_X] - x;
			Input::mouse[EMouse::MOUSE_OFF_Y] = Input::mouse[EMouse::MOUSE_Y] - y;
		}
		setMousePos(x, y);
	}
};

char Input::key[512];
int Input::mouse[EMouse::COUNT];
bool Input::isFirstMousePos = true;
vec2 Input::moveVec;
POINT Input::mousePos;

void (*onKeyboardEvent)(unsigned char key, int x, int y, bool isDown) = NULL;

void Keyboard(unsigned char key, int x, int y) {
	//debug("Keyboard down: %c [x:%d, y:%d]", key, x, y);
	if (Input::key[key] == 1) return;
	switch (key) {
	case 'w':
		++Input::moveVec.y;
		break;
	case 's':
		--Input::moveVec.y;
		break;
	case 'a':
		--Input::moveVec.x;
		break;
	case 'd':
		++Input::moveVec.x;
		break;
	case 'q': glutLeaveMainLoop(); break;
	}
	Input::key[key] = 1;

	if (onKeyboardEvent != NULL)
		onKeyboardEvent(key, x, y, true);
}

void keyboardUp(unsigned char key, int x, int y) {
	//debug("Keyboard up: %c [x:%d, y:%d]", key, x, y);
	Input::key[key] = 0;
	switch (key) {
	case 'w':
		--Input::moveVec.y;
		break;
	case 's':
		++Input::moveVec.y;
		break;
	case 'a':
		++Input::moveVec.x;
		break;
	case 'd':
		--Input::moveVec.x;
		break;
	}

	if (onKeyboardEvent != NULL)
		onKeyboardEvent(key, x, y, false);
}

void specialInput(int key, int x, int y) {
	debug("specialInput: %c [x:%d, y:%d]", key, x, y);
	/*switch (key) {
	case GLUT_KEY_UP:
		obj.pos.y += 0.1f;
		break;
	case GLUT_KEY_DOWN:
		obj.pos.y -= 0.1f;

		break;
	case GLUT_KEY_LEFT:
		obj.pos.x -= 0.1f;

		break;
	case GLUT_KEY_RIGHT:
		obj.pos.x += 0.1f;

		break;
	}*/
}

void mouseWheel(int wheel, int direction, int x, int y) {
	debug("Mouse: wheel(%d), direction(%d)", wheel, direction);
	Input::mouse[EMouse::MOUSE_WHEEL] += direction;
}

void setMousePos(int x, int y) {
	float screenMouseX = (float)x / Window::get().halfWidth - 1;
	float screenMouseY = -(float)y / Window::get().halfHeight + 1;

	Input::mouse[EMouse::MOUSE_X] = x;
	Input::mouse[EMouse::MOUSE_Y] = y;

	Input::mouse[EMouse::MOUSE_NORMALIZE_X] = screenMouseX;
	Input::mouse[EMouse::MOUSE_NORMALIZE_Y] = screenMouseY;
}

// button: 0(left), 1(mid), 2(right), 3(scrollup), 4(scrolldown)
// state: 0(down), 1(up)
void mouseClickHandler(int button, int state, int x, int y) {
	debug("mouseClickHandler: button(%d), state(%d)", button, state);
	Input::mouse[EMouse::MOUSE_L_BUTTON + button] = !state;
	//setMousePos(x, y);
}

void mouseMotionHandler(int x, int y) {
	//debug("mouseMotionHandler: x(%d), y(%d)", x,y);
	
}

void bindInput() {
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutMouseFunc(mouseClickHandler);
	glutPassiveMotionFunc(mouseMotionHandler);
	glutSpecialFunc(specialInput);
	glutMouseWheelFunc(mouseWheel);

	Input::mouse[EMouse::MOUSE_WHEEL] = 0;
}





unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad() {
	if (quadVAO == 0) {
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}





mat4 mat4_1 = mat4(1);

class DebugMesh {
public:
	VO vo;
	char isDraw;
	vec3 color;
};

class Debug {
public:
	static vector<DebugMesh> mesh;
	static int drawIdx;

	static void drawLine(vec3 a, vec3 b, vec3 color=vec3(1)) {
		while (mesh.size() <= drawIdx) {
			mesh.push_back(DebugMesh());
			mesh.back().vo.drawStyle = GL_LINES;
			mesh.back().vo.vertex.push_back(a);
			mesh.back().vo.vertex.push_back(b);
			mesh.back().vo.bind();
		}
		glBindVertexArray(mesh[drawIdx].vo.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, mesh[drawIdx].vo.VBO);
		mesh[drawIdx].vo.vertex[0] = a;
		mesh[drawIdx].vo.vertex[1] = b;
		mesh[drawIdx].color = color;
		glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(vec3), &mesh[drawIdx].vo.vertex[0]);
		++drawIdx;
	}

	static void render() {
		auto unlit = Shader::shaders["unlit"];
		unlit->changeUniformValue("trans", &mat4_1);

		for (size_t i = 0; i < drawIdx; i++){
			unlit->changeUniformValue("color", &mesh[i].color);
			unlit->use();
			mesh[i].vo.render();
		}
		drawIdx = 0;
	}
};

vector<DebugMesh> Debug::mesh;
int Debug::drawIdx = 0;


