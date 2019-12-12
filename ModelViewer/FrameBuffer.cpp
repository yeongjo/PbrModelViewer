#include "FrameBuffer.h"

inline void FrameBuffer::init(int width, int height) {
	init(*Texture::createEmpty(width, height));
}

void FrameBuffer::init(const Texture& tex) {
	w = tex.getWidth();
	h = tex.getHeight();
	colorTex = tex;
	glGenFramebuffers(1, &FBO);
	// create floating point color buffer
	// create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex.getId(), 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		print("Framebuffer not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, w, h);
}

void FrameBuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
