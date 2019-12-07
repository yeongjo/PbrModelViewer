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
protected:
	Texture* texture;
	Shader* shader;
public:
	string name;

	// 초기화
	virtual void init() {
	}

	// 반복되는곳
	virtual void tick(float dt) {

	}

	virtual void render() {
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







class CTextureRenderer : public TextureRenderer{
public:
	virtual void init() {
		shader->addUniform("texture0", *texture);
		// 너가 추가하면 되는곳

		Texture tex3;
		tex3.load("../textures/c.jpg");
 		shader->addUniform("texture1", tex3);
		shader->addUniform("color", new vec3(1, 0.2f, 0.5f));
		//
	}
};


namespace sterma {
	FrameBuffer frameBuffer;
	CTextureRenderer texRenderer;

	// 전역변수 여기에
	FrameBuffer frame;

	Shader frameShader;

	Texture* texture;

	void Init() {
		frame.init(Window::get().w, Window::get().h);
		frame.colorBuffer;

		
		frameBuffer.init(Window::get().w, Window::get().h);
		frameShader.complieShader("postprocess");
		texRenderer.setShader(frameShader);


		texRenderer.setTexture(frameBuffer.colorBuffer);
		texRenderer.init();
	}


	void render() {
		/*frame.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		frame.unbind();

		frame.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		frame.unbind();*/


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		texRenderer.render();
	}

	void bind() {
		frameBuffer.bind();
	}

	void unbind() {
		frameBuffer.unbind();
	}
}


/*
FrameBuffer frame;

	void Init() {
		frame.init(Window::get().w, Window::get().h);
	}


	void Render() {
		frame.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		frame.unbind();
	}
*/