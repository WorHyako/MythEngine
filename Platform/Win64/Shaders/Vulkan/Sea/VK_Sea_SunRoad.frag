#version 450

in VS_OUT {
    vec3 T1;
    vec3 T2;
    vec3 T3;
    vec3 viewDir;  // View direction from vertex shader
    vec2 texCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler3D VolumeTexture;  // Corresponds to pVolumeTexture or pRenderTargetBumpMap
layout(binding = 1) uniform samplerCube SunRoadMap;

void main() {
    // Sample the volume texture (3D texture lookup using transformed coordinates)
    vec3 N = texture(VolumeTexture, vec3(fs_in.texCoords)).rgb;
    vec3 Nworld;
    Nworld.x = dot(N*2-1, T1);
    Nworld.y = dot(N*2-1, T2);
    Nworld.z = dot(N*2-1, T3);

    vec3 R = 2 * dot(Nworld, Eye) * Nworld – Eye * dot(Nworld, Nworld);
    vec4 specular = texture(SunRoadMap, R);
    
    outColor = specular;
}
