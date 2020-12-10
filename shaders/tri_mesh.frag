#version 450 
layout (location=0) in vec3 iColor; 

layout (location=0) out vec4 oColor;

void main (){
//gl_FragDepth  =0.0;

oColor =vec4(iColor,1.0f);
}