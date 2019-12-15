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

void renderCube();


class Window {
	static Window* active;
	uint w, h;
	uint halfWidth, halfHeight;
	float ratio;
public:
	Window(int w, int h) {
		active = this;
		init(w, h);
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

	void resetViewport() {
		glViewport(0, 0, w, h);
	}
public:
    uint getW() const { return w; }

    uint getH() const { return h; }

    uint getHalfWidth() const { return halfWidth; }

    uint getHalfHeight() const { return halfHeight; }

    float getRatio() const { return ratio; }

};


Window* Window::active = nullptr;


class TickObj;
class Scene {
public:
	vector<TickObj*> objs;

	void tick(float dt);
	void render();
	void gui();

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
		Scene::self().objs.push_back(obj);
	}
};


Scene* Scene::activeScene = nullptr;


class TickObj {
	bool bIsRemoved = false;
	static int uniqueIdCounter;
protected:
	int uniqueId;
public:
	string name;

	TickObj() { uniqueId = uniqueIdCounter++;  Scene::addObj(this); }
	void remove(){ bIsRemoved = true; }
	bool getIsRemoved() const { return bIsRemoved; }
	virtual void tick(float dt) = 0;
	virtual void render() = 0;

	virtual bool guiStart() {
		auto tree = ImGui::TreeNode((std::to_string(uniqueId) + name).c_str());
		
		if (tree) {
			ImGui::SameLine(150);
			if (ImGui::Button("delete")) {
				this->remove();
			}
			return true;
		}
		return false;
	}

	virtual void gui() {
		if (guiStart()) {
			ImGui::TreePop();
		}
	}
};
int TickObj::uniqueIdCounter = 0;


void Scene::tick(float dt) {
	auto size = objs.size();
	for (size_t i = 0; i < size; ++i) {
		auto cache = objs[i];
		if (cache->getIsRemoved()) {
			--size;
			objs.erase(objs.begin() + i);
			delete cache;
			continue;
		}

		cache->tick(dt);
	}
}

void Scene::render() {
	auto size = objs.size();
	for (size_t i = 0; i < size; ++i) {
		objs[i]->render();
	}
}

void Scene::gui() {
	auto size = objs.size();
	for (size_t i = 0; i < size; ++i) {
		objs[i]->gui();
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
		//UBO에 대한 설명
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
		glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::vec4), pos); // 카메라 위치
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
	}
};


class Texture {
protected:
	int width, height, nrChannels;
	unsigned int id;
	static map<string, Texture*> loadedTextures;
public:
	const int getWidth() const { return width; }
	const int getHeight() const { return height; }
	const int getNrChannels() const { return nrChannels; }
	const unsigned int getId() const { return id; }

	static Texture* get(const char* name) {
		auto iter = loadedTextures.find(name);
		if (iter != loadedTextures.end())
			return iter->second;
		return nullptr;
	}

	static Texture* load(const char* path, unsigned type= GL_RGB, unsigned tagetType= GL_RGB) {
		auto loaded = get(path);
		if (loaded) {
			return loaded;
		}
		auto temp = new Texture();
		auto& id = temp->id;
		auto& width = temp->width;
		auto& height = temp->height;
		auto& nrChannels = temp->nrChannels;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
		// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
		unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
		if (data) {
			if(nrChannels == 3)
				glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, tagetType, GL_UNSIGNED_BYTE, data);
			else if (nrChannels == 4)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			assert("Failed to load texture: " && path && 0);
		}
		stbi_image_free(data);
		loadedTextures[path] = temp;
		return temp;
	}

	static Texture* loadHDR(const char* path) {
		auto loaded = get(path);
		if (loaded) {
			return loaded;
		}
		auto temp = new Texture();
		auto& id = temp->id;
		auto& width = temp->width;
		auto& height = temp->height;
		auto& nrChannels = temp->nrChannels;
		stbi_set_flip_vertically_on_load(true);
		float* data = stbi_loadf(path, &width, &height, &nrChannels, 0);
		if (data) {
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		} else {
			std::cout << "Failed to load HDR image." << std::endl;
		}
		loadedTextures[path] = temp;
		return temp;
	}

	static Texture* createEmpty(int width, int height, unsigned type= GL_RGB16F, unsigned color=GL_RGB) {
		auto temp = new Texture;
		temp->width = width;
		temp->height = height;
		glGenTextures(1, &temp->id);
		glBindTexture(GL_TEXTURE_2D, temp->id);
		glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, color, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		return temp;
	}
};
map<string, Texture*> Texture::loadedTextures;

