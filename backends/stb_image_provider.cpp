#include "stb_image_provider.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace hui
{
	stbi_uc* data = stbi_load(filename, &width, &height, &comp, 4);

}