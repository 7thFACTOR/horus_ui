#pragma once
#include <horus.h>
#include <horus_interfaces.h>
#include <ft2build.h>
#include <freetype/freetype.h>

namespace hui
{
struct FreetypeFontProvider
{
	void initializeFreetype(FT_Library context = 0);
	void shutdownFreetype();
	~FreetypeFontProvider();
	bool loadFont(const char* path, u32 faceSize, FontInfo& outFontInfo);
	bool rasterizeGlyph(GlyphCode glyphCode, FontGlyph* outGlyph);

private:

};

}