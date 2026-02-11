#pragma once
#include <horus.h>
#include <ft2build.h>
#include <freetype/freetype.h>

namespace hui
{
void initFreetype(Services& services, FT_Library freetypeContext = 0);
void shutdownFreetype(Services& services);
}