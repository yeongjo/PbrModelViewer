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

	// �ʱ�ȭ
	void init() {
		shader->addUniform("texture0", *texture);
		// �ʰ� �߰��ϸ� �Ǵ°�



		//
	}

	// �ݺ��Ǵ°�
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
			assert(0 && name.c_str()); // shader ����
		}
	}
};