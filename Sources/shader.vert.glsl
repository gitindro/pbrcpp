#version 450

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



in vec3 pos;

in vec3 normal;
in vec2 texcoord;

out vec2 TexCoord;
out vec3 norm;
out vec3 WorldPos;
void main() 
{
	norm = (model * vec4(normal, 0.0)).xyz;
	TexCoord = texcoord;

	gl_Position = projection * view * model * vec4(pos, 1.0);
	WorldPos = gl_Position.xyz; 
}
