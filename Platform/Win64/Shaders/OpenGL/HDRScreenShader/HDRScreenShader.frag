#version 330

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D colorMap;
uniform sampler2D bloomMap;
uniform float exposure = 1.0f;

void main()
{
	const float gamma = 2.2f;
	vec3 hdrColor = texture(colorMap, TexCoords).rgb;
	vec3 bloomColor = texture(bloomMap, TexCoords).rgb;
	hdrColor += bloomColor;

	vec3 mapped = vec3(0.0f);
	// tonal compression
	//mapped = hdrColor / (hdrColor + vec3(1.0f));
	// tonal compression with exposition
	mapped = vec3(1.0f) - exp(-hdrColor * exposure);
	// gamma - correction
	mapped = pow(mapped, vec3(1.0f / gamma));
	FragColor = vec4(vec3(mapped), 1.0f);
}