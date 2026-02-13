#pragma once
#include "types.h"

namespace hui
{
struct AtlasImage
{
	ImageId id = 0;
	u32 width = 0, height = 0;
	bool rotated = false;
	bool halfTexelInset = false;
	Rect uvRect;
	Rect rect;
	Rgba32* imageData = nullptr;
};

struct Atlas
{
	Atlas();
	Atlas(u32 width, u32 height, u32 spacing = 2, const Color& bgColor = Color::black);
	~Atlas();

	void create(u32 width, u32 height, u32 spacing = 2, const Color& bgColor = Color::black);
	bool addImage(ImageId id, Rgba32* imageData, u32 width, u32 height, bool halfTexelInset = false);
	bool pack();
	void clearImages();
	
	bool addImageInternal(ImageId imgId, Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool halfTexelInset);
	void addWhiteImage(u32 size);

	u32 width = 0;
	u32 height = 0;
	u32 spacing = 0;
	Color bgColor = Color::black;
	std::vector<Rgba32> atlasImageData;
	std::unordered_map<ImageId, AtlasImage> images;
	bool lastPackSuccess = false;
};

}