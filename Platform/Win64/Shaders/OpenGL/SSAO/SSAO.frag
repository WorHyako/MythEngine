#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

// tile noise texture over screen based on screen dimensions divided by noise sizeof
const vec2 noiseScale = vec2(800.0f/4.0f, 600.0f/4.0f);

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 0.5f;
float bias = 0.025f;


void main()
{
	// get input for SSAO algorithm
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

	// create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	for(int i = 0; i < kernelSize; i++)
	{
		// getting sample point coords in view space
		vec3 samplePos  = TBN * samples[i]; // translate from tangent to view
		samplePos = fragPos + samplePos  * radius;

		// project sample position (to sample texture) (to get position on screen/texture)
		vec4 offset = vec4(samplePos, 1.0f);
		offset = projection * offset; // translate from view to clip (screen space)
		offset.xyz /= offset.w; // perspective divide
		offset.xyz = offset.xyz * 0.5f + 0.5f; // to [0., 1.]

		// get sample depth
		float sampleDepth = texture(gPosition, offset.xy).z;

		// range check & accumulate
		float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth));
		occlusion += ((sampleDepth >= samplePos .z + bias ? 1.0f : 0.0f) * rangeCheck);
		
	}
	occlusion = 1.0f - (occlusion / kernelSize);
	FragColor = occlusion;
}