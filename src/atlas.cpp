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
	clearImages();
	width = textureWidth;
	height = textureHeight;
	this->spacing = spacing;
	this->bgColor = bgColor;
	atlasImageData.resize((size_t)width * height);
	memset(atlasImageData.data(), 0, atlasImageData.size());
}

bool Atlas::addImage(ImageId id, Rgba32* imageData, u32 width, u32 height, bool halfTexelInset)
{
	return addImageInternal(id, imageData, width, height, halfTexelInset);
}

bool Atlas::addImageInternal(ImageId imgId, Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool halfTexelInset)
{
	if (!imageWidth || !imageHeight)
		return false;

	HUI_ASSERT(imageData);
	u32 imageSize = imageWidth * imageHeight;
	AtlasImage image;

	image.id = imgId;
	image.imageData = imageData;
	image.width = imageWidth;
	image.height = imageHeight;
	image.uvRect.set(0, 0, 0, 0);
	image.rect.set(0, 0, 0, 0);
	image.halfTexelInset = halfTexelInset;
	images.insert(std::make_pair(imgId, image));

	return true;
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

		img.rect.set(0, 0, 0, 0);
		img.uvRect.set(0, 0, 0, 0);

		if (img.width == 0 || img.height == 0)
			continue;

		PackedRect prc;
		prc.id = img.id;
		prc.rect.width = img.width + border2;
		prc.rect.height = img.height + border2;
		packRects.push_back(prc);
	}

	auto ret = ctx->settings.services.packRects(packRects.data(), packRects.size(), width, height);
	u32 packedOkCount = 0;

	for (auto& prc : packRects)
	{
		if (prc.packedOk)
		{
			auto& img = images[prc.id];
			img.rect = prc.rect;
			++packedOkCount;
		}
	}

	lastPackSuccess = packedOkCount == packRects.size();

	// we have now the rects inside the atlas, copy to atlas image data and prepare the uv rects of the images
	for (auto& imgPair : images)
	{
		auto& image = imgPair.second;

		// bring back the original rect
		image.rect.x += spacing;
		image.rect.y += spacing;
		image.rect.width -= border2;
		image.rect.height -= border2;

		// if packed width != from image width, it was rotated CW
		image.rotated = (u32)image.rect.width != image.width;

		image.uvRect.set(
			(f32)image.rect.x / (f32)width,
			(f32)image.rect.y / (f32)height,
			(f32)image.rect.width / (f32)width,
			(f32)image.rect.height / (f32)height);

		if (image.halfTexelInset)
		{
			f32 halfTexelU = 0.5f / width;
			f32 halfTexelV = 0.5f / height;
			image.uvRect.x += halfTexelU;
			image.uvRect.y += halfTexelV;
			image.uvRect.width -= halfTexelU * 2.0f;
			image.uvRect.height -= halfTexelV * 2.0f;
		}

		if (image.rotated)
		{
			// we prepare the final rect of the image, swap the dimensions
			image.rect.width = image.width;
			image.rect.height = image.height;
		}

		auto imgData = image.imageData;

		// copy image to the atlas image buffer
		if (image.rotated)
		{
			// rotation is clockwise
			for (u32 y = 0; y < image.height; y++)
			{
				for (u32 x = 0; x < image.width; x++)
				{
					u32 destIndex =
						image.rect.x + y + (image.rect.y + x) * width;
					u32 srcIndex = y * image.width + (image.width - 1) - x;
					atlasImageData[destIndex] = imgData[srcIndex];
				}
			}
		}
		else
		{
			for (u32 y = 0; y < image.height; y++)
			{
				for (u32 x = 0; x < image.width; x++)
				{
					u32 destIndex =
						image.rect.x + x + (image.rect.y + y) * width;
					u32 srcIndex = x + y * image.width;
					atlasImageData[destIndex] = imgData[srcIndex];
				}
			}
		}
	}

	return lastPackSuccess;
}

void Atlas::clearImages()
{
	images.clear();
}

}