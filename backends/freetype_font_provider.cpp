#include "freetype_font.h"
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include FT_STROKER_H
#include FT_LCD_FILTER_H

#define PIXEL(x) ((((x)+63) & -64)>>6)
#define PIXEL2(x) ((x) >> 6)

namespace hui
{
struct FTContextInfo
{
	FT_Library libHandle;
	bool hasUserLibHandle = false;
};

FTContextInfo ftContext;

void FreetypeFontLoader::initializeFreetype(FT_Library context = 0)
{
	ftContext.libHandle = context;
	ftContext.hasUserLibHandle = context != 0;

	if (!context)
	{
		FT_Init_FreeType(&ftContext.libHandle);
	}
}

void FreetypeFontLoader::shutdownFreetype()
{
	if (!ftContext.hasUserLibHandle)
	{
		FT_Done_FreeType(ftContext.libHandle);
	}
}

FreetypeFontLoader::~FreetypeFontLoader()
{
	if (face)
	{
		FT_Done_Face((FT_Face)face);
	}
}

bool FreetypeFontLoader::loadFont(const char* path, u32 faceSize)
{
	if (face)
	{
		FT_Done_Face((FT_Face)face);
	}

	face = new FT_Face();

	// load the font from the file
	if (FT_New_Face(freetypeLibHandle, fontFilename.c_str(), 0, (FT_Face*)&face))
	{
		FT_Done_Face((FT_Face)face);
		return;
	}

	int error = FT_Select_Charmap((FT_Face)face, FT_ENCODING_UNICODE);

	if (error)
	{
		FT_Done_Face((FT_Face)face);
		return;
	}

	// freetype measures fonts in 64ths of pixels
	//FT_Set_Char_Size((FT_Face)face, faceSize << 6, faceSize << 6, 96, 96);
	FT_Set_Pixel_Sizes((FT_Face)face, 0, faceSize);

	metrics.ascender = PIXEL2(((FT_Face)face)->size->metrics.ascender);
	metrics.descender = PIXEL2(((FT_Face)face)->size->metrics.descender);
	metrics.height = PIXEL2(((FT_Face)face)->size->metrics.height);
	metrics.underlinePosition = PIXEL2(((FT_Face)face)->underline_position);
	metrics.underlineThickness = PIXEL2(((FT_Face)face)->underline_thickness);

	// if its too big, clamp it
	if (metrics.underlinePosition < -2)
		metrics.underlinePosition = -2;

	metrics.underlinePosition = round(metrics.underlinePosition);
}

bool FreetypeFontLoader::resetFaceSize(u32 faceSize)
{

}

FontGlyph* FreetypeFontLoader::getGlyph(GlyphCode glyphCode)
{

}

f32 FreetypeFontLoader::getKerning(GlyphCode glyphCodeLeft, GlyphCode glyphCodeRight)
{

}

const FontMetrics& FreetypeFontLoader::getMetrics() const
{

}

void FreetypeFontLoader::precacheGlyphs(GlyphCode* glyphs, u32 glyphCount)
{

}

}