class Shader;
class Cubemap : public Texture {
public:
	void init(int width, int height) {
		if(id==0)
			glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		this->width = width;
		this->height = height;
	}

	void convertFromEquirectangular(Texture& tex, Shader* shader);
	void convertFromEquirectangular(Cubemap& tex, Shader* shader);
	void quasiMonteCarloSimulation(Cubemap& tex, Shader* shader);

protected:
	static unsigned int captureFBO;
	static unsigned int captureRBO;
	static glm::mat4 captureProjection;
	static glm::mat4 captureViews[];
};
unsigned int Cubemap::captureFBO = 0;
unsigned int Cubemap::captureRBO = 0;
glm::mat4 Cubemap::captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 Cubemap::captureViews[] =
{
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
};

class Shader;
class IUniform {
protected:
	int location;
	string name;
public:
	IUniform(const Shader& shader, string& name);
	void updateLocation(const Shader& shader);
	int getLocation() const { return location; }
	virtual void setData(const void*) = 0;
	virtual void apply() const = 0;
	const string& getName() const { return name; }
};

template<class T>
class Uniform : public IUniform {
	T data;
public:
	Uniform(const Shader& shader, string& name, const T& data):
		IUniform(shader, name), data(data){}
	virtual void setData(const void* data) {
		this->data = *((T*)data);
	}
	virtual void apply() const {
		//assert("정의하지않은 값이 할당되려함" && 0);
	}
};
template<>
void Uniform<int>::apply() const {
	if (location == -1) return;
	glUniform1i(location, data);
}
template<>
void Uniform<float>::apply() const {
	if (location == -1) return;
	glUniform1f(location, data);
}
template<>
void Uniform<vec2>::apply() const {
	if (location == -1) return;
	glUniform2fv(location, 1, (GLfloat*)&data);
}
template<>
void Uniform<vec3>::apply() const {
	if (location == -1) return;
	glUniform3fv(location, 1, (GLfloat*)&data);
}
template<>
void Uniform<vec4>::apply() const {
	if (location == -1) return;
	glUniform4fv(location, 1, (GLfloat*)&data);
}
template<>
void Uniform<mat4>::apply() const {
	if (location == -1) return;
	glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&data);
}

struct TextureBindInfo {
	unsigned int id;
	unsigned int activeIdx;
	unsigned int type;
};

class TextureUniform : public IUniform {
public:
	TextureBindInfo textureInfo;
	TextureUniform(const Shader& shader, string& name) :
		IUniform(shader, name) {
	}
	void updateLocation(const Shader& shader) {
		IUniform::updateLocation(shader);
		if (location != -1)
		glUniform1i(location, textureInfo.activeIdx);
	}
	virtual void setData(const void* d) {
		textureInfo = *(TextureBindInfo*)d;
		if(location != -1)
		glUniform1i(location, textureInfo.activeIdx);
	}
	virtual void apply() const {
		if (location == -1) return;
		glActiveTexture(textureInfo.activeIdx + GL_TEXTURE0);
		glBindTexture(textureInfo.type, textureInfo.id);
	}
};

class Shader {
	unsigned int lastTextureIdx = 0;
	static int usingId;
	unsigned int id = 0;

	std::map<string, IUniform*> uniforms;
	vector<TextureUniform> texUniforms;
	string vsPath;
	string fsPath;
	static map<string, Shader*> shaders;
public:
	// 이미 있다면 만들지않고 반환
	static Shader* create(const char* vs, const char* fs) {
		stringstream ss;
		ss << vs << "," << fs;
		auto getShader = get(ss.str().c_str());
		if (getShader) {
			return getShader;
		}
		auto temp = new Shader();
		temp->complie(vs, fs);
		return temp;
	}

	static Shader* get(const char* name) {
		auto iter = shaders.find(name);
		if (iter != shaders.end())
			return iter->second;
		return nullptr;
	}

	static void recomplieAll() {
		for (auto i = shaders.begin(); i != shaders.end(); ++i) {
			i->second->recomplie();
		}
	}

	void complie(const char* vsPath) {
		complie(vsPath, vsPath);
	}

