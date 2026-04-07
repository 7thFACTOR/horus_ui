#pragma once
#include "types.h"
#include "atlas.h"
#include "font.h"

namespace hui
{
struct Image
{
	ImageId id = 0;
	std::vector<Rgba32> pixels;
	u32 width = 0, height = 0;
	bool rotated = false;
	Rect uvRect;
	Rect rect;
};

struct Theme
{
	Theme(u32 atlasTextureSize);
	~Theme();

	inline ThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void widgetSetDefaultStyle();
	void addImagesToAtlas();
	Font* createFont(const std::string& name, const std::string& filename, u32 size);
	void deleteFont(Font* font);
	void deleteFonts();
	void deleteImages();
	void rescaleFonts(f32 scale);
	void addFontGlyphsToAtlas();
	Font* getFont(const char* name);
	Image* getImage(ImageId id);
	void addWhiteImage(u32 width);
	void build();

	struct FontVariation
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size = 0;
		u32 usageCount = 0;
	};

	Atlas atlas;
	u32 atlasSize = 4069;
	HTexture texture = 0;
	Image* whiteImage = nullptr;
	std::vector<FontVariation*> fonts;
	std::unordered_map<ImageId, Image*> images;
	ThemeElement elements[(u32)WidgetElementId::Count];
	std::unordered_map<std::string, ThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
};

}
