#version 430 core
out vec4 color;

in Pipe{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
}fsInput;

void main(){
     //color = vec4(texture(texture,texcoord));
	 //color = vec4(texcoord.x,texcoord.y,0.5,1);
	 vec3 N = normalize(fsInput.Normal);
	 color = vec4(N*0.5+0.5,1.0);
} 