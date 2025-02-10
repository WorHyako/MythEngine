#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

uniform vec4 constant1 = vec4(0.0f, 1.0f, 0.5f, -0.04f);
uniform vec4 tangentBasis = vec4(1.0f, 0.0f, 0.0f, 1.0f);
uniform vec4 frenelK = vec4(1.0f, 0.0f, 0.0f, 1.0f)
uniform vec4 frenelMax = vec4(0.0f, 1.0f, 0.0f, 1.0f);

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec4 constant2;
    vec4 shadowConst;
    vec4 animation;
    vec4 cameraPos;
    vec4 seaParameters;
    vec4 seaColor;
    vec4 skyColor;
    vec4 vec7;
    mat4 mTexProjection;
} ubo;

out VS_OUT {
    vec4 diffuse;
    vec4 specular;
    vec3 T0;
    vec3 T1;
    vec3 T2;
    vec3 T3;
    vec3 viewDir;
    float fogFactor;
} vs_out;  

void main()
{
    vec4 worldPos = ubo.mvp * vec4(inPosition, 1.0);
    gl_Position = worldPos;

    // Compute fog factor
    float depthFactor = worldPos.z * ubo.constant2.w;
    vs_out.fogFactor = 1.0 / exp2(depthFactor);

    // Normalize the normal
    vec3 normal = normalize(inNormal);
    
    // Compute view direction
    vec3 viewDir = normalize(inPosition - ubo.cameraPos.xyz);
    
    // Reflection direction
    vec3 reflectedDir = reflect(-viewDir, normal);

    vec3 r3;
    vs_out.diffuse.rgb = r3.yyy * skyColor;
    max(0, dot(normal, Constant2.xyz));
    vs_out.diffuse.a = max(0, dot(reflection, constant1.xyz));
    output.specular.rgb = SeaColor.xyz * output.diffuse.a;
    output.specular.a = 1.0;

    // Compute tangent and bitangent
    vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0))); // Approximate tangent
    vec3 bitangent = normalize(cross(normal, tangent));
    
    // Compute transformed tangent space vectors
    vs_out.T0 = vec3(dot(tangent, ubo.mTexProjection[0].xyz),
                     dot(bitangent, ubo.mTexProjection[0].xyz),
                     dot(normal, ubo.mTexProjection[0].xyz));
    vs_out.T1 = vec3(dot(tangent, ubo.mTexProjection[1].xyz),
                     dot(bitangent, ubo.mTexProjection[1].xyz),
                     dot(normal, ubo.mTexProjection[1].xyz));
    vs_out.T2 = vec3(dot(tangent, ubo.mTexProjection[2].xyz),
                     dot(bitangent, ubo.mTexProjection[2].xyz),
                     dot(normal, ubo.mTexProjection[2].xyz));

    vs_out.viewDir = -viewDir;

	out.T4.z = ubo.animation.x;
    out.T4.xy = inUV;
}
