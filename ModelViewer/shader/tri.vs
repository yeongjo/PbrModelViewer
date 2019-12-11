#version 420

layout(location=0) in vec3 pos;
layout(location=2) in vec3 aNormal;
layout(location=3) in vec2 aUv;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 0 : 16 * 4 = 64
	mat4 v; // 64 : 128
	mat4 vp; // 128 : 192
	vec3 viewPos; // 192 : (16*3=48)+192= 240
};

uniform mat4 model;

out vec3 normal;
out vec3 fragPos;
out vec2 uv;
out vec3 WorldPos;

void main(){
	normal = normalize(mat3(transpose(inverse(model))) * aNormal);
	WorldPos = pos; 
	uv = aUv;
	gl_Position = vp*model*vec4(pos,1);
	fragPos = (model*vec4(pos,1)).xyz;
}