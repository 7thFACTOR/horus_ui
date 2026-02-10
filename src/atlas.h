#pragma once
#include "types.h"

namespace hui
{
struct Image
{
	ImageId id = 0;
	u32 width = 0, height = 0;
	bool rotated = false;
	bool bleedOut = false;
	Rect uvRect;
	Rect rect;
	std::vector<Rgba32> imageData;
};

struct Atlas
{
	Atlas();
	Atlas(u32 width, u32 height, u32 spacing = 2, const Color& bgColor = Color::black);
	~Atlas();

	void create(u32 width, u32 height, u32 spacing = 2, const Color& bgColor = Color::black);
	Image* getImage(ImageId id) const;
	Image* addImage(const Rgba32* imageData, u32 width, u32 height, bool addBleedOut = false);
	void deleteImage(Image* image);
	Image* addWhiteImage(u32 width = 4);
	bool pack();
	void clearImages();

	Image* whiteImage = nullptr;

protected:
	Image* addImageInternal(ImageId imgId, const Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool addBleedOut);

	u32 lastImageId = 1;
	u32 width = 0;
	u32 height = 0;
	u32 spacing = 0;
	Color bgColor = Color::black;
	std::vector<Rgba32> atlasImageData;
	std::unordered_map<ImageId, Image*> images;
	bool lastPackSuccess = false;
};

}