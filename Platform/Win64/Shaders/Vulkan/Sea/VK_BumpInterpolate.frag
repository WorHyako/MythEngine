//
#version 460

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform AlphaBuffer { float alpha; } ubo;
layout(binding = 1) uniform sampler2D texSampler1;
layout(binding = 2) uniform sampler2D texSampler2;

void main()
{
    outColor = mix(texture(texSampler1, texCoord), texture(texSampler2, texCoord), ubo.alpha);
}
