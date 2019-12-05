#version 420

in vec2 uv;

uniform sampler2D texture0;

out vec4 ocolor;

void main(){
	ocolor = texture(texture0, uv).bgra;
	//ocolor.a = 0.0f;
	//ocolor = vec4(uv,0,1);
}