	void complie(const char* vs, const char* fs) {
		if(id != 0)
			glDeleteProgram(id);
		this->vsPath = vs;
		this->fsPath = fs;
		stringstream ss;
		ss << vs << "," << fs;

		auto find = shaders.insert(make_pair<string, Shader*>(ss.str(), this));
		if (!find.second) {
			assert(0 && vs && fs); //vs fs에 경로가 없음
		}
		id = ::complie(vs, fs);
	}

	void recomplie() {
		assert(id != 0);
		glDeleteProgram(id);
		id = ::complie(vsPath.c_str(), fsPath.c_str());
		pureUse();
		for (auto begin = uniforms.begin();
			begin != uniforms.end(); begin++) {
			begin->second->updateLocation(*this);
			begin->second->apply();
		}
		for (size_t i = 0; i < texUniforms.size(); i++) {
			texUniforms[i].updateLocation(*this);
		}
	}

	void use() const {
		pureUse();
		for (size_t i = 0; i < texUniforms.size(); i++) {
			texUniforms[i].apply();
		}
	}

	void pureUse() const {
		if (usingId != id) {
			glUseProgram(id);
			usingId = id;
		}
	}

	void setUniform(string name, int ptr) {
		getUniformLocation<int>(name, ptr)->apply();
	}
	void setUniform(string name, float ptr) {
		getUniformLocation<float>(name, ptr)->apply();
	}
	void setUniform(string name, const vec2& ptr) {
		getUniformLocation<vec2>(name, ptr)->apply();
	}
	void setUniform(string name, const vec3& ptr) {
		getUniformLocation<vec3>(name, ptr)->apply();
	}
	void setUniform(string name, const vec4& ptr) {
		getUniformLocation<vec4>(name, ptr)->apply();
	}
	void setUniform(string name, const mat4& ptr) {
		getUniformLocation<mat4>(name, ptr)->apply();
	}
	void setUniform(string name, Texture ptr) {
		setTexture(name, ptr);
	}
	void setUniform(string name, const Cubemap& ptr) {
		setTexture(name, ptr, GL_TEXTURE_CUBE_MAP);
	}

private:
	// 이름으로 유니폼가져옴
	template<class T>
	IUniform* getUniformLocation(string name, const T& data) {
		pureUse();
		auto findKey = uniforms.find(name.c_str());
		if (findKey == uniforms.end()) {
			auto temp = new Uniform<T>(*this, name, data);
			uniforms[name]= temp;
			return temp;
		}
		findKey->second->setData(&data);
		return findKey->second;
	}

	// 없으면 null 반환
	TextureUniform* getTextureUniform(string& name) {
		pureUse();
		for (size_t i = 0; i < texUniforms.size(); i++) {
			if (texUniforms[i].getName()._Equal(name)) {
				return &texUniforms[i];
			}
		}
		return nullptr;
	}

	void setTexture(string name, const Texture& ptr, unsigned type= GL_TEXTURE_2D) {
		auto uniform = getTextureUniform(name);
		if (uniform) {
			uniform->textureInfo.id = ptr.getId();
			uniform->textureInfo.type = type;
		} else {
			TextureUniform temp(*this, name);
			auto bindInfo = TextureBindInfo{ ptr.getId(), lastTextureIdx, type };
			temp.setData(&bindInfo);
			texUniforms.push_back(temp);
			++lastTextureIdx;
		}
	}
public:
	unsigned int getId() const { return id; }

};
map<string, Shader*> Shader::shaders;
int Shader::usingId = -1;

