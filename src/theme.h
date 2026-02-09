#pragma once
#include "types.h"
#include "atlas.h"

namespace hui
{
struct Theme
{
	Theme(u32 atlasTextureSize);
	~Theme();

	inline ThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void setDefaultWidgetStyle();

	std::unordered_map<std::string/*path*/, Font*> fonts;
	std::unordered_map<std::string/*path*/, Image*> images;
	ThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, ThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	Atlas* atlas = nullptr;
	FontCache* fontCache = nullptr;
};

}
