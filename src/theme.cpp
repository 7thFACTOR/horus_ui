#include "theme.h"
#include "font_cache.h"

namespace hui
{
Theme::Theme(u32 atlasTextureSize)
{
	atlas = new Atlas(atlasTextureSize, atlasTextureSize);
	fontCache = new FontCache();
	atlas->addWhiteImage(4);
}

Theme::~Theme()
{
	delete fontCache;
	delete atlas;
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
