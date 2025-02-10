#version 450

in VS_OUT {
    vec3 T0;
    vec3 T1;
    vec3 T2;
    vec3 T3;
    vec4 T4;
} fs_in;

layout(location = 0) out vec4 outColor;

uniform float FoamTextureDisturb;
uniform sampler3D VolumeTexture;
uniform sampler2D FoamTexture; // Additional texture for foam rendering

void main() {
    // Sample the 3D volume texture
    vec3 volumeColor = texture(VolumeTexture, fs_in.T4.xyz).rgb;

    // Bias and scale (equivalent to r4_bx2 in ASM)
    vec3 biasedVolumeColor = 2.0 * volumeColor - 1.0;

    // Compute perturbed normal using dot products
    vec3 volumeNormal = vec3(
        dot(fs_in.T0, biasedVolumeColor),
        dot(fs_in.T1, biasedVolumeColor),
        dot(fs_in.T2, biasedVolumeColor)
    ) * FoamTextureDisturb;

    // Sample the foam texture using perturbed texture coordinates
    vec3 foamTexCoords = fs_in.T3.rgb;
    vec3 foamColor = texture(FoamTexture, foamTexCoords.xy).rgb;

    // Store foam color with transparency (alpha = foam depth factor)
    outColor = vec4(foamColor, foamTexCoords.b);
}