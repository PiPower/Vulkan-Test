#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 lightCol;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTex;


layout(location = 0) out vec3 faceNormal;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec4 worldPos;
void main() 
{
    worldPos =  ubo.model * vec4(inPosition, 1.0);
    gl_Position =  ubo.proj * ubo.view * worldPos;
    //gl_Position =  vec4(inPosition, 0.0, 1.0);
    faceNormal =  transpose(inverse(mat3(ubo.model))) * inNormal;
    //faceNormal = inNormal;
    texCoord = inTex;
}