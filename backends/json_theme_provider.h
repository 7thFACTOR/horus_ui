#pragma once
#include <horus.h>

namespace hui
{
/// Load a theme from a JSON file
/// \param filename the JSON filename (*.json), relative to executable
HORUS_API HTheme loadThemeFromJson(const char* filename, char* errorTextBuffer = 0, size_t errorTextBufferSize = 0);

/// Checks if the theme file changed and reloads it if so.
/// \return A NEW HTheme handle if reloaded, or nullptr if not changed or error.
HORUS_API HTheme hotReloadTheme(const char* filename, char* errorTextBuffer = 0, size_t errorTextBufferSize = 0);
}