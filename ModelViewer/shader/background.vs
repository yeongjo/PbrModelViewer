#version 420
layout (location = 0) in vec3 aPos;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 0 : 16 * 4 = 64
	mat4 v; // 64 : 128
	mat4 vp; // 128 : 192
	vec3 viewPos; // 192 : (16*3=48)+192= 240
};

out vec3 WorldPos;

void main()
{
    WorldPos = aPos;

	mat4 rotView = mat4(mat3(v));
	vec4 clipPos = p * rotView * vec4(WorldPos, 1.0);

	gl_Position = clipPos.xyww;
}