#version 420

layout(location=0) in vec3 pos;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 16 * 4 = 64
	mat4 v; // 128
	mat4 vp; // 192
};

out vec3 vcolor;

void main(){
	gl_Position = vp*vec4(pos,1);
	vcolor = abs(pos);
	vcolor = (pos);
}