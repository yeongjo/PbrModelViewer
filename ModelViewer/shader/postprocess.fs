#version 420
out vec4 ocolor;

in vec2 uv;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 color;

uniform float u_lightAmount;
uniform float u_blurAmount;
uniform bool enalbe;

void main(){	
	//vec4 test;
	//if(texture(texture0, uv).r > u_lightAmount2)
	//	test = vec4(color, 1) * texture(texture0, uv).rgba;
	//ocolor = test ;//+ texture(texture0, uv);
	//ocolor = texture(texture0, uv) + test * u_lightAmount;	
	////////////////////////////////////////////////////
	
	vec4 blur_color = vec4(0,0,0,1);
	vec2 gaussFilter[9];
	
	gaussFilter[0] = vec2(-4.0, 0.0162162162);
	gaussFilter[1] = vec2(-3.0, 0.0540540541);
	gaussFilter[2] = vec2(-2.0, 0.1216216216);
	gaussFilter[3] = vec2(-1.0, 0.1945945946);
	gaussFilter[4] = vec2(0.0, 0.2270270270);
	gaussFilter[5] = vec2(1.0, 0.1945945946);
	gaussFilter[6] = vec2(2.0, 0.1216216216);    
	gaussFilter[7] = vec2(3.0, 0.0540540541);    
	gaussFilter[8] = vec2(4.0, 0.0162162162);    	 

	float blurSize = u_blurAmount * 0.01;

	for (int i = 0; i < 9; i++)			
		blur_color += texture(texture0, vec2( uv.x+gaussFilter[i].x*blurSize, uv.y+gaussFilter[i].x*blurSize ) )*gaussFilter[i].y;											
	
	
	if (enalbe)
		ocolor = texture(texture0, uv) + blur_color * u_lightAmount;
	else
		ocolor = texture(texture0, uv);
	
}



