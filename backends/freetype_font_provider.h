#pragma once
#include <horus.h>
#include <horus_interfaces.h>
#include <ft2build.h>
#include <freetype/freetype.h>

namespace hui
{
struct FreetypeFontProvider : FontProvider
{
	void initializeFreetype(FT_Library context = 0);
	void shutdownFreetype();
	bool loadFont(const char* path, u32 faceSize, FontInfo& outFontInfo) override;
	void freeFont(FontHandle font) override;
	f32 getKerning(FontHandle font, GlyphCode leftGlyphCode, GlyphCode rightGlyphCode) override;
	bool rasterizeGlyph(FontHandle font, GlyphCode glyphCode, FontGlyph& outGlyph) override;
};

}