#pragma once
#include "types.h"
#include "atlas.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace hui
{
class Font
{
public:
	Font() {}
	Font(const std::string& fontFilename, u32 fontFaceSize, Atlas* themeAtlas);
	~Font();

	void load(const std::string& fontFilename, u32 fontFaceSize, Atlas* themeAtlas);
	void resetFaceSize(u32 fontFaceSize);
	FontGlyph* getGlyph(GlyphCode glyphCode);
	Image* getGlyphImage(GlyphCode glyphCode);
	f32 getKerning(GlyphCode leftGlyphCode, GlyphCode rightGlyphCode);
	const FontMetrics& getMetrics() const { return fontInfo.metrics; }
	void precacheGlyphs(const Utf32String& glyphCodes);
	void precacheGlyphs(u32* glyphs, u32 glyphCount);
	void precacheLatinAlphabetGlyphs();
	FontTextSize computeTextSize(const GlyphCode* const text, u32 size);
	FontTextSize computeTextSize(const Utf32String& text);
	FontTextSize computeTextSize(const char* text);
	void deleteGlyphs();

	Atlas* atlas = nullptr;

protected:
	FontGlyph* cacheGlyph(GlyphCode glyphCode);

	FontInfo fontInfo;
	bool resizeFaceMode = false;
	std::string filename;
	u32 faceSize = 12;
	f32 ascender = 0;
	std::unordered_map<GlyphCode, FontGlyph*> glyphs;
	std::unordered_map<u64, f32> kerningPairs;
};

}