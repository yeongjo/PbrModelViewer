#version 420
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 0 : 16 * 4 = 64
	mat4 v; // 64 : 128
	mat4 vp; // 128 : 192
	vec3 viewPos; // 192 : (16*3=48)+192= 240
};
uniform mat4 model;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;   

    gl_Position =  vp * vec4(WorldPos, 1.0);
}