//
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

vec4 constant = vec4(5.0, 0.5, 0.04, 1.0);
uniform vec4 constant1 = vec4(0.0f, 1.0f, 0.5f, -0.04f);
uniform vec4 tangentBasis = vec4(1.0f, 0.0f, 0.0f, 1.0f);
uniform vec4 frenelK = vec4(1.0f, 0.0f, 0.0f, 1.0f)
uniform vec4 frenelMax = vec4(0.0f, 1.0f, 0.0f, 1.0f);

layout(binding = 0) uniform UniformBufferObject{
    mat4 mvp; //16
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
    vec3 Tangent;
    vec3 Bitangent;
    vec3 Normal;
    vec3 T0;
    vec3 T1;
    vec3 T2;
    vec3 T3;
    vec4 T4;
} vs_out;  

void main()
{
    vec4 worldPos = mvp * (inPosition, 1.0);
    gl_Position = worldPos;

    float oFog = 1 / exp2(-worldPos.z * ubo.constant2.w);

    // normalize 
    vec4 normal = vec4(inNormal, 1.0);
    normal.w = dot(inNormal, inNormal);
    normal.w = 1.0 / sqrt(normal.w);
    normal = normal * normal.w;

    // viewDir
    vec4 viewPos = (inPosition - cameraPos, 1.0);
    viewPos.w = dot(viewPos.xyz, viewPos.xyz);
    viewPos.w = 1.0 / sqrt(viewPos.w);
    viewPos = viewPos * viewPos.w;

    float dotProduct = dot(viewPos.xyz, normal.xyz);
    vec3 refDir = 2 * dotProduct * normal.xyz + (-viewPos.xyz);

    vec4 tangent = normalize(normal.yzxw * ubo.tangentBasis.zxyw - ubo.tangentBasis.yzxw * normal.zxyw);
    vec4 bitangent = normalize(normal.yzxw * tangent.zxyw - tangent.yzxw * normal.zxyw);

    out.T0 = vec3(dot(tangent.xyz, mTexProjection[0].xyz), dot(bitangent.xyz, mTexProjection[0].xyz), dot(normal.xyz, mTexProjection[0].xyz));
    out.T1 = vec3(dot(tangent.xyz, mTexProjection[1].xyz), dot(bitangent.xyz, mTexProjection[1].xyz), dot(normal.xyz, mTexProjection[1].xyz));
    out.T2 = vec3(dot(tangent.xyz, mTexProjection[2].xyz), dot(bitangent.xyz, mTexProjection[2].xyz), dot(normal.xyz, mTexProjection[2].xyz));

    out.T3.xy = inUV * ubo.shadowConst.z;
    out.T3.z = max((inPosition.y - ubo.shadowConst.x) * ubo.shadowConst.y, ubo.constant1.x)

	out.T4.z = ubo.animation.x;
    out.T4.xy = inUV;
}
