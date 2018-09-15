#pragma once
#include <unordered_map>
#include "horus.h"
#include "types.h"
#include "ui_atlas.h"

namespace hui
{
struct ThemeSettings
{
	f32 radioBulletTextSpacing = 5; /// distance in pixels between radio bullet and its label text
	f32 checkBulletTextSpacing = 5; /// distance in pixels between check bullet and its label text
	f32 comboSliderArrowSideSpacing = 5;
	f32 comboSliderArrowClickSize = 15;
};

class UiTheme
{
public:
	UiTheme(u32 atlasTextureSize);
	~UiTheme();

	UiImage* addImage(const Rgba32* pixels, u32 width, u32 height);
	void packAtlas();
	inline UiThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void cacheBuiltinSettings();

	std::unordered_map<std::string, UiFont*> fonts;
	std::unordered_map<std::string, UiImage*> images;
	UiThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, UiThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	UiAtlas* atlas = nullptr;
	FontCache* fontCache = nullptr;
	ThemeSettings settings;
};

}
