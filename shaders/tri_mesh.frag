#version 450 
layout (location=0) in vec3 iColor; 

layout (location=0) out vec4 oColor;


layout(set=0,binding=1) uniform SceneData {
	vec4 fogColor;
	vec4 fogDistances;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} sceneData;

void main (){

oColor =vec4(iColor+ sceneData.ambientColor.xyz ,1.0f);
}