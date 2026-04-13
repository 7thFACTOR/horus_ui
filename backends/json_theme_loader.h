#pragma once
#include <horus.h>

namespace hui
{
/// Load a theme from a JSON file, will use the file I/O services provided by Horus, so it can be used in any platform supported
/// \param filename the JSON filename (*.json), relative to executable
HUI_API HTheme loadThemeFromJson(const char* filename, char* errorTextBuffer = 0, size_t errorTextBufferSize = 0);
HUI_API HImage loadThemeImage(HTheme theme, const char* pngFilename);
HUI_API bool loadPngImage(const char* path, ImageData& outImage);
HUI_API bool savePngImage(const char* path, const ImageData& image);
HUI_API void deleteImageData(ImageData& image);
}