#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out VS_OUT
{	
	vec3 outColor;
	vec3 Normal;
	vec3 FragPos;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} vs_out;

uniform mat4 model;
layout(std140) uniform Matrices
{
	uniform mat4 view;
	uniform mat4 projection;
};

uniform mat4 lightSpaceMatrix;

//uniform vec3 lightColor;
uniform vec3 lightPos;


void main()
{
   vs_out.FragPos = vec3(/*view **/ model * vec4(aPos, 1.0f));
   vs_out.Normal = mat3(transpose(inverse(/*view **/ model))) * aNormal;
   vs_out.outColor =  vec3(1.0f, 1.0f, 1.0f);//aColor;
   vs_out.TexCoords = vec2(aTexCoord.x, aTexCoord.y);
   vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0f);
   gl_Position = projection * view * model * vec4(aPos, 1.0);

   /*float ambientStrength = 0.1f;
   vec3 ambient = ambientStrength * lightColor;

   float diffuseStrength = 1.0f;

   vec3 norm = normalize(vs_out.Normal);
   vec3 lightDir = normalize(vs_out.LightPos - vs_out.FragPos);

   float diff = max(dot(norm, lightDir), 0.0f);
   vec3 diffuse = diff * lightColor * diffuseStrength;

   float specularStrength = 0.5f;
   vec3 viewDir = normalize(-vs_out.FragPos);
   vec3 reflectDir = reflect(-lightDir, norm);
   float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
   vec3 specular = specularStrength * spec * lightColor;

   vec3 result = (ambient + diffuse + specular);
   vs_out.outColor = result;*/
};