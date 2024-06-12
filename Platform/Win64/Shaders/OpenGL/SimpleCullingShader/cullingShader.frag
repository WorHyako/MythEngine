#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D color;

void main()
{
	vec4 result = texture(color, TexCoords);
	//if(gl_FrontFacing)
		//result = vec4(1.0f);
	FragColor = result;
}