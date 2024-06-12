#version 330
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;

struct Light
{
	vec3 Position;
	vec3 Color;

	float Linear;
	float Quadratic;
	float Radius;
};

const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main()
{
	// retrieve data from gBuffer
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;
	float AmbientOcclusion = texture(ssao, TexCoords).r;

	// then calculate lighting as usual
	vec3 lighting = Albedo * 0.3f * AmbientOcclusion;
	vec3 viewDir = normalize(-FragPos);
	for(int i = 0; i < NR_LIGHTS; ++i)
	{
		float distance = length(lights[i].Position - FragPos);
		if(distance < lights[i].Radius)
		{	
			// diffuse
			vec3 lightDir = normalize(lights[i].Position - FragPos);
			vec3 diffuse = max(dot(Normal, lightDir), 0.0f) * Albedo * lights[i].Color;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(Normal, halfwayDir), 0.0f), 16.0f);
			vec3 specular = lights[i].Color * spec * Specular;
			// attenuation
			float attenuation = 1.0f / (1.0f + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
			diffuse *= attenuation;
			specular *= attenuation;
			lighting += diffuse + specular;
		}
	}
	FragColor = vec4(vec3(lighting), 1.0f);
}