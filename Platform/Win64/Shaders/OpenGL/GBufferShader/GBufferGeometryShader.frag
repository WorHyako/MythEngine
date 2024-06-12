#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material
{
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
};
uniform Material material;

void main()
{
	gPosition = vec4(FragPos, 1.0f);
	gNormal = vec4(normalize(Normal), 1.0f);
	gAlbedoSpec.rgb = texture(material.texture_diffuse1, TexCoords).rgb;
	gAlbedoSpec.a = texture(material.texture_specular1, TexCoords).r;
}