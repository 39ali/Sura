#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location=0) in vec3 iPosition ; 
layout (location=1) in vec3 iNormal ; 
layout (location=2) in vec3 iColor;

layout (location=0) out vec3 oColor;


layout (push_constant) uniform constants {
    vec4 data;
    mat4 render_matrix ;
} pushConstants ; 


void main (){
gl_Position =pushConstants.render_matrix* vec4 (iPosition, 1.0);
oColor = iColor;
}