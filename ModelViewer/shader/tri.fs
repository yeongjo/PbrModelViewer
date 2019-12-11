#version 420

out vec4 color;

in vec3 normal;
in vec3 fragPos;
in vec2 uv;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 0 : 16 * 4 = 64
	mat4 v; // 64 : 128
	mat4 vp; // 128 : 192
	vec3 viewPos; // 192 : (16*3=48)+192= 240
};

uniform vec3 ambient;
uniform vec3 lightPos;
uniform vec4 lightColor;

uniform vec4 vcolor;

uniform samplerCube texture0;
//uniform sampler2D texture0;

void main(){
	
	vec3 lightDir = normalize(lightPos - fragPos);
	vec3 diffuse = max(dot(normal, lightDir), 0) * lightColor.rgb;

	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = 1 * spec * lightColor.rgb;  

	//color = vec4(lightColor.rgb, 1);
	//color = lightColor;
	//return;

	//vec3 result = (diffuse+ambient+specular) * vcolor.rgb * texture(texture0, uv).rgb;
	vec3 result = texture(texture0, normal).rgb;
	color = vec4(result, vcolor.a);
	//color = vec4(normal, vcolor.a);
}