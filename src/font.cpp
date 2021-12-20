#include "font.h"
#include "util.h"
#include <assert.h>
#include "horus_interfaces.h"

namespace hui
{
UiFont::UiFont(const std::string& fontFilename, u32 faceSize, UiAtlas* themeAtlas)
{
	load(fontFilename, faceSize, themeAtlas);
}

void UiFont::load(const std::string& fontFilename, u32 facePointSize, UiAtlas* themeAtlas)
{
	filename = fontFilename;
	faceSize = facePointSize;
	atlas = themeAtlas;

}

void UiFont::resetFaceSize(u32 fontFaceSize)
{
	faceSize = fontFaceSize;
	load(filename, faceSize, atlas);
	resizeFaceMode = true;

	for (auto glyph : glyphs)
	{
		cacheGlyph(glyph.first);
	}

	resizeFaceMode = false;
}

UiFont::~UiFont()
{
	HORUS_FONT->freeFont(fontInfo.handle);
	deleteGlyphs();
}

FontGlyph* UiFont::getGlyph(GlyphCode glyphCode)
{
	auto iter = glyphs.find(glyphCode);

	// glyph not cached, do it
	if (iter == glyphs.end())
	{
		return cacheGlyph(glyphCode, true);
	}

	return iter->second;
}

f32 UiFont::getKerning(GlyphCode leftGlyphCode, GlyphCode rightGlyphCode)
{
	u64 hash = ((u64)leftGlyphCode) << 32 + rightGlyphCode;
	auto iter = kerningPairs.find(hash);

	if (iter != kerningPairs.end())
	{
		return iter->second;
	}
	else
	{
		auto kern = HORUS_FONT->getKerning(fontInfo.handle, leftGlyphCode, rightGlyphCode);
		kerningPairs[hash] = kern;

		return kern;
	}

	return 0;
}

void UiFont::precacheGlyphs(const UnicodeString& glyphCodes)
{
	for (auto glyphCode : glyphCodes)
	{
		cacheGlyph(glyphCode);
	}
}

void UiFont::precacheGlyphs(u32* glyphs, u32 glyphCount)
{
	for (size_t i = 0; i < glyphCount; i++)
	{
		cacheGlyph(glyphs[i]);
	}
}

void UiFont::precacheLatinAlphabetGlyphs()
{
	std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()_+-=~`[]{};':\",./<>?®© ";

	for (auto code : alphabet)
	{
		cacheGlyph((GlyphCode)code);
	}
}

FontGlyph* UiFont::cacheGlyph(GlyphCode glyphCode, bool packAtlasNow)
{
	if (!fontInfo.handle)
	{
		return nullptr;
	}

	auto iter = glyphs.find(glyphCode);

	if (iter != glyphs.end() && !resizeFaceMode)
		return iter->second;

	FontGlyph* fontGlyph = resizeFaceMode ? iter->second : new FontGlyph();

	if (resizeFaceMode)
		delete[] fontGlyph->rgbaBuffer;

	HORUS_FONT->rasterizeGlyph(fontInfo.handle, glyphCode, *fontGlyph);

	// if we do not currently resizing the font glyphs, then create and insert the image into the atlas
	if (!resizeFaceMode)
	{
		glyphs.insert(std::make_pair(glyphCode, fontGlyph));
		assert(rgbaBuffer);

		auto image = atlas->addImage(
			fontGlyph->rgbaBuffer,
			fontGlyph->pixelWidth,
			fontGlyph->pixelHeight);

		fontGlyph->image = image;

		if (packAtlasNow)
		{
			atlas->packWithLastUsedParams();
		}
	}
	else
	{
		// if we are in resize mode, then just update the image buffer for the glyph and its size
		delete[] ((UiImage*)fontGlyph->image)->imageData;
		auto imgSize = fontGlyph->pixelWidth * fontGlyph->pixelHeight * sizeof(Rgba32);
		((UiImage*)fontGlyph->image)->imageData = new Rgba32[imgSize];
		((UiImage*)fontGlyph->image)->width = fontGlyph->pixelWidth;
		((UiImage*)fontGlyph->image)->height = fontGlyph->pixelHeight;
		memcpy(((UiImage*)fontGlyph->image)->imageData, fontGlyph->rgbaBuffer, imgSize);
	}

	return fontGlyph;
}

UiImage* UiFont::getGlyphImage(GlyphCode glyphCode)
{
	auto iter = glyphs.find(glyphCode);

	if (iter == glyphs.end())
		return nullptr;

	return (UiImage*)iter->second->image;
}

FontTextSize UiFont::computeTextSize(const UnicodeString& text)
{
	return computeTextSize(text.data(), text.size());
}

FontTextSize UiFont::computeTextSize(const char* text)
{
	static UnicodeString str;

	utf8ToUtf32(text, str);

	return computeTextSize(str.data(), str.size());
}

FontTextSize UiFont::computeTextSize(const GlyphCode* const text, u32 size)
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

		if (glyphImage && glyph)
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

void UiFont::deleteGlyphs()
{
	for (auto glyph : glyphs)
	{
		atlas->deleteImage((UiImage*)glyph.second->image);
		delete[] glyph.second->rgbaBuffer;
		delete glyph.second;
	}

	kerningPairs.clear();
	glyphs.clear();
}

}