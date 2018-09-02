#pragma once
#include "types.h"
#include "ui_atlas.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace hui
{
struct FontGlyph
{
	UiImage* image = nullptr;
	GlyphCode code = 0;
	f32 bearingX = 0.0f;
	f32 bearingY = 0.0f;
	f32 advanceX = 0.0f;
	f32 advanceY = 0.0f;
	i32 bitmapLeft = 0;
	i32 bitmapTop = 0;
	u32 pixelWidth = 0;
	u32 pixelHeight = 0;
	i32 pixelX = 0;
	i32 pixelY = 0;
	Rgba32* rgbaBuffer = nullptr;
};

struct FontKerningPair
{
	GlyphCode glyphLeft = 0;
	GlyphCode glyphRight = 0;
	f32 kerning = 0.0f;
};

struct FontMetrics
{
	f32 height;
	f32 ascender;
	f32 descender;
	f32 underlinePosition;
	f32 underlineThickness;
};

struct FontTextSize
{
	f32 width = 0;
	f32 height = 0;
	f32 maxBearingY = 0;
	f32 maxGlyphHeight = 0;
	u32 lastFontIndex = 0;
	std::vector<f32> lineHeights;
};

class UiFont
{
public:
	UiFont() {}
	UiFont(const std::string& fontFilename, u32 fontFaceSize, UiAtlas* themeAtlas);
	~UiFont();

	void load(const std::string& fontFilename, u32 fontFaceSize, UiAtlas* themeAtlas);
	void resetFaceSize(u32 fontFaceSize);
	FontGlyph* getGlyph(GlyphCode glyphCode);
	UiImage* getGlyphImage(GlyphCode glyphCode);
	f32 getKerning(GlyphCode glyphCodeLeft, GlyphCode glyphCodeRight);
	const FontMetrics& getMetrics() const { return metrics; }
	void precacheGlyphs(const UnicodeString& glyphCodes);
	void precacheGlyphs(u32* glyphs, u32 glyphCount);
	void precacheLatinAlphabetGlyphs();
	FontTextSize computeTextSize(const GlyphCode* const text, u32 size);
	FontTextSize computeTextSize(const UnicodeString& text);
	FontTextSize computeTextSize(const char* text);
	void deleteGlyphs();

	UiAtlas* atlas = nullptr;

protected:
	FontGlyph* cacheGlyph(GlyphCode glyphCode, bool packAtlasNow = false);

	bool resizeFaceMode = false;
	std::string filename;
	u32 faceSize = 12;
	f32 ascender = 0;
	FontMetrics metrics;
	void* face = 0;
	std::map<GlyphCode, FontGlyph*> glyphs;
	std::unordered_map<u64, f32> kerningPairs;
};

}