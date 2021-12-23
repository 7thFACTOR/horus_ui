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
class UiAtlas;

struct AtlasTexture
{
	TextureArray* textureArray = nullptr;
	u32 textureIndex = 0;
	Rgba32* textureImage = nullptr;
	bool dirty = false;
};

struct UiImage
{
	UiImageId id = 0;
	UiAtlas* atlas = nullptr;
	AtlasTexture* atlasTexture = nullptr;
	bool rotated = false;
	Rect uvRect;
	Rect rect;
	Rgba32* imageData = nullptr;
	u32 width = 0, height = 0;
	bool bleedOut = false;
};

class UiAtlas
{
public:
	UiAtlas() {}
	UiAtlas(u32 width, u32 height);
	~UiAtlas();

	void create(u32 width, u32 height);
	UiImage* getImageById(UiImageId id) const;
	UiImage* addImage(const Rgba32* imageData, u32 width, u32 height, bool addBleedOut = false);
	void updateImageData(UiImageId imgId, const Rgba32* imageData, u32 width, u32 height);
	void deleteImage(UiImage* image);
	UiImage* addWhiteImage(u32 width = 8);
	bool pack(
		u32 spacing = 5,
		const Color& bgColor = Color::black);
	void repackImages();
	void packWithLastUsedParams() { pack(lastUsedSpacing, lastUsedBgColor); }
	void clearImages();

	UiImage* whiteImage = nullptr;
	TextureArray* textureArray = nullptr;

protected:
	struct PackImageData
	{
		UiImageId id = 0;
		UiAtlas* atlas = nullptr;
		AtlasTexture* atlasTexture = nullptr;
		Rgba32* imageData = nullptr;
		u32 width = 0;
		u32 height = 0;
		Rect packedRect;
		bool bleedOut = false;
	};

	void deletePackerImages();
	UiImage* addImageInternal(UiImageId imgId, const Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool addBleedOut);

	u32 id = 0;
	u32 lastImageId = 1;
	u32 width = 0;
	u32 height = 0;
	u32 lastUsedSpacing = 0;
	Color lastUsedBgColor = Color::black;
	bool useWasteMap = true;
	std::vector<AtlasTexture*> atlasTextures;
	std::unordered_map<UiImageId, UiImage*> images;
	std::vector<PackImageData> pendingPackImages;
};

}