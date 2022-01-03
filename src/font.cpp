#include "font.h"
#include "util.h"
#include <assert.h>
#include "horus_interfaces.h"

namespace hui
{
Font::Font(const std::string& fontFilename, u32 faceSize, Atlas* themeAtlas)
{
	load(fontFilename, faceSize, themeAtlas);
}

void Font::load(const std::string& fontFilename, u32 facePointSize, Atlas* themeAtlas)
{
	filename = fontFilename;
	faceSize = facePointSize;
	atlas = themeAtlas;

	if (fontInfo.fontFace)
	{
		HORUS_FONT->freeFont(fontInfo.fontFace);
		fontInfo.fontFace = 0;
	}

	if (!HORUS_FONT->loadFont(fontFilename.c_str(), facePointSize, fontInfo))
		return;
}

void Font::resetFaceSize(u32 fontFaceSize)
{
	faceSize = fontFaceSize;
	load(filename, faceSize, atlas);
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
		HORUS_FONT->freeFont(fontInfo.fontFace);
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
	u64 hash = ((u64)leftGlyphCode) << 32 + rightGlyphCode;
	auto iter = kerningPairs.find(hash);

	if (iter != kerningPairs.end())
	{
		return iter->second;
	}
	else
	{
		auto kern = HORUS_FONT->getKerning(fontInfo.fontFace, leftGlyphCode, rightGlyphCode);
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
	std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()_+-=~`[]{};':\",./<>?®© ";

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

	auto ret = HORUS_FONT->rasterizeGlyph(fontInfo.fontFace, glyphCode, *fontGlyph);

	// if we do not currently resizing the font glyphs, then create and insert the image into the atlas
	if (!resizeFaceMode)
	{
		glyphs.insert(std::make_pair(glyphCode, fontGlyph));

		auto image = atlas->addImage(
			fontGlyph->rgbaBuffer,
			fontGlyph->pixelWidth,
			fontGlyph->pixelHeight);

		fontGlyph->image = image;
	}
	else
	{
		// if we are in resize mode, then just update the image buffer for the glyph and its size
		auto img = ((Image*)fontGlyph->image);
		if (img)
		{
			delete[] img->imageData;
			auto imgSize = (size_t)fontGlyph->pixelWidth * fontGlyph->pixelHeight * sizeof(Rgba32);
			img->imageData = new Rgba32[imgSize];
			img->width = fontGlyph->pixelWidth;
			img->height = fontGlyph->pixelHeight;
			memcpy(img->imageData, fontGlyph->rgbaBuffer, imgSize);
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

FontTextSize Font::computeTextSize(const Utf32String& text)
{
	return computeTextSize(text.data(), text.size());
}

FontTextSize Font::computeTextSize(const char* text)
{
	static Utf32String str;

	HORUS_UTF->utf8To32(text, str);

	return computeTextSize(str.data(), str.size());
}

FontTextSize Font::computeTextSize(const GlyphCode* const text, u32 size)
{
	FontTextSize fsize;
	u32 lastChr = 0;
	f32 lineCount = 0;
	f32 crtLineWidth = 0;

	for (size_t i = 0; i < size; i++)
	{
		auto chr = text[i];
		auto glyph = getGlyph(chr);
		auto glyphImage = glyph->image;

		if (chr == '\n')
		{
			if (fsize.width < crtLineWidth)
			{
				fsize.width = crtLineWidth;
			}

			crtLineWidth = 0;
			lineCount++;
			continue;
		}

		if (glyph)
		{
			f32 top = glyph->bearingY;
			f32 bottom = -(glyph->pixelHeight - glyph->bearingY);
			auto kern = getKerning(lastChr, chr);
			crtLineWidth += glyph->advanceX + kern;
			lastChr = chr;

			if (fsize.maxGlyphHeight < fabs(top - bottom))
			{
				fsize.maxGlyphHeight = fabs(top - bottom);
			}

			if (fsize.maxBearingY < glyph->bearingY)
			{
				fsize.maxBearingY = glyph->bearingY;
			}
		}
	}

	if (fsize.width < crtLineWidth)
	{
		fsize.width = crtLineWidth;
	}

	lineCount++;
	fsize.height = lineCount * fontInfo.metrics.height;

	return fsize;
}

void Font::deleteGlyphs()
{
	for (auto glyph : glyphs)
	{
		atlas->deleteImage((Image*)glyph.second->image);
		delete[] glyph.second->rgbaBuffer;
		delete glyph.second;
	}

	kerningPairs.clear();
	glyphs.clear();
}

}