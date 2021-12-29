#include "theme.h"
#include "font_cache.h"

namespace hui
{
Theme::Theme(u32 atlasTextureSize)
{
	atlas = new Atlas(atlasTextureSize, atlasTextureSize);
	fontCache = new FontCache(atlas);
	auto whiteImage = atlas->addWhiteImage(32);
}

Theme::~Theme()
{
	delete fontCache;
	delete atlas;
}

Image* Theme::addImage(const Rgba32* pixels, u32 width, u32 height)
{
	if (!pixels || !width || !height)
	{
		return 0;
	}

	return atlas->addImage(pixels, width, height);
}

void Theme::packAtlas()
{
	atlas->pack();
}

void Theme::setDefaultWidgetStyle()
{
	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		elements[i].setDefaultStyle();
	}

	for (auto& elem : userElements)
	{
		elem.second->setDefaultStyle();
	}
}

}
