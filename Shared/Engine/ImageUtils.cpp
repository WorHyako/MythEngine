#include <ImageUtils.hpp>

#include <cstdlib>
#include <cassert>

/// Draw a checkerboard on a pre-allocated square RGB image.
uint8_t* genDefaultCheckboardImage(int* width, int* height) {
    const int w = 128;
    const int h = 128;

    uint8_t* imgData = (uint8_t*)std::malloc(w * h * 3); //stbi_load() uses malloc(), so this is safe

    assert(imgData && w > 0 && h > 0);
    assert(w == h);

    if (!imgData || w <= 0 || h <= 0) return nullptr;
    if (w != h) return nullptr;

    for (int i = 0; i < w * h; i++) {
        const int row = i / w;
        const int col = i % w;
        imgData[i * 3 + 0] = imgData[i * 3 + 1] = imgData[i * 3 + 2] = 0xFF * ((row + col) % 2);
    }

    if (width) *width = w;
    if (height) *height = h;

    return imgData;
}