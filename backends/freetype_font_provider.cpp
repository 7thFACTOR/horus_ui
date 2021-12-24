#include "freetype_font_provider.h"
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

FreetypeFontProvider::FreetypeFontProvider(FT_Library context)
{
	initializeFreetype(context);
}

FreetypeFontProvider::~FreetypeFontProvider()
{
	shutdownFreetype();
}

void FreetypeFontProvider::initializeFreetype(FT_Library context)
{
	ftContext.libHandle = context;
	ftContext.hasUserLibHandle = context != 0;

	if (!context)
	{
		FT_Init_FreeType(&ftContext.libHandle);
	}
}

void FreetypeFontProvider::shutdownFreetype()
{
	if (!ftContext.hasUserLibHandle)
	{
		FT_Done_FreeType(ftContext.libHandle);
	}
}

bool FreetypeFontProvider::loadFont(const char* path, u32 faceSize, FontInfo& outFontInfo)
{
	auto face = new FT_Face();

	// load the font from the file
	if (FT_New_Face(ftContext.libHandle, path, 0, (FT_Face*)&face))
	{
		FT_Done_Face((FT_Face)face);
		return false;
	}

	int error = FT_Select_Charmap((FT_Face)face, FT_ENCODING_UNICODE);

	if (error)
	{
		FT_Done_Face((FT_Face)face);
		return false;
	}

	// freetype measures fonts in 64ths of pixels
	//FT_Set_Char_Size((FT_Face)face, faceSize << 6, faceSize << 6, 96, 96);
	FT_Set_Pixel_Sizes((FT_Face)face, 0, faceSize);

	outFontInfo.metrics.ascender = PIXEL2(((FT_Face)face)->size->metrics.ascender);
	outFontInfo.metrics.descender = PIXEL2(((FT_Face)face)->size->metrics.descender);
	outFontInfo.metrics.height = PIXEL2(((FT_Face)face)->size->metrics.height);
	outFontInfo.metrics.underlinePosition = PIXEL2(((FT_Face)face)->underline_position);
	outFontInfo.metrics.underlineThickness = PIXEL2(((FT_Face)face)->underline_thickness);

	// if its too big, clamp it
	if (outFontInfo.metrics.underlinePosition < -2)
		outFontInfo.metrics.underlinePosition = -2;

	outFontInfo.metrics.underlinePosition = round(outFontInfo.metrics.underlinePosition);
	outFontInfo.fontFace = face;

	return true;
}

void FreetypeFontProvider::freeFont(HFontFace font)
{
	if (font)
	{
		FT_Done_Face((FT_Face)font);
	}
}

f32 FreetypeFontProvider::getKerning(HFontFace font, GlyphCode leftGlyphCode, GlyphCode rightGlyphCode)
{
	FT_Vector kerning;

	FT_Get_Kerning(
		(FT_Face)font,
		FT_Get_Char_Index((FT_Face)font, leftGlyphCode),
		FT_Get_Char_Index((FT_Face)font, rightGlyphCode),
		FT_KERNING_DEFAULT,
		&kerning);

	return kerning.x >> 6;
}

bool FreetypeFontProvider::rasterizeGlyph(HFontFace font, GlyphCode glyphCode, FontGlyph& outGlyph)
{
	FT_GlyphSlot slot = ((FT_Face)font)->glyph;

	// FT_LCD_FILTER_LIGHT   is (0x00, 0x55, 0x56, 0x55, 0x00)
	// FT_LCD_FILTER_DEFAULT is (0x10, 0x40, 0x70, 0x40, 0x10)
	u8 lcd_weights[10];

	lcd_weights[0] = 0x10;
	lcd_weights[1] = 0x40;
	lcd_weights[2] = 0x70;
	lcd_weights[3] = 0x40;
	lcd_weights[4] = 0x10;

	int flags = FT_LOAD_FORCE_AUTOHINT;
	FT_Library_SetLcdFilter(ftContext.libHandle, FT_LCD_FILTER_LIGHT);
	flags |= FT_LOAD_TARGET_LCD;
	FT_Library_SetLcdFilterWeights(ftContext.libHandle, lcd_weights);

	if (FT_Load_Glyph(
		(FT_Face)font,
		FT_Get_Char_Index((FT_Face)font, glyphCode),
		flags))
	{
		return false;
	}

	if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL))
	{
		return false;
	}

	FT_Bitmap bitmap = slot->bitmap;
	u32 width = bitmap.width;
	u32 height = bitmap.rows;
	Rgba32* rgbaBuffer = new Rgba32[(size_t)width * height];

	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			u8 lum = bitmap.buffer[i + width * j];
			u32 index = (i + j * width);
			rgbaBuffer[index] = ~0;
			*((u8*)&(rgbaBuffer[index]) + 3) = lum;
		}
	}

	outGlyph.pixelWidth = width;
	outGlyph.pixelHeight = height;
	outGlyph.rgbaBuffer = rgbaBuffer;
	outGlyph.code = glyphCode;
	outGlyph.advanceX = ((FT_Face)font)->glyph->advance.x >> 6;
	outGlyph.advanceY = ((FT_Face)font)->glyph->advance.y >> 6;
	outGlyph.bearingX = ((FT_Face)font)->glyph->metrics.horiBearingX >> 6;
	outGlyph.bearingY = ((FT_Face)font)->glyph->metrics.horiBearingY >> 6;
	outGlyph.bitmapLeft = ((FT_Face)font)->glyph->bitmap_left;
	outGlyph.bitmapTop = ((FT_Face)font)->glyph->bitmap_top;

	return true;
}

}