#version 450

layout(location=0) in FS_IN {
    vec4 diffuse;
    vec4 specular;
    vec3 T0;
    vec3 T1;
    vec3 T2;
    vec3 Eye; // viewDir from vertex shader
    vec3 texCoords;
    float fogFactor;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler3D VolumeTexture;  // Corresponds to pVolumeTexture or pRenderTargetBumpMap
layout(binding = 2) uniform samplerCube EnvMap;

void main() {
    // Sample the volume texture (3D texture lookup using transformed coordinates)
    vec3 N = texture(VolumeTexture, vec3(fs_in.texCoords)).rgb;
    vec3 Nworld;
    Nworld.x = dot(N*2-1, fs_in.T0);
    Nworld.y = dot(N*2-1, fs_in.T1);
    Nworld.z = dot(N*2-1, fs_in.T2);

    vec3 R = reflect(fs_in.Eye, Nworld); //2 * dot(Nworld, fs_in.Eye) * Nworld – fs_in.Eye * dot(Nworld, Nworld);
    vec4 specular = texture(EnvMap, R);
    
    outColor = vec4(0.3098, 0.2588, 0.7098, 1.0) * fs_in.diffuse + specular;//specular;
    //outColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
}
