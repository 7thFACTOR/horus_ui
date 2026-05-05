#include "horus.h"
#include "font.h"
#include "util.h"
#include "theme.h"
#include <string.h>
#include "context.h"

namespace hui
{
Font::Font(const std::string& fontFilename, u32 faceSize)
{
	load(fontFilename, faceSize);
}

void Font::load(const std::string& fontFilename, u32 facePointSize)
{
	filename = fontFilename;
	faceSize = facePointSize;

	if (fontInfo.fontFace)
	{
		ctx->settings.services.freeFont(fontInfo.fontFace);
		fontInfo.fontFace = 0;
	}

	if (!ctx->settings.services.loadFont(fontFilename.c_str(), facePointSize, fontInfo))
		return;
}

void Font::loadFromMemory(const void* data, size_t size, u32 facePointSize)
{
	fontData = data;
	fontDataSize = size;
	faceSize = facePointSize;

	if (fontInfo.fontFace)
	{
		ctx->settings.services.freeFont(fontInfo.fontFace);
		fontInfo.fontFace = 0;
	}

	if (!ctx->settings.services.loadFontFromMemory(data, size, facePointSize, fontInfo))
		return;
}

void Font::resetFaceSize(u32 fontFaceSize)
{
	faceSize = fontFaceSize;
	if (fontData)
		loadFromMemory(fontData, fontDataSize, faceSize);
	else
		load(filename, faceSize);
	resizeFaceMode = true;

	for (auto& glyph : glyphs)
	{
		cacheGlyph(glyph.first);
	}

	resizeFaceMode = false;
}

Font::~Font()
{
	if (fontInfo.fontFace)
	{
		ctx->settings.services.freeFont(fontInfo.fontFace);
	}

	deleteGlyphs();
}

FontGlyph* Font::getGlyph(GlyphCode glyphCode)
{
	auto iter = glyphs.find(glyphCode);

	// glyph not cached, do it
	if (iter == glyphs.end())
	{
		return cacheGlyph(glyphCode);
	}

	return iter->second;
}

f32 Font::getKerning(GlyphCode leftGlyphCode, GlyphCode rightGlyphCode)
{
	// Compose a 64-bit key: left in high 32 bits, right in low 32 bits.
	// Parentheses are required because << has lower precedence than +.
	u64 hash = (((u64)leftGlyphCode) << 32) | (u64)rightGlyphCode;
	auto iter = kerningPairs.find(hash);

	if (iter != kerningPairs.end())
	{
		return iter->second;
	}
	else
	{
		auto kern = ctx->settings.services.getFontKerning(fontInfo.fontFace, leftGlyphCode, rightGlyphCode);
		kerningPairs[hash] = kern;

		return kern;
	}

	return 0;
}

void Font::precacheGlyphs(const Utf32String& glyphCodes)
{
	for (auto glyphCode : glyphCodes)
	{
		cacheGlyph(glyphCode);
	}
}

void Font::precacheGlyphs(u32* glyphs, u32 glyphCount)
{
	for (size_t i = 0; i < glyphCount; i++)
	{
		cacheGlyph(glyphs[i]);
	}
}

void Font::precacheLatinAlphabetGlyphs()
{
	static const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()_+-=~`[]{};':\",./<>?\\|?®© ";

	for (auto code : alphabet)
	{
		cacheGlyph((GlyphCode)code);
	}
}

FontGlyph* Font::cacheGlyph(GlyphCode glyphCode)
{
	if (!fontInfo.fontFace)
	{
		return nullptr;
	}

	auto iter = glyphs.find(glyphCode);

	if (iter != glyphs.end() && !resizeFaceMode)
		return iter->second;

	FontGlyph* fontGlyph = resizeFaceMode ? iter->second : new FontGlyph();

	if (resizeFaceMode)
		delete[] fontGlyph->rgbaBuffer;

	auto ret = ctx->settings.services.rasterizeFontGlyph(fontInfo.fontFace, glyphCode, *fontGlyph);

	// if we do not currently resizing the font glyphs
	if (!resizeFaceMode)
	{
		glyphs.insert(std::make_pair(glyphCode, fontGlyph));
	}
	else
	{
		// if we are in resize mode, then just contextUpdate the image buffer for the glyph and its size
		auto img = (Image*)fontGlyph->image;

		if (img)
		{
			img->pixels.clear();
			auto imgSize = (size_t)fontGlyph->pixelWidth * fontGlyph->pixelHeight * sizeof(Rgba32);
			img->pixels.resize(imgSize);
			img->width = fontGlyph->pixelWidth;
			img->height = fontGlyph->pixelHeight;
			
			if (imgSize)
			{
				memcpy(&img->pixels[0], fontGlyph->rgbaBuffer, imgSize);
			}
		}
	}

	return fontGlyph;
}

Image* Font::getGlyphImage(GlyphCode glyphCode)
{
	auto iter = glyphs.find(glyphCode);

	if (iter == glyphs.end())
		return nullptr;

	return (Image*)iter->second->image;
}

void Font::deleteGlyphs()
{
	for (auto& glyph : glyphs)
	{
		delete[] glyph.second->rgbaBuffer;
		delete glyph.second;
	}

	kerningPairs.clear();
	glyphs.clear();
}

FontTextSize Font::computeTextSize(const GlyphCode* const text, u32 size, u32 maxWidth)
{
	FontTextSize fsize;

	if (ctx)
	{
		// glyph-array overload on renderer expects (text, size, position, outSize, doDraw, font, maxWidth)
		return ctx->renderer.computeSizeOrDrawText(text, size, Rect(0, 0, FLT_MAX, FLT_MAX), HAlignType::Left, VAlignType::Top, false, this);
	}
	
	return fsize;
}

FontTextSize Font::computeTextSize(const Utf32String& text)
{
	FontTextSize fsize;

	if (ctx)
	{
		return ctx->renderer.computeSizeOrDrawText(text.data(), (u32)text.size(), Rect(0, 0, FLT_MAX, FLT_MAX), HAlignType::Left, VAlignType::Top, false, this);
	}

	return fsize;
}

FontTextSize Font::computeTextSize(const char* text, u32 maxWidth)
{
	FontTextSize fsize;

	if (ctx)
	{
		return ctx->renderer.computeSizeOrDrawText(text, Rect(0, 0, maxWidth, FLT_MAX), HAlignType::Left, VAlignType::Top, false, this);
	}
	
	return fsize;
}

}