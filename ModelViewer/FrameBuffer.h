#pragma once
#include "inc/ToolModule.h"

class FrameBuffer {
public:
	unsigned int FBO;
	Texture colorTex;
	unsigned int rboDepth;
	unsigned int w, h;

	void init(int width, int height);
	void init(const Texture& tex);
	void bind() const;
	void unbind() const;
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


void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}




class CTextureRenderer : public TextureRenderer{
public:
	virtual void init() {
		
	}
};


namespace sterma {
	FrameBuffer frameBuffer;
	CTextureRenderer texRenderer;

	// 전역변수 여기에
	FrameBuffer frame;

	Shader frameShader;

	Texture* texture;

	vec3 color = vec3(1);
	bool hdr;

	void Init() {
		frame.init(Window::get().getW(), Window::get().getH());

		
		frameBuffer.init(Window::get().getW(), Window::get().getH());


		frameShader.complie("postprocess");
		texRenderer.setShader(frameShader);


		frameShader.setUniform("texture0", frameBuffer.colorTex);
		// 너가 추가하면 되는곳

		Texture* tex3;
		tex3 = Texture::load("textures/c.jpg");
		frameShader.setUniform("texture1", *tex3);
		//


		texRenderer.setTexture(frameBuffer.colorTex);
		texRenderer.init();
	}


	void render() {
		/*frame.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		frame.unbind();

		frame.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		frame.unbind();*/

		frameShader.setUniform("color", color);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		texRenderer.render();
	}

	void renderWindow() {
		if (ImGui::TreeNode("postprocess")) {
			static bool alpha_preview = true;
			static bool alpha_half_preview = false;
			static bool drag_and_drop = true;
			static bool options_menu = true;
			ImGui::Checkbox("With HDR", &hdr); ImGui::SameLine(); HelpMarker("Currently all this does is to lift the 0..1 limits on dragging widgets.");
			ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

			ImGui::ColorEdit3("MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | misc_flags);
			ImGui::TreePop();
		}
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