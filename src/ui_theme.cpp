#include "ui_theme.h"
#include "font_cache.h"

namespace hui
{
UiTheme::UiTheme(u32 atlasTextureSize)
{
	atlas = new UiAtlas(atlasTextureSize, atlasTextureSize);
	fontCache = new FontCache(atlas);
	auto whiteImage = atlas->addWhiteImage(32);
}

UiTheme::~UiTheme()
{
	delete fontCache;
	delete atlas;
}

UiImage* UiTheme::addImage(const Rgba32* pixels, u32 width, u32 height)
{
	if (!pixels || !width || !height)
	{
		return 0;
	}

	return atlas->addImage(pixels, width, height);
}

void UiTheme::packAtlas()
{
	atlas->pack();
}

}
