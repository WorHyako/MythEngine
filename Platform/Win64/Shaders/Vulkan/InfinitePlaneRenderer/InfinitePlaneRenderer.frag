//
#version 460 core

#include <GridParameters.h>
#include <GridCalculation.h>

layout (location=0) in vec2 uv;
layout (location=1) in vec2 cameraPos;
layout (location=0) out vec4 out_FragColor;

void main()
{
	out_FragColor = gridColor(uv, cameraPos);
}