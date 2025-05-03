#version 450
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec4 lightPosIn;
layout(location = 2) in vec4 lightColIn; 
layout(location = 3) in vec2 texCoord; 

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texCol = texture(texSampler, texCoord);
    vec3 colorScalling = lightColIn.w *  lightColIn.xyz;
    outColor = texCol * vec4(colorScalling, 1.0f);
    //outColor = vec4(objColor, 1.0);
}