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
	void addImagesToAtlas();
	Font* createFont(const std::string& name, const std::string& filename, u32 size);
	void releaseFont(Font* font);
	void deleteFonts();
	void rescaleFonts(f32 scale);
	void addFontGlyphsToAtlas();

	struct CachedFontInfo
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size = 0;
		u32 usageCount = 0;
	};

	std::unordered_map<std::string/*path*/, Image*> images;
	ThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, ThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	Atlas* atlas = nullptr;
	std::unordered_map<std::string/*name*/, FontInfo*> fonts;
};

}
