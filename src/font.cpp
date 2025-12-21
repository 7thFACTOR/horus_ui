#include "horus.h"
#include "font.h"
#include "util.h"
#include <string.h>

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
	static const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()_+-=~`[]{};':\",./<>?®© ";

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
		auto img = (Image*)fontGlyph->image;

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

void Font::cacheEllipsisSize()
{
	ellipsisSize = computeTextSize("...");
}

FontTextSize Font::computeTextSize(const Utf32String& text)
{
	return computeTextSize(text.data(), text.size());
}

FontTextSize Font::computeTextSize(const char* text, u32 maxWidth)
{
	static Utf32String str;

	HORUS_UTF->utf8To32(text, str);

	return computeTextSize(str.data(), str.size(), maxWidth);
}

FontTextSize Font::computeTextSize(const GlyphCode* const text, u32 size, u32 maxWidth)
{
	FontTextSize fsize;
	u32 lastChr = 0;
	u32 lineCount = 1;
	f32 crtLineWidth = 0.0f;
	f32 crtWordWidth = 0.0f;
	u32 currentLineChars = 0;
	u32 longestLineChars = 0;
	u32 lastWordIndex = 0;

	if (size == 0)
	{
		return fsize;
	}

	for (u32 i = 0; i < size; ++i)
	{
		auto chr = text[i];

		// explicit newline -> finish current line and start new one
		if (chr == '\n')
		{
			if (fsize.width < crtLineWidth) fsize.width = crtLineWidth;

			fsize.lineHeights.push_back(fontInfo.metrics.height);
			if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;

			// reset for next line
			crtLineWidth = 0.0f;
			crtWordWidth = 0.0f;
			currentLineChars = 0;
			lastChr = 0;
			lastWordIndex = i + 1;
			++lineCount;
			continue;
		}

		auto glyph = getGlyph(chr);
		if (!glyph)
			continue;

		// track word boundaries
		if (chr == ' ')
		{
			lastWordIndex = i + 1;
			crtWordWidth = 0.0f;
		}

		// compute advance including kerning with previous glyph on same line
		auto kern = getKerning(lastChr, chr);
		f32 glyphAdvance = glyph->advanceX + kern;
		f32 projectedLineWidth = crtLineWidth + glyphAdvance;
		f32 projectedWordWidth = crtWordWidth + glyphAdvance;

		// NOTE: use >= to match the draw-time check (avoids 1-pixel flip on border)
		if (maxWidth != ~0u && projectedLineWidth >= (f32)maxWidth)
		{
			// If the current word itself exceeds the line, break inside the word
			if (projectedWordWidth >= (f32)maxWidth)
			{
				// find break position inside the word (from lastWordIndex to i)
				f32 wordSize = 0.0f;
				u32 breakPos = lastWordIndex;
				GlyphCode localLast = 0;

				for (u32 k = lastWordIndex; k <= i; ++k)
				{
					auto g2 = getGlyph(text[k]);
					if (!g2) continue;

					auto kern2 = getKerning(localLast, text[k]);
					f32 cw = g2->advanceX + kern2;
					wordSize += cw;

					// use >= here as well to be consistent
					if (wordSize >= (f32)maxWidth)
					{
						breakPos = k;
						break;
					}

					localLast = text[k];
				}

				// finalize current line
				if (fsize.width < crtLineWidth) fsize.width = crtLineWidth;
				fsize.lineHeights.push_back(fontInfo.metrics.height);
				if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;
				++lineCount;

				// start new line at breakPos
				crtLineWidth = 0.0f;
				crtWordWidth = 0.0f;
				currentLineChars = 0;
				lastChr = 0;

				// set iterator so loop will process breakPos next
				if (breakPos > 0) i = breakPos - 1;
				else i = breakPos;

				continue;
			}
			else
			{
				// move whole word to next line
				if (fsize.width < crtLineWidth) fsize.width = crtLineWidth;
				fsize.lineHeights.push_back(fontInfo.metrics.height);
				if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;
				++lineCount;

				crtLineWidth = 0.0f;
				crtWordWidth = 0.0f;
				currentLineChars = 0;
				lastChr = 0;

				if (lastWordIndex > 0) i = lastWordIndex - 1;
				else i = lastWordIndex;

				continue;
			}
		}

		// accept glyph into current line
		crtLineWidth = projectedLineWidth;
		crtWordWidth = projectedWordWidth;
		++currentLineChars;
		lastChr = chr;

		// keep vertical metrics for centering
		f32 top = glyph->bearingY;
		f32 bottom = -(glyph->pixelHeight - glyph->bearingY);
		f32 glyphHeight = fabs(top - bottom);

		if (fsize.maxGlyphHeight < glyphHeight) fsize.maxGlyphHeight = glyphHeight;
		if (fsize.maxBearingY < glyph->bearingY) fsize.maxBearingY = glyph->bearingY;
	}

	// finalize last line
	if (fsize.width < crtLineWidth) fsize.width = crtLineWidth;

	// If text ends with a trailing newline, we previously incremented lineCount and pushed a lineHeight
	// for the (non-existent) line after the newline. Remove that empty trailing line so computeTextSize
	// matches drawMultilineText behavior.
	if (size > 0 && text[size - 1] == '\n')
	{
		if (lineCount > 0) --lineCount;
		if (!fsize.lineHeights.empty())
			fsize.lineHeights.pop_back();
	}

	// ensure last line height recorded
	if (fsize.lineHeights.empty() || (fsize.lineHeights.size() < lineCount))
		fsize.lineHeights.push_back(fontInfo.metrics.height);

	// finalize longest line char count with the last line
	if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;

	// total height = number of lines * metrics.height
	fsize.height = (f32)lineCount * fontInfo.metrics.height;

	// maxLength is longest visible glyph count per line
	fsize.maxLength = longestLineChars;

	return fsize;
}

void Font::deleteGlyphs()
{
	for (auto& glyph : glyphs)
	{
		atlas->deleteImage((Image*)glyph.second->image);
		delete[] glyph.second->rgbaBuffer;
		delete glyph.second;
	}

	kerningPairs.clear();
	glyphs.clear();
}

}