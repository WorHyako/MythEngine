#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

uniform mat4 model;
layout(std140) uniform Matrices
{
	uniform mat4 view;
	uniform mat4 projection;
};

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;  

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0f));
    vs_out.TexCoords = aTexCoords;
    vec3 T = normalize(vec3(model * vec4(aTangent, 1.0f)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 1.0f)));
    vec3 B = normalize(cross(N, T));
    vs_out.TBN = mat3(T, B, N);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}