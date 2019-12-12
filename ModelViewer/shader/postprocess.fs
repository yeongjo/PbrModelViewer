#version 420
out vec4 ocolor;

in vec2 uv;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 color;


void main(){	
	vec4 test;
	if(texture(texture0, uv).r > 0.5f)
		test = vec4(color, 1) * texture(texture0, uv).rgba;
	// ocolor = test ;//+ texture(texture0, uv);
	ocolor = texture(texture0, uv) + test*2f;	
}
