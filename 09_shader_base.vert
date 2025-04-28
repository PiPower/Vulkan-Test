#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 lightCol;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec4 lightPosOut;
layout(location = 2) out vec4 lightColOut;
void main() {
    gl_Position =  ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    //gl_Position =  vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    lightPosOut = ubo.lightPos;
    lightColOut = ubo.lightCol;
}