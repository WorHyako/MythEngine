#version 330 core

uniform bool bUseBlinnPhong;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emissive;
    sampler2D shadowMap;
    samplerCube shadowCubemap;
    float shininess;
};
uniform Material material;

struct Light{
    //vec3 position; // no longer necessary when using directional lights.
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
uniform Light light;

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);

struct DirLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

struct PointLight{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

out vec4 FragColor;

in VS_OUT
{
    vec3 outColor;
    vec3 Normal;
    vec3 FragPos;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform vec3 objectColor;
uniform vec3 viewPos;

// shadow cubemap
uniform float far_plane;
uniform vec3 lightPos;

// for shadow depth map Percentage-closer filtering
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5f + 0.5f;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    //float closestDepth = texture(material.shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    if(projCoords.z > 1.0f)
    {
        return 0.0f;
    }
    float currentDepth = projCoords.z;
    // anti muar effect
    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
    // check whether current frag pos is in shadow
    float shadow = 0.0f;
    vec2 texelSize = 1.0f / textureSize(material.shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(material.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
        }
    }
    //shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    shadow /= 9.0f;

    return shadow;
}

float ShadowCubemapCalculation(vec3 fragPos, vec3 lightPos)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float shadow = 0.0f;
    float bias = 0.05f;
    //float samples = 4.0f;
    int samples = 20;
    //float offset = 0.1f;
    float viewDistance = length(viewPos - fragPos);
    //float diskRadius = 0.05f;
    float diskRadius = (1.0f + (viewDistance / far_plane)) / 25.0f;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(material.shadowCubemap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        // from [0, 1] to source diaposon
        closestDepth *= far_plane;
        shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    }
    //for(float x = -offset; x < offset; x += offset / (samples * 0.5f))
    //{
    //    for(float y = -offset; y < offset; y += offset / (samples * 0.5f))
    //    {
    //        for(float z = -offset; z < offset; z += offset / (samples * 0.5f))
    //        {
    //            float closestDepth = texture(material.shadowCubemap, fragToLight + vec3(x, y, z)).r;
    //            // from [0, 1] to source diaposon
    //            closestDepth *= far_plane;
    //            shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    //        }
    //    }
    //}
    //return (shadow / (samples * samples * samples));
    return (shadow / float(samples));
}

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
   vec3 lightDir = normalize(light.position - fragPos);

   // diffuse
   float diff = max(dot(normal, lightDir), 0.0f);
   vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));

   // ambient
   vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));

   // specular
   float spec = 0.0f;
   if(bUseBlinnPhong)
   {
       vec3 halfwayDir = normalize(lightDir + viewDir);
       spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
   }
   else
   {
       vec3 reflectDir = reflect(-lightDir, normal);
       float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
   }
   vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords));

   float theta = dot(lightDir, normalize(-light.direction));
   float epsilon = light.cutOff - light.outerCutOff;
   float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0f, 1.0f);

   // we'll leace ambient unaffected so we always have a little light.
   diffuse *= intensity;
   specular *= intensity; 

   return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
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
        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    }
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords));
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);
    return (ambient + (diffuse + specular) * (1.0f - shadow));
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fs_in.FragPos);
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
        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    }
    // attenuation
    float distance = length(light.position - fs_in.FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    // cubemap shadow calculation
    float shadow = ShadowCubemapCalculation(fs_in.FragPos, lightPos);
    return (ambient + (1.0f - shadow) * (diffuse + specular));
    //vec3 fragToLight = fs_in.FragPos - lightPos;
    //float closestDepth = texture(material.shadowCubemap, fragToLight).r;
    //return vec3(closestDepth * 5.0f / far_plane);
}

void main()
{
   //FragColor = mix(texture(texture1, TexCoord), texture(texture2, vec2(TexCoord.x, TexCoord.y)), 0.2);

   // emissive
   int isEmissive = length(vec3(texture(material.specular, fs_in.TexCoords))) > 0.0f ? 0 : 1;
   vec3 emissive = vec3(texture(material.emissive, fs_in.TexCoords)) * isEmissive;

   // diffuse
   vec3 norm = normalize(fs_in.Normal);
   //else if(light.direction.w == 0)
   //    lightDir = normalize(-light.direction.xyz);
   vec3 viewDir = normalize(viewPos - fs_in.FragPos);

   vec3 result = vec3(0.0f);
   // phase 1: Directional lightning
   //result = CalcDirLight(dirLight, norm, viewDir);

   // phase 2: Point lights
   //for(int i = 0; i < NR_POINT_LIGHTS; i++)
   //   result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir);
   result += CalcPointLight(pointLights[3], norm, fs_in.FragPos, viewDir);

   // phase 3: Spot light
  // result += CalcSpotLight(light, norm, fs_in.FragPos, viewDir);

   //vec3 result = (ambient + diffuse + specular/* + emissive*/);
   FragColor = vec4(result, 1.0f);
   
   // with gamma - correction
   //float gamma = 2.2f;
   //FragColor = vec4(pow(result.rgb, vec3(1.0f/gamma)), 1.0f);

   //FragColor = vec4(fs_in.outColor * objectColor, 1.0f);
};