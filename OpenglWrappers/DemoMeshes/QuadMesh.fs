#version 430 core
out vec4 color;

in vec2 texcoord;

void main(){
     //color = vec4(texture(texture,texcoord));
	 color = vec4(texcoord.x,texcoord.y,0.5,1);
} 