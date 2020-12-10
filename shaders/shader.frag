#version 450

//output write
layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec3 inColor;

void main() 
{
	//return red
	outFragColor = vec4(1.f,0.f,0.f,1.0f);
	outFragColor = vec4(inColor+0.1f,1.0f);
}