#include "ui_context.h"
#include "ui_font.h"
#include "renderer.h"
#include "util.h"
#include "unicode_text_cache.h"
#include <string.h>

namespace hui
{
UiContext* ctx = nullptr;

Rect UiContext::drawMultilineText(
	const char* text,
	const Rect& rect,
	HAlignType horizontal,
	VAlignType vertical)
{
	// wrap text while finding lines
	f32 crtWidth = 0;
	f32 crtWordWidth = 0;
	u32 lastWordIndex = 0;
	GlyphCode lastGlyphCode = 0;
	Rect newRect = rect;

	if (!strcmp(text, ""))
		return newRect;

	TextLine line;
	auto crtFont = renderer->getFont();

	newRect.height = 0;
	textLines.resize(0);

	UnicodeString utext;

	utf8ToUtf32(text, utext);

	for (int i = 0, iCount = utext.size(); i < iCount; i++)
	{
		auto chr = utext[i];

		if (chr == '\n')
		{
			crtWidth = 0;
			crtWordWidth = 0;
			lastWordIndex = i;

			line.length = i - line.start;
			textLines.push_back(line);
			line.start = i;

			continue;
		}

		auto glyph = crtFont->getGlyph(chr);

		if (!glyph)
			continue;

		if (chr == ' ')
		{
			lastWordIndex = i + 1;
			crtWordWidth = 0;
		}

		auto kern = crtFont->getKerning(lastGlyphCode, chr);
		f32 charWidth = glyph->advanceX + kern;

		crtWidth += charWidth;
		crtWordWidth += charWidth;

		if (crtWidth >= rect.width)
		{
			if (crtWordWidth >= rect.width)
			{
				// break the word which is bigger than the maximum line width allowed
				f32 wordSize = 0;
				int k = 0;

				lastGlyphCode = utext[lastWordIndex];

				for (k = lastWordIndex; k < i; k++)
				{
					auto kern2 = crtFont->getKerning(lastGlyphCode, utext[k]);
					auto glyph2 = crtFont->getGlyph(utext[k]);

					if (!glyph2)
						continue;

					f32 charWidth = glyph2->advanceX + kern2;

					wordSize += charWidth;

					if (wordSize > rect.width)
					{
						lastWordIndex = k;
						break;
					}

					lastGlyphCode = utext[k];
				}

				line.length = lastWordIndex - line.start;
				textLines.push_back(line);
				line.start = lastWordIndex;
				crtWordWidth = 0;
				crtWidth = 0;

				if (!line.start && !line.length)
					break;
				//TODO: make sure if there is one single char not fitting in the max width to just clip it
				// otherwise we get into an infinite loop
				i = k;
			}
			else
			{
				// move onto next line
				line.length = lastWordIndex - line.start;
				textLines.push_back(line);
				line.start = lastWordIndex;
				i = lastWordIndex;
				crtWordWidth = 0;
				crtWidth = 0;
				lastGlyphCode = 0;
			}
		}
	}

	line.length = utext.size() - line.start;
	textLines.push_back(line);

	Point pos;

	switch (vertical)
	{
	case hui::VAlignType::Top:
		pos.y = rect.y + crtFont->getMetrics().ascender;
		break;
	case hui::VAlignType::Bottom:
		pos.y = rect.bottom() - textLines.size() * crtFont->getMetrics().height;
		break;
	case hui::VAlignType::Center:
		if (rect.height <= 0.0f)
			pos.y = rect.y + crtFont->getMetrics().ascender;
		else
			pos.y = rect.y + crtFont->getMetrics().ascender + (rect.height - textLines.size() * crtFont->getMetrics().height) / 2.0f;
		break;
	default:
		break;
	}

	for (size_t i = 0; i < textLines.size(); i++)
	{
		auto& line = textLines[i];
		FontTextSize fsize = crtFont->computeTextSize(utext.data() + line.start, line.length);

		switch (horizontal)
		{
		case hui::HAlignType::Left:
			pos.x = rect.x;
			break;
		case hui::HAlignType::Right:
			pos.x = rect.right() - fsize.width;
			break;
		case hui::HAlignType::Center:
			pos.x = rect.x + (rect.width - fsize.width) / 2.0f;
			break;
		default:
			break;
		}

		UnicodeString lineText;

		lineText = UnicodeString(utext.begin() + line.start, utext.begin() + line.start + line.length);

		static char strUtf8[1024] = { 0 };

		utf32ToUtf8NoAlloc(lineText, strUtf8, 1024);
		renderer->cmdDrawTextAt(strUtf8, pos);

		pos.y += crtFont->getMetrics().height;
	}

	newRect.height = fabs(rect.height - textLines.size() * crtFont->getMetrics().height);

	return newRect;
}

void UiContext::setSkipRenderAndInput(bool skip)
{
	skipRenderAndInput = skip;
	renderer->skipRender = skip;
}

void UiContext::initializeGraphics()
{
	if (!renderer)
	{
		renderer = new Renderer();
		textCache = new UnicodeTextCache();
	}
}

}