void Cubemap::convertFromEquirectangular(Texture& tex, Shader *shader) {
	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	
	shader->setUniform("map", tex);
	shader->setUniform("projection", captureProjection);

	glViewport(0, 0, width, height);

	shader->use();
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {
		shader->setUniform("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, id, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}
void Cubemap::convertFromEquirectangular(Cubemap& tex, Shader* shader) {
	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	shader->setUniform("map", tex);
	shader->setUniform("projection", captureProjection);

	glViewport(0, 0, width, height);

	shader->use();
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {
		shader->setUniform("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, id, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void Cubemap::quasiMonteCarloSimulation(Cubemap& tex, Shader* shader) {
	if (captureFBO == 0) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	shader->setUniform("map", tex);
	shader->setUniform("projection", captureProjection);

	shader->use();
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = 128 * std::pow(0.5, mip);
		unsigned int mipHeight = 128 * std::pow(0.5, mip);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		shader->setUniform("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i) {
			shader->setUniform("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, id, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IUniform::updateLocation(const Shader& shader) {
	location = glGetUniformLocation(shader.getId(), name.c_str());
}

IUniform::IUniform(const Shader& shader, string& name) {
	this->name = name;
	updateLocation(shader);
}

class Obj : public TickObj {
protected:
	Obj* parentObj = nullptr;
	glm::mat4 trans = glm::mat4(1.0f);
	Shader* shader = nullptr;

public:
	VO* vo = nullptr;
	vec3 color = vec3(1);

	// call after this have verteies
	void loadObj(const char* path) {
		vo = VO::loadObj(path);
		vo->bind();
	}

	vec3 getForward() {
		vec3 forward;
		forward.x = sin(glm::radians(rot.y));
		forward.y = tan(glm::radians(rot.x));
		forward.z = cos(glm::radians(rot.y));
		return forward;
	}

	vec3 getRight() {
		vec3 right;
		right.x = sin(glm::radians(rot.y+90));
		right.y = tan(glm::radians(rot.x));
		right.z = cos(glm::radians(rot.y + 90));
		return right;
	}

	vec3 getUp() {
		vec3 up;
		up.x = sin(glm::radians(rot.y));
		up.y = tan(glm::radians(rot.x+90));
		up.z = cos(glm::radians(rot.y));
		return up;
	}

	glm::mat4& getTrans() {
		if (bIsGetTransInThisTick) return trans;
		if (parentObj) trans = parentObj->getTrans();
		else trans = mat4(1);
		trans = glm::translate(trans, pos);
		trans = glm::rotate(trans, glm::radians(rot.y), glm::vec3(0.0, 1, 0));
		trans = glm::rotate(trans, glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
		trans = glm::rotate(trans, glm::radians(rot.x), glm::vec3(1, 0.0, 0));
		trans = glm::scale(trans, scale);
		bIsGetTransInThisTick = true;
		return trans;
	}

	virtual void tick(float dt) {
		updateTransform();
	}

	virtual bool guiStart() {
		if (TickObj::guiStart()) {
			ImGui::DragFloat3("pos", (float*)&pos, 0.04f, 0, 0, "%.2f");
			ImGui::DragFloat3("rot", (float*)&rot, 1, 0, 0, "%.2f");
			ImGui::DragFloat3("scale", (float*)&scale, 0.04f, 0, 0, "%.2f", 0.03f);
			return true;
		}
		return false;
	}

	virtual void render() {
		assert(shader && &name); // shader 없음
		shader->setUniform("model", getTrans());
		shader->setUniform("color", color);
		shader->use();
		vo->render();
	}

private:
	bool bIsGetTransInThisTick = false;
protected:
	vec3 pos = vec3(0);
	vec3 scale = vec3(1);
	vec3 rot = vec3(0);
public:
	const vec3& getPos()const { return pos; }
	const vec3& getRotation()const { return rot; }
	const vec3& getScale()const { return scale; }
	void setPos(const vec3& p) { pos = p; updateTransform(); }
	void setRotation(const vec3& r) { rot = r; updateTransform(); }
	void setScale(const vec3& s) { scale = s; updateTransform(); }
	void translateX(float x) { updateTransform(); pos.x += x; }
	void translateY(float y) { updateTransform(); pos.y += y; }
	void translateZ(float z) { updateTransform(); pos.z += z; }
	void rotateX(float x) { updateTransform(); rot.x += x; }
	void rotateY(float y) { updateTransform(); rot.y += y; }
	void rotateZ(float z) { updateTransform(); rot.z += z; }
	void setParent(Obj* parent) { assert(parent != this); }
	Obj* GetParent() { return parentObj; }
	void setShader(Shader& shader) { this->shader = &shader; }
	void updateTransform() { bIsGetTransInThisTick = false; }
};

class Camera : public Obj {
public:
	glm::vec3 armVector = glm::vec3(0, 0, -10);
	glm::vec3 target = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 viewPoint = glm::vec3(0, 1, 0);

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
		name = "Camera";
	}

	virtual void tick(float dt) {
		bIsGetTransInThisTick = 0;
		bIsGetVPInThisTick = 0;
	}

	virtual void bind() {
		UBO->setData(*this, Window::get());
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
				p = glm::perspective(glm::radians(fov), win.getRatio(), 0.1f, 1000.f);
			else {
				float size = 5;
				p = glm::ortho<float>(-size * win.getRatio(), size * win.getRatio(), -size, size, 0.1, 50.f);
			}
			bIsGetVPInThisTick |= 0b1;
		}

		vp = p * v;
		viewPoint = glm::inverse(v) * vec4(0, 0, 0, 1);
		bIsGetTransInThisTick = 1;
		return vp;
	}

	vec3& getViewPoint() {
		return viewPoint;
	}

	void translate(vec3 off) {
		vec3 forward = getForward();
		vec3 right = glm::normalize(glm::cross(forward, up));
		vec3 up = glm::normalize(glm::cross(right, forward));
		vec3 moveOff = forward * off.z + /*glm::reflect(up, forward)*/up * off.y + right * off.x;
		pos += moveOff;
	}

	void render() {}
	void gui(){}

private:
	uint bIsGetTransInThisTick = 0;
	uint bIsGetVPInThisTick = 0;
};


CameraShaderUniformBuffer* Camera::UBO = nullptr;


void CameraShaderUniformBuffer::setData(Camera& cam, Window& win) {
	this->p = &cam.p;
	this->v = &cam.v;
	this->vp = &cam.getTrans(win);
	this->pos = &cam.getViewPoint();
}




enum EMouse {
	NONE,
	MOUSE_X, MOUSE_Y,
	MOUSE_OFF_X, MOUSE_OFF_Y,
	MOUSE_NORMALIZE_X, MOUSE_NORMALIZE_Y,
	MOUSE_NORMALIZE_OFF_X, MOUSE_NORMALIZE_OFF_Y, // 안만듬
	MOUSE_L_BUTTON, MOUSE_M_BUTTON, MOUSE_R_BUTTON,
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
	ImGui_ImplGLUT_KeyboardFunc(key, x, y);
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
	ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
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
	ImGui_ImplGLUT_MouseWheelFunc(wheel, direction, x, y);
	Input::mouse[EMouse::MOUSE_WHEEL] += direction;
}

void setMousePos(int x, int y) {
	float screenMouseX = (float)x / Window::get().getHalfWidth() - 1;
	float screenMouseY = -(float)y / Window::get().getHalfHeight() + 1;

	Input::mouse[EMouse::MOUSE_X] = x;
	Input::mouse[EMouse::MOUSE_Y] = y;

	Input::mouse[EMouse::MOUSE_NORMALIZE_X] = screenMouseX;
	Input::mouse[EMouse::MOUSE_NORMALIZE_Y] = screenMouseY;
}

// button: 0(left), 1(mid), 2(right), 3(scrollup), 4(scrolldown)
// state: 0(down), 1(up)
void mouseClickHandler(int button, int state, int x, int y) {
	debug("mouseClickHandler: button(%d), state(%d)", button, state);
	ImGui_ImplGLUT_MouseFunc(button, state, x, y);
	Input::mouse[EMouse::MOUSE_L_BUTTON + button] = !state;
	//setMousePos(x, y);
}

void mouseMotionHandler(int x, int y) {
	//debug("mouseMotionHandler: x(%d), y(%d)", x,y);
	ImGui_ImplGLUT_MotionFunc(x, y);

}

void bindInput() {
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutMouseFunc(mouseClickHandler);
	glutPassiveMotionFunc(mouseMotionHandler);
	glutSpecialFunc(specialInput);
	glutMouseWheelFunc(mouseWheel);

	glutMotionFunc(ImGui_ImplGLUT_MotionFunc);

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

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube() {
	// initialize (if necessary)
	if (cubeVAO == 0) {
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom-right
			1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
			1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
			-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
			-1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
			1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
			1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left     
		   // bottom face
		   -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
		   1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
		   1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
		   1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
		   -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
		   -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
		   // top face
		   -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
		   1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
		   1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right     
		   1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
		   -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
		   -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
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

	static void drawLine(vec3 a, vec3 b, vec3 color = vec3(1)) {
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
		auto unlit = Shader::get("unlit,unlit");
		unlit->setUniform("model", mat4_1);

		for (size_t i = 0; i < drawIdx; i++) {
			unlit->setUniform("color", mesh[i].color);
			unlit->use();
			mesh[i].vo.render();
		}
		drawIdx = 0;
	}
};

vector<DebugMesh> Debug::mesh;
int Debug::drawIdx = 0;
