#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "horus_interfaces.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace hui
{
class Atlas;

struct AtlasTexture
{
	TextureArray* textureArray = nullptr;
	u32 textureIndex = 0;
	Rgba32* textureImage = nullptr;
	HRectPacker packer = 0;
	bool dirty = false;
	bool filledUp = false;
	std::vector<PackRect> rects;
};

struct Image
{
	ImageId id = 0;
	Atlas* atlas = nullptr;
	AtlasTexture* atlasTexture = nullptr;
	bool rotated = false;
	Rect uvRect;
	Rect rect;
	Rgba32* imageData = nullptr;
	u32 width = 0, height = 0;
	bool bleedOut = false;
};

class Atlas
{
public:
	Atlas() {}
	Atlas(u32 width, u32 height);
	~Atlas();

	void create(u32 width, u32 height);
	Image* getImageById(ImageId id) const;
	Image* addImage(const Rgba32* imageData, u32 width, u32 height, bool addBleedOut = false);
	void updateImageData(ImageId imgId, const Rgba32* imageData, u32 width, u32 height);
	void deleteImage(Image* image);
	Image* addWhiteImage(u32 width = 8);
	bool pack(
		u32 spacing = 2,
		const Color& bgColor = Color::black);
	void repackImages();
	void packWithLastUsedParams() { pack(lastUsedSpacing, lastUsedBgColor); }
	void clearImages();

	Image* whiteImage = nullptr;
	TextureArray* textureArray = nullptr;

protected:
	Image* addImageInternal(ImageId imgId, const Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool addBleedOut);

	u32 id = 0;
	u32 lastImageId = 1;
	u32 width = 0;
	u32 height = 0;
	u32 lastUsedSpacing = 0;
	Color lastUsedBgColor = Color::black;
	std::vector<AtlasTexture*> atlasTextures;
	std::unordered_map<ImageId, Image*> images;
	std::vector<Image*> nonPackedImages;
};

}