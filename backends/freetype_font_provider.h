#pragma once
#include <horus.h>
#include <horus_interfaces.h>
#include <ft2build.h>
#include <freetype/freetype.h>

namespace hui
{
struct FreetypeFontProvider : FontProvider
{
	FreetypeFontProvider(FT_Library context = 0);
	~FreetypeFontProvider();
	bool loadFont(const char* path, u32 faceSize, FontInfo& outFontInfo) override;
	void freeFont(HFontFace font) override;
	f32 getKerning(HFontFace font, GlyphCode leftGlyphCode, GlyphCode rightGlyphCode) override;
	bool rasterizeGlyph(HFontFace font, GlyphCode glyphCode, FontGlyph& outGlyph) override;

private:
	void initializeFreetype(FT_Library context = 0);
	void shutdownFreetype();
};

}