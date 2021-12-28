#pragma once
#include <horus.h>

namespace hui
{
/// Load a theme from a JSON file
/// \param filename the JSON filename (*.json), relative to executable
HORUS_API HTheme loadThemeFromJson(const char* filename, char* errorTextBuffer = 0, size_t errorTextBufferSize = 0);
}