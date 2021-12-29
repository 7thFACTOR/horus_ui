#include <string.h>
#include "atlas.h"
#include "horus_interfaces.h"
#include "renderer.h"
#include "context.h"
#include "util.h"
#include <assert.h>

namespace hui
{
static u32 atlasId = 0;

Atlas::Atlas(u32 textureWidth, u32 textureHeight)
{
	id = atlasId++;
	create(textureWidth, textureHeight);
}

Atlas::~Atlas()
{
	for (auto& at : atlasTextures)
	{
		delete[] at->textureImage;
	}

	delete textureArray;

	clearImages();
}

void Atlas::create(u32 textureWidth, u32 textureHeight)
{
	width = textureWidth;
	height = textureHeight;
	textureArray = ctx->providers->gfx->createTextureArray();
	textureArray->resize(1, textureWidth, textureHeight);
}

Image* Atlas::getImageById(ImageId id) const
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
	if (!imageWidth || !imageHeight) return nullptr;

	Image* image = new Image();

	u32 imageSize = imageWidth * imageHeight;
	image->id = imgId;
	image->atlas = this;
	image->imageData = new Rgba32[imageSize];
	memcpy(image->imageData, imageData, (size_t)imageSize * 4);
	image->width = imageWidth;
	image->height = imageHeight;
	image->rect.set(0, 0, 0, 0);
	image->bleedOut = addBleedOut;
	images.insert(std::make_pair(imgId, image));
	nonPackedImages.push_back(image);

	return image;
}

void Atlas::updateImageData(ImageId imgId, const Rgba32* imageData, u32 width, u32 height)
{
	//TODO
}

void Atlas::deleteImage(Image* image)
{
	auto iter = images.find(image->id);

	if (iter == images.end())
		return;

	delete[] image->imageData;
	delete image;

	//TODO: we would need to recreate the atlas texture where this image was in
	images.erase(iter);
}

Image* Atlas::addWhiteImage(u32 width)
{
	u32 whiteImageSize = width * width;
	Rgba32* whiteImageData = new Rgba32[whiteImageSize];

	memset(whiteImageData, 0xff, (size_t)whiteImageSize * 4);
	whiteImage = addImage(whiteImageData, width, width, true);
	delete[]whiteImageData;

	return whiteImage;
}

bool Atlas::pack(
	u32 spacing,
	const Color& bgColor)
{
	if (images.empty())
		return true;

	lastUsedBgColor = bgColor;
	lastUsedSpacing = spacing;

	u32 border2 = spacing * 2;
	Rect packedRect;
	bool rotated = false;
	bool allTexturesDirty = false;
	std::vector<PackRect> packRects;
	const u32 maxAtlasPageCount = 64;
	u32 atlasPageCount = 0;
	std::vector<Image*> packedImages;

	for (auto img : nonPackedImages)
	{
		img->rect.set(0, 0, 0, 0);
		img->atlasTexture = nullptr;

		if (img->width == 0 || img->height == 0)
			continue;

		PackRect prc;
		prc.id = img->id;
		prc.rect.width = img->width + border2;
		prc.rect.height = img->height + border2;
		packRects.push_back(prc);
	}

	auto packIntoAtlasTex = [&packRects, &packedImages, border2, this](AtlasTexture* atlasTex)
	{
		packRects.clear();
		for (auto img : nonPackedImages)
		{
			PackRect rc;
			rc.id = img->id;
			rc.rect = { 0.f, 0.f, (f32)img->width + border2, (f32)img->height + border2 };
			packRects.push_back(rc);
		}

		auto ret = HORUS_RECTPACK->packRects(atlasTex->packer, packRects.data(), packRects.size());
		auto iter = packRects.begin();

		while (iter != packRects.end())
		{
			if (iter->packedOk)
			{
				atlasTex->rects.push_back(*iter);
				images[iter->id]->atlasTexture = atlasTex;
				images[iter->id]->rect = iter->rect;
				packedImages.push_back(images[iter->id]);
				auto iterImg = std::find(nonPackedImages.begin(), nonPackedImages.begin(), images[iter->id]);
				if (iterImg != nonPackedImages.end())
					nonPackedImages.erase(iterImg);
				iter = packRects.erase(iter);
				continue;
			}

			++iter;
		}

		if (!packRects.empty())
		{
			atlasTex->filledUp = true;
		}

		return ret;
	};

	for (auto& atlasTex : atlasTextures)
	{
		if (atlasTex->filledUp)
			continue;

		if (packIntoAtlasTex(atlasTex))
			break;
	}

	while (!nonPackedImages.empty() && atlasTextures.size() <= maxAtlasPageCount)
	{
		AtlasTexture* newTexture = new AtlasTexture();

		newTexture->textureImage = new Rgba32[(size_t)width * height];
		memset(newTexture->textureImage, 0, (size_t)width * height * sizeof(Rgba32));
		newTexture->textureIndex = atlasTextures.size();
		newTexture->textureArray = textureArray;
		newTexture->dirty = true;
		newTexture->packer = HORUS_RECTPACK->createRectPacker();
		HORUS_RECTPACK->reset(newTexture->packer, width, height);
		atlasTextures.push_back(newTexture);
		// resize the texture array
		textureArray->resize(atlasTextures.size(), width, height);
		packIntoAtlasTex(newTexture);
	}

	assert(nonPackedImages.empty());

	// we have now the rects inside the atlas, copy to atlas textures
	for (auto image : packedImages)
	{
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
					image->atlasTexture->textureImage[destIndex] = ((Rgba32*)image->imageData)[srcIndex];
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
					image->atlasTexture->textureImage[destIndex] = ((Rgba32*)image->imageData)[srcIndex];
				}
			}
		}

		image->atlasTexture->dirty = true;
	}

	// upload atlas textures to gfx card
	for (auto& atlasTex : atlasTextures)
	{
		if (atlasTex->dirty || allTexturesDirty)
		{
			atlasTex->textureArray->updateLayerData(atlasTex->textureIndex, atlasTex->textureImage);
			atlasTex->dirty = false;
		}
	}

	return true;
}

void Atlas::repackImages()
{
	packWithLastUsedParams();
}

void Atlas::clearImages()
{
	for (auto image : images)
	{
		delete[] image.second->imageData;
		delete image.second;
	}

	images.clear();
}

}