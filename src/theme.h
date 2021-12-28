#pragma once
#include <unordered_map>
#include "horus.h"
#include "types.h"
#include "atlas.h"

namespace hui
{
class UiTheme
{
public:
	UiTheme(u32 atlasTextureSize);
	~UiTheme();

	Image* addImage(const Rgba32* pixels, u32 width, u32 height);
	void packAtlas();
	inline UiThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void setDefaultWidgetStyle();

	std::unordered_map<std::string, Font*> fonts;
	std::unordered_map<std::string, Image*> images;
	UiThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, UiThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	Atlas* atlas = nullptr;
	FontCache* fontCache = nullptr;
};

}
