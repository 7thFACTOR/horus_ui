#pragma once
#include "types.h"
#include "atlas.h"
#include "font.h"

namespace hui
{
// baked line coverage texture layout, shared between theme generation and the renderer
constexpr u32 lineTexMaxWidth = 16;
constexpr u32 lineTexSupersample = 4;
constexpr u32 lineTexFringeTexels = 2;
constexpr u32 lineTexPad = 1;

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
	struct FontVariation
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size = 0;
		u32 usageCount = 0;
	};

	Theme(u32 atlasTextureSize);
	~Theme();
	inline ThemeElement& getElement(WidgetElementId id) { return elements[(u32)id]; }
	void setDefaultStyle();
	void addImagesToAtlas();
	Font* createFont(const std::string& name, const std::string& filename, u32 size);
	Font* createFontFromMemory(const std::string& name, const void* data, u32 dataSize, u32 size);
	void deleteFont(Font* font);
	void deleteFonts();
	void deleteImages();
	void rescaleFonts(f32 scale);
	void addFontGlyphsToAtlas();
	Font* getFont(const char* name);
	Image* getImage(ImageId id);
	void addWhiteImage(u32 width);
	void addLineImage();
	void build();

	Atlas atlas;
	u32 atlasSize = 4069;
	HTexture texture = 0;
	Image* whiteImage = nullptr;
	Image* lineImage = nullptr;
	std::vector<FontVariation*> fonts;
	std::unordered_map<ImageId, Image*> images;
	ThemeElement elements[(u32)WidgetElementId::Count];
	std::unordered_map<std::string, ThemeElement*> userElements;
	std::unordered_map<std::string, std::string> userSettings;
	bool atlasDirty = false;
};

}
