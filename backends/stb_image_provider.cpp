#include "stb_image_provider.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace hui
{
bool StbImageProvider::loadImage(const char* path, ImageData& outImage)
{
	int width = 0;
	int height = 0;
	int comp = 0;

	stbi_uc* data = stbi_load(path, &width, &height, &comp, 4);

	outImage.pixels = (u8*)data;
	outImage.bpp = 32;
	outImage.width = width;
	outImage.height = height;

	if (!data || !width || !height || !comp)
		return false;

	return true;
}

bool StbImageProvider::saveImage(const char* path, const ImageData& image)
{
	auto res = stbi_write_png(path, image.width, image.height, image.bpp / 8, image.pixels, 0);

	return res != 0;
}

}