#pragma once
#include "types.h"
#include "atlas.h"
#include "font.h"

namespace hui
{
struct ThemeImage
{
	Image* atlasImage = nullptr;
	ImageData imageData;
};

struct Theme
{
	Theme(u32 atlasTextureSize);
	~Theme();

	inline ThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void setDefaultWidgetStyle();
	void addImagesToAtlas();
	Font* createFont(const std::string& name, const std::string& filename, u32 size);
	void deleteFont(Font* font);
	void deleteFonts();
	void deleteImages();
	void rescaleFonts(f32 scale);
	void addFontGlyphsToAtlas();
	Font* getFont(const char* name);

	struct FontVariation
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size = 0;
		u32 usageCount = 0;
	};

	Atlas* atlas = nullptr;
	std::vector<FontVariation*> fonts;
	std::unordered_map<std::string/*path*/, ThemeImage> images;
	ThemeElement elements[(int)WidgetElementId::Count];
	std::unordered_map<std::string, ThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
};

}
