#version 430 core
out vec4 color;

uniform vec3 lineColor;

void main(){
	color = vec4(lineColor,1.0f);
} 