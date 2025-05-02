#version 450
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec4 lightPosIn;
layout(location = 2) in vec4 lightColIn; 

layout(location = 0) out vec4 outColor;

void main() {
    vec3 colorScalling = lightColIn.w *  lightColIn.xyz;
    vec3 objColor = fragColor * colorScalling;
    outColor = vec4(objColor, 1.0);
    vec2 texCoord = vec2(0.0f, 0.0f);
    outColor = texture(texSampler, texCoord);
}