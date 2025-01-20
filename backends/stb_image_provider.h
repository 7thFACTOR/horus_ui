#pragma once
#include "horus.h"

namespace hui
{
struct StbImageProvider : ImageProvider
{
	bool loadImage(const char* path, ImageData& outImage) override;
	bool savePngImage(const char* path, const ImageData& image) override;
};
}