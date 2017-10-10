#version 430 core
layout (location = 0) in vec3 position;

uniform sampler1D lights;

out vec3 debug;

void main(){
   debug = texture(lights,1023.5f/1024).rgb;

   gl_Position = vec4(position,1.0f);
}