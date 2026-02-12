#include <string.h>
#include <algorithm>
#include "atlas.h"
#include "renderer.h"
#include "context.h"
#include "util.h"

namespace hui
{

Atlas::Atlas()
{}

Atlas::Atlas(u32 textureWidth, u32 textureHeight, u32 spacing, const Color& bgColor)
{
	create(textureWidth, textureHeight, spacing, bgColor);
}

Atlas::~Atlas()
{
	clearImages();
}

void Atlas::create(u32 textureWidth, u32 textureHeight, u32 spacing, const Color& bgColor)
{
	width = textureWidth;
	height = textureHeight;
	this->spacing = spacing;
	this->bgColor = bgColor;
	atlasImageData.resize((size_t)width * height);
}

Image* Atlas::getImage(ImageId id) const
{
	auto iter = images.find(id);

	if (iter == images.end())
	{
		return nullptr;
	}

	return iter->second;
}

Image* Atlas::addImage(const Rgba32* imageData, u32 width, u32 height, bool addBleedOut)
{
	return addImageInternal(lastImageId++, imageData, width, height, addBleedOut);
}

Image* Atlas::addImageInternal(ImageId imgId, const Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool addBleedOut)
{
	if (!imageWidth || !imageHeight)
		return nullptr;

	Image* image = new Image();
	HORUS_ASSERT(image);
	HORUS_ASSERT(imageData);

	u32 imageSize = imageWidth * imageHeight;

	image->id = imgId;
	image->imageData.resize(imageSize);
	memcpy(&image->imageData[0], imageData, (size_t)imageSize * sizeof(Rgba32));
	image->width = imageWidth;
	image->height = imageHeight;
	image->uvRect.set(0, 0, 0, 0);
	image->rect.set(0, 0, 0, 0);
	image->bleedOut = addBleedOut;
	images.insert(std::make_pair(imgId, image));

	return image;
}

void Atlas::deleteImage(Image* image)
{
	// seems space glyph doesnt have an image, just check...
	//TODO: maybe space should also have an image
	if (!image)
		return;

	auto iter = images.find(image->id);

	if (iter == images.end())
		return;

	delete image;
	images.erase(iter);
}

Image* Atlas::addWhiteImage(u32 width)
{
	u32 whiteImageSize = width * width;
	Rgba32* whiteImageData = new Rgba32[whiteImageSize];

	memset(whiteImageData, 0xFF, (size_t)whiteImageSize * sizeof(Rgba32));
	whiteImage = addImage(whiteImageData, width, width, true);
	delete[] whiteImageData;

	return whiteImage;
}

bool Atlas::pack()
{
	if (images.empty())
		return false;

	u32 border2 = spacing * 2;
	Rect packedRect;
	bool rotated = false;
	std::vector<PackedRect> packRects;

	for (auto& imgPair : images)
	{
		auto& img = imgPair.second;

		img->rect.set(0, 0, 0, 0);
		img->uvRect.set(0, 0, 0, 0);

		if (img->width == 0 || img->height == 0)
			continue;

		PackedRect prc;
		prc.id = img->id;
		prc.rect.width = img->width + border2;
		prc.rect.height = img->height + border2;
		packRects.push_back(prc);
	}

	auto ret = ctx->settings.services.packRects(packRects.data(), packRects.size(), width, height);
	u32 packedOkCount = 0;

	for (auto& prc : packRects)
	{
		if (prc.packedOk)
		{
			auto& img = images[prc.id];
			img->rect = prc.rect;
			++packedOkCount;
		}
	}

	lastPackSuccess = packedOkCount == packRects.size();

	// we have now the rects inside the atlas, copy to atlas image data and prepare the uv rects of the images
	for (auto& imgPair : images)
	{
		auto& image = imgPair.second;

		// bring back the original rect
		image->rect.x += spacing;
		image->rect.y += spacing;
		image->rect.width -= border2;
		image->rect.height -= border2;

		// if bleedOut, then limit/shrink the rect so we sample from within the image
		if (image->bleedOut)
		{
			const int bleedOutSize = 3;
			image->rect.x += bleedOutSize;
			image->rect.y += bleedOutSize;
			image->rect.width -= bleedOutSize * 2;
			image->rect.height -= bleedOutSize * 2;
		}

		// if packed width != from image width, it was rotated CW
		image->rotated = image->rect.width != image->width;

		image->uvRect.set(
			(f32)image->rect.x / (f32)width,
			(f32)image->rect.y / (f32)height,
			(f32)image->rect.width / (f32)width,
			(f32)image->rect.height / (f32)height);

		if (image->rotated)
		{
			// we prepare the final rect of the image, swap the dimensions
			image->rect.width = image->width;
			image->rect.height = image->height;
		}

		auto imgData = image->imageData.data();

		// copy image to the atlas image buffer
		if (image->rotated)
		{
			// rotation is clockwise
			for (u32 y = 0; y < image->height; y++)
			{
				for (u32 x = 0; x < image->width; x++)
				{
					u32 destIndex =
						image->rect.x + y + (image->rect.y + x) * width;
					u32 srcIndex = y * image->width + (image->width - 1) - x;
					atlasImageData[destIndex] = imgData[srcIndex];
				}
			}
		}
		else
		{
			for (u32 y = 0; y < image->height; y++)
			{
				for (u32 x = 0; x < image->width; x++)
				{
					u32 destIndex =
						image->rect.x + x + (image->rect.y + y) * width;
					u32 srcIndex = x + y * image->width;
					atlasImageData[destIndex] = imgData[srcIndex];
				}
			}
		}
	}

	return lastPackSuccess;
}

void Atlas::clearImages()
{
	for (auto& image : images)
	{
		delete image.second;
	}

	images.clear();
}

}