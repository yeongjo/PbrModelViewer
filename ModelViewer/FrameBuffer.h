#pragma once
#include "inc/ToolModule.h"

class FrameBuffer {
public:
	unsigned int FBO;
	Texture colorBuffer;
	unsigned int rboDepth;

	void init(int width, int height);
	void bind();
	void unbind();
};


class TextureRenderer {
	Texture* texture;
	Shader* shader;
public:
	string name;

	// 초기화
	void init() {
		shader->addUniform("texture0", *texture);
		// 너가 추가하면 되는곳



		//
	}

	// 반복되는곳
	void tick(float dt) {

	}

	void render() {
		bindShader();
		renderQuad();
	}

	void setTexture(Texture& texture) {
		this->texture = &texture;
	}

	void setShader(Shader& shader) {
		this->shader = &shader;
	}

protected:
	void bindShader() {
		if (shader) {
			shader->use();
		} else {
			assert(0 && name.c_str()); // shader 없음
		}
	}
};