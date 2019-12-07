#version 420
out vec4 ocolor;

in vec2 uv;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 color;

void main(){	
	//if(texture(texture0, uv).r > 0.2f)
	//ocolor = vec4(sex2, 1) * texture(texture1, uv).rgba;
	//ocolor = texture(texture1, uv);
	ocolor = texture(texture1, uv)*0.8f*vec4(color,1) + texture(texture0, uv)*0.2f;
}
