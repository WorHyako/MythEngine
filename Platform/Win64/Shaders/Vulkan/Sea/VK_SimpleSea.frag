//
#version 450

in VS_OUT {
    vec3 Tangent;
    vec3 Bitangent;
    vec3 Normal;
    vec3 T0;
    vec3 T1;
    vec3 T2;
    vec3 T3;
    vec4 T4;
} fs_in;

layout(location = 0) out vec4 outColor;

uniform sampler2D uTexture;
uniform vec3 uLightDirection;

uniform sampler3D VolumeTexture;

varying vec3 vTangent;
varying vec3 vBitangent;
varying vec3 vNormalOut;
varying vec2 vTexCoordOut;

void main() {
    vec3 texColor = texture2D(uTexture, vTexCoordOut).rgb;
    vec3 lightDir = normalize(uLightDirection);
    
    float diffuse = max(dot(vNormalOut, lightDir), 0.0);
    vec3 color = texColor * diffuse;
    
    gl_FragColor = vec4(color, 1.0);
}
