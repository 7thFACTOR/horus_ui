#pragma once
#include <horus.h>

namespace hui
{
/// Load a theme from a JSON file, will use the file I/O services provided by Horus, so it can be used in any platform supported
/// \param filename the JSON filename (*.json), relative to executable
HORUS_API HTheme loadThemeFromJson(const char* filename, char* errorTextBuffer = 0, size_t errorTextBufferSize = 0);
HORUS_API bool loadPngImage(const char* path, ImageData& outImage);
HORUS_API bool savePngImage(const char* path, const ImageData& image);
HORUS_API void deleteImageData(ImageData& image);
}