#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 lightCol;
} ubo;


layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 faceNormal;
layout(location = 1) in vec2 texCoord; 
layout(location = 2) in vec4 worldPos;


layout(location = 0) out vec4 outColor;
void main()
{

    vec3 norm = normalize(faceNormal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - worldPos.xyz);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightCol.xyz;

    vec4 texCol = texture(texSampler, texCoord);
    vec3 ambient =  ubo.lightCol.w *  ubo.lightCol.xyz;
    outColor = texCol * vec4(ambient + diffuse, 1.0f);
}