#version 330

layout(location=0) in vec3 pos;
layout(location=1) in vec3 color;
out vec3 vcolor;

void main(){
	vcolor = color;
	gl_Position = vec4(pos,1);
}