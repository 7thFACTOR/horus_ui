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
	cacheBuiltinSettings();
}

void UiTheme::cacheBuiltinSettings()
{
	if (userSettings.find("radioBulletTextSpacing") != userSettings.end())
		settings.radioBulletTextSpacing = atoi(userSettings["radioBulletTextSpacing"].c_str());

	if (userSettings.find("checkBulletTextSpacing") != userSettings.end())
		settings.checkBulletTextSpacing = atoi(userSettings["checkBulletTextSpacing"].c_str());

	if (userSettings.find("comboSliderArrowSideSpacing") != userSettings.end())
		settings.comboSliderArrowSideSpacing = atoi(userSettings["comboSliderArrowSideSpacing"].c_str());

	if (userSettings.find("comboSliderArrowClickSize") != userSettings.end())
		settings.comboSliderArrowClickSize = atoi(userSettings["comboSliderArrowClickSize"].c_str());
}

void UiTheme::setDefaultWidgetStyle()
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
