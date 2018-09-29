#pragma once
#include <unordered_map>
#include "horus.h"
#include "types.h"
#include "ui_atlas.h"

namespace hui
{
class UiTheme
{
public:
	UiTheme(u32 atlasTextureSize);
	~UiTheme();

	UiImage* addImage(const Rgba32* pixels, u32 width, u32 height);
	void packAtlas();
	inline UiThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void setDefaultWidgetStyle();

	std::unordered_map<std::string, UiFont*> fonts;
	std::unordered_map<std::string, UiImage*> images;
	UiThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, UiThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	UiAtlas* atlas = nullptr;
	FontCache* fontCache = nullptr;
};

}
