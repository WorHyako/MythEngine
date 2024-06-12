#pragma once

#include <glm/glm.hpp>

#include <Bitmap.hpp>

Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b);
Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b);

inline Bitmap convertEquirectangularMapToCubeFaceMap(const Bitmap& b)
{
	return convertEquirectangularMapToCubeFaceMap(convertEquirectangularMapToVerticalCross(b));
}

void convolveDiffuse(const glm::vec3* data, int srcW, int srcH, int dstW, int dstH, glm::vec3* output, int numMonteCarloSamples);