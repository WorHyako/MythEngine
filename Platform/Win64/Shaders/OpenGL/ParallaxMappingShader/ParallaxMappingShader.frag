#version 330 core

out vec4 FragColor;

struct Material
{
	sampler2D diffuseMap;
    sampler2D normalMap;
    sampler2D depthMap;
    float shininess;
};
uniform Material material;

uniform bool bUseBlinnPhong;
uniform float height_scale = 0.03f;

struct PointLight{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform PointLight pointLight;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords, float lastDepthValue);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);
vec2 SteppingParallaxMapping(vec2 texCoords, vec3 viewDir);
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir);
vec2 ReliefParallaxMapping(vec2 texCoords, vec3 viewDir, out float lastDepthValue);
float getParallaxSelfShadowing(vec2 texCoords, vec3 lightDir, float lastDepthValue);

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords, float lastDepthValue)
{
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0f);
    // specular shading
    float spec = 0.0f;
    if(bUseBlinnPhong)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    }
    // attenuation
    float distance = length(fs_in.TangentLightPos - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuseMap, texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuseMap, texCoords));
    vec3 specular = light.specular * spec * vec3(0.2f);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    float shadowMultiplier = getParallaxSelfShadowing(texCoords, lightDir, lastDepthValue);
    return (ambient + diffuse + specular) * shadowMultiplier;
    return vec3(shadowMultiplier);
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float height = texture(material.depthMap, texCoords).r;
    vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
    return texCoords - p;
}

vec2 SteppingParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    // amount of depth layers
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDir)));
    // size of each layer
    float layerDepth = 1.0f / numLayers;
    // depth current layer
    float currentLayerDepth = 0.0f;
    // texCoords step size
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // move texCoords along vector P
        currentTexCoords -= deltaTexCoords;
        // sample depthMap in current texCoords
        currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;
        // calculate depth next layer
        currentLayerDepth += layerDepth;
    }

    return currentTexCoords;
}

vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    // amount of depth layers
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDir)));
    // size of each layer
    float layerDepth = 1.0f / numLayers;
    // depth current layer
    float currentLayerDepth = 0.0f;
    // texCoords step size
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // move texCoords along vector P
        currentTexCoords -= deltaTexCoords;
        // sample depthMap in current texCoords
        currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;
        // calculate depth next layer
        currentLayerDepth += layerDepth;
    }

    // previous texCoords
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // previous and after depthMap value
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation texCoords
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1 - weight);

    return finalTexCoords;
}

vec2 ReliefParallaxMapping(vec2 texCoords, vec3 viewDir, out float lastDepthValue)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    // amount of depth layers
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDir)));
    // size of each layer
    float layerDepth = 1.0f / numLayers;
    // depth current layer
    float currentLayerDepth = 0.0f;
    // texCoords step size
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // move texCoords along vector P
        currentTexCoords -= deltaTexCoords;
        // sample depthMap in current texCoords
        currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;
        // calculate depth next layer
        currentLayerDepth += layerDepth;
    }

    // code implementation ReliefParallaxMapping
    //--------------
    // divide by 2 texCoords offset and layerDepth
    deltaTexCoords *= 0.5f;
    layerDepth *= 0.5f;

    // step back from current Step PM find point
    currentTexCoords += deltaTexCoords;
    currentLayerDepth -= layerDepth;

    // iteration of search
    const int reliefSteps = 5;
    int currentStep = reliefSteps;
    while(currentStep > 0)
    {
        currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;
        deltaTexCoords *= 0.5f;
        layerDepth *= 0.5f;

        if(currentDepthMapValue > currentLayerDepth)
        {
            currentTexCoords -= deltaTexCoords;
            currentLayerDepth += layerDepth;
        }
        else{
            currentTexCoords += deltaTexCoords;
            currentLayerDepth -= layerDepth;
        }
        currentStep--;
    }

    lastDepthValue = currentDepthMapValue;
    return currentTexCoords;
}

float getParallaxSelfShadowing(vec2 texCoords, vec3 lightDir, float lastDepthValue)
{
    float shadowMultiplier = 0.0f;
    // check light dot
    float alignFactor = dot(vec3(0.0f, 0.0f, 1.0f), lightDir);
    if(alignFactor > 0)
    {
        const float minLayers = 8.0f;
        const float maxLayers = 32.0f;
        // amount of depth layers
        float numLayers = mix(maxLayers, minLayers, abs(alignFactor));
        // size of each layer
        float layerDepth = lastDepthValue / numLayers;
        vec2 deltaTexCoords = height_scale * lightDir.xy / (lightDir.z * numLayers);
        
        // count of points under surface
        int numSamplesUnderSurface = 0;

        float currentLayerDepth = lastDepthValue - layerDepth;
        vec2 currentTexCoords = texCoords + deltaTexCoords;

        float currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;

        // current step index
        float stepIndex = 1.0f;
        while(currentLayerDepth > 0.0f)
        {
            if(currentDepthMapValue < currentLayerDepth)
            {
                numSamplesUnderSurface++;
                float currenShadowMultiplier = (currentLayerDepth - currentDepthMapValue) * (1.0f - stepIndex / numLayers);

                shadowMultiplier = max(shadowMultiplier, currenShadowMultiplier);
            }
            stepIndex++;
            currentLayerDepth -= layerDepth;
            currentTexCoords += deltaTexCoords;
            currentDepthMapValue = texture(material.depthMap, texCoords).r;
        }

        if(numSamplesUnderSurface < 1)
        {
            shadowMultiplier = 1.0f;
        }
        else
        {
            shadowMultiplier = 1.0f - shadowMultiplier;
        }

        return shadowMultiplier;
    }
}

void main()
{
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    // get moved texture coordinate with ParallaxMapping
    //vec2 texCoords = ParallaxOcclusionMapping(fs_in.TexCoords, viewDir);
    float lastDepthValue = 0.0f;
    vec2 texCoords = ReliefParallaxMapping(fs_in.TexCoords, viewDir, lastDepthValue);
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard; 

    vec3 normal = texture(material.normalMap, texCoords).rgb;
    normal = normalize(normal * 2.0f - 1.0f);
    
    vec3 result = vec3(0.0f);
    result += CalcPointLight(pointLight, normal, fs_in.TangentFragPos, viewDir, texCoords, lastDepthValue);
    FragColor = vec4(result, 1.0f);
}