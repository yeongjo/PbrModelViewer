#version 420
out vec4 ocolor;

in vec2 uv;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 color;


void main(){	
	vec4 test;
	if(texture(texture0, uv).r > 0.3f)
		test = vec4(color, 1) * texture(texture0, uv).rgba;
	ocolor = texture(texture0, uv) ;//+ texture(texture0, uv);
	//ocolor = texture(texture0, uv);
	//ocolor = texture(texture1, uv)*0.8f*vec4(color,1) + texture(texture0, uv)*0.2f;
}
