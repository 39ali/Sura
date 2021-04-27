#version 460

layout (location=0) in vec3 iPosition ; 
layout (location=1) in vec3 iNormal ; 
layout (location=2) in vec3 iColor;
layout (location=3) in vec2 iUv;

layout (location=0) out vec3 oColor;
layout (location=1) out vec2 oUv;

layout(set=0,binding=0) uniform CameraData {
    mat4 view;
    mat4 proj;
    mat4 projView; 
} cameraData;


struct ObjectData {
    mat4 model; 
}; 

layout(set=1, binding=0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer; 


layout (push_constant) uniform constants {
    vec4 data;
    mat4 render_matrix ;
} pushConstants ; 


void main (){
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
    mat4 transformMatrix = cameraData.projView* modelMatrix; 
    gl_Position =transformMatrix * vec4 (iPosition, 1.0);
    oColor = iColor;
    oUv = iUv;
}