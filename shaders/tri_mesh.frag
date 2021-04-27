#version 450 
layout (location=0) in vec3 iColor; 
layout (location=1) in vec2 iUv; 

layout (location=0) out vec4 oColor;


layout(set=0,binding=1) uniform SceneData {
	vec4 fogColor;
	vec4 fogDistances;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} sceneData;

layout(set=2, binding=0) uniform sampler2D tex1; 

void main (){

//oColor =vec4(iColor+ sceneData.ambientColor.xyz ,1.0f);
vec3 color = texture(tex1,iUv).xyz; 
oColor = vec4(color,1.0f);
//oColor = vec4(iUv.x,iUv.y,0.5f,1.0f);
}