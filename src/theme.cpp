#include "theme.h"
#include "util.h"
#include "context.h"

namespace hui
{
Theme::Theme(u32 atlasTextureSize)
{
	atlasSize = atlasTextureSize;
	addWhiteImage(32);
	addLineImage();

	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
		elements[i].styles.reserve((u32)WidgetStateType::Count);

	setDefaultStyle();
}

Theme::~Theme()
{
	deleteImages();
	deleteFonts();
}

void Theme::addWhiteImage(u32 width)
{
	u32 whiteImageSize = width * width;

	whiteImage = new Image();
	whiteImage->pixels.resize(whiteImageSize);
	whiteImage->width = width;
	whiteImage->height = width;
	memset(whiteImage->pixels.data(), 0xFF, (size_t)whiteImageSize * sizeof(Rgba32));
	whiteImage->id = hashString("__WHITEIMAGE__");
	images[whiteImage->id] = whiteImage;
}

void Theme::addLineImage()
{
	// each width gets a row holding a baked coverage profile: solid core surrounded by a
	// super sampled anti-aliasing fringe, with a zero alpha guard texel at both ends so
	// the outer edge of the line fades cleanly to nothing under bilinear filtering
	lineImage = new Image();
	lineImage->id = hashString("__LINEIMAGE__");
	lineImage->width = lineTexPad * 2 + lineTexFringeTexels * 2 + lineTexMaxWidth * lineTexSupersample;
	lineImage->height = lineTexMaxWidth + 1;
	lineImage->pixels.resize((size_t)lineImage->width * lineImage->height);
	memset(lineImage->pixels.data(), 0, lineImage->pixels.size() * sizeof(Rgba32));

	for (u32 w = 1; w <= lineTexMaxWidth; w++)
	{
		u32 x = 0;
		const u32 coreTexels = w * lineTexSupersample;

		auto writeTexel = [&](u8 alpha)
		{
			lineImage->pixels[(size_t)w * lineImage->width + x] = (Rgba32)(0x00FFFFFFu | ((u32)alpha << 24));
			x++;
		};

		writeTexel(0);

		for (u32 i = 0; i < lineTexFringeTexels; i++)
		{
			f32 t = (f32)(i + 1) / (f32)(lineTexFringeTexels + 1);
			writeTexel((u8)(t * 255.0f + 0.5f));
		}

		for (u32 i = 0; i < coreTexels; i++)
		{
			writeTexel(255);
		}

		for (u32 i = lineTexFringeTexels; i > 0; i--)
		{
			f32 t = (f32)i / (f32)(lineTexFringeTexels + 1);
			writeTexel((u8)(t * 255.0f + 0.5f));
		}

		writeTexel(0);
	}

	images[lineImage->id] = lineImage;
}

void Theme::setDefaultStyle()
{
	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		elements[i].setDefaultStyle();

		for (auto& stylePair : elements[i].styles)
		{
			for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
			{
				if (!stylePair.second.states[j].image)
					stylePair.second.states[j].image = whiteImage;
			}
		}
	}

	for (auto& elem : userElements)
	{
		elem.second->setDefaultStyle();

		for (auto& stylePair : elem.second->styles)
		{
			for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
			{
				if (!stylePair.second.states[j].image)
					stylePair.second.states[j].image = whiteImage;
			}
		}
	}
}

void Theme::addImagesToAtlas()
{
	for (auto& img : images)
	{
		atlas.addImage(
			img.first,
			img.second->pixels.data(),
			img.second->width,
			img.second->height,
			true);
	}
}

Font* Theme::createFont(const std::string& name, const std::string& filename, u32 size)
{
	for (auto& fnt : fonts)
	{
		if (fnt->name == name
			&& fnt->filename == filename
			&& fnt->size == size)
		{
			fnt->usageCount++;

			return &fnt->font;
		}
	}

	FontVariation* newFont = new FontVariation();

	newFont->font.load(filename, size * ctx->scale);
	newFont->font.precacheLatinAlphabetGlyphs();
	newFont->size = size;
	newFont->usageCount = 1;
	newFont->filename = filename;
	newFont->name = name;
	fonts.push_back(newFont);

	return &newFont->font;
}

Font* Theme::createFontFromMemory(const std::string& name, const void* data, u32 dataSize, u32 size)
{
	for (auto& fnt : fonts)
	{
		if (fnt->name == name
			&& fnt->font.fontData == data
			&& fnt->size == size)
		{
			fnt->usageCount++;

			return &fnt->font;
		}
	}

	FontVariation* newFont = new FontVariation();

	newFont->font.loadFromMemory(data, dataSize, size * ctx->scale);
	newFont->font.precacheLatinAlphabetGlyphs();
	newFont->size = size;
	newFont->usageCount = 1;
	newFont->filename = "";
	newFont->name = name;
	fonts.push_back(newFont);

	return &newFont->font;
}

void Theme::deleteFont(Font* font)
{
	FontVariation* fontVar = nullptr;
	size_t index = 0;

	for (auto& fv : fonts)
	{
		if (&fv->font == font)
		{
			fontVar = fv;
			break;
		}

		++index;
	}

	if (!fontVar)
		return;

	--fontVar->usageCount;

	if (fontVar->usageCount <= 0)
	{
		delete fontVar;
		fonts.erase(fonts.begin() + index);
	}
}

void Theme::deleteFonts()
{
	for (auto& font : fonts)
	{
		delete font;
	}

	fonts.clear();
}

void Theme::deleteImages()
{
	for (auto& img : images)
	{
		delete img.second;
	}

	images.clear();
}

void Theme::rescaleFonts(f32 scale)
{
	for (auto& fontVar : fonts)
	{
		fontVar->font.resetFaceSize(fontVar->size * scale);
	}
}

void Theme::addFontGlyphsToAtlas()
{
	for (auto& fontVar : fonts)
	{
		for (auto& glyphPair : fontVar->font.glyphs)
		{
			auto fontGlyph = glyphPair.second;

			ImageId imgId = hashString(("__FONTGLYPH__" + std::to_string(fontGlyph->code) + std::string(fontVar->name)).c_str());
			
			// we check for buffer valid, since SPACE glyph might not have one
			if (fontGlyph->rgbaBuffer)
			{
				Image* glyphImage = (Image*)fontGlyph->image;

				if (!glyphImage)
				{
					glyphImage = new Image();
					glyphImage->id = imgId;
					glyphImage->width = fontGlyph->pixelWidth;
					glyphImage->height = fontGlyph->pixelHeight;
					fontGlyph->image = glyphImage;
				}
				
				// add to atlas for packing
				atlas.addImage(
					imgId,
					fontGlyph->rgbaBuffer,
					fontGlyph->pixelWidth,
					fontGlyph->pixelHeight, false);
			}
		}
	}
}

Font* Theme::getFont(const char* name)
{
	for (auto& fontVar : fonts)
	{
		if (fontVar->name == name)
			return &fontVar->font;
	}

	return nullptr;
}

Image* Theme::getImage(ImageId id)
{
	auto iter = images.find(id);

	if (iter == images.end())
		return nullptr;

	return images[id];
}

void Theme::build()
{
	atlas.create(atlasSize, atlasSize, 5);
	HUI_ASSERT(whiteImage);
	addImagesToAtlas();
	addFontGlyphsToAtlas();
	atlas.pack();

	for (auto& packedImage : atlas.images)
	{
		auto& img = packedImage.second;
		auto iterImages = images.find(img.id);

		if (iterImages != images.end())
		{
			iterImages->second->rect = img.rect;
			iterImages->second->uvRect = img.uvRect;
			iterImages->second->rotated = img.rotated;
		}
		else
		{
			//TODO: this isnt the fastest, but its ok on rebuilding theme
			for (auto& fontVar : fonts)
			{
				for (auto& glyphPair : fontVar->font.glyphs)
				{
					auto fontGlyph = glyphPair.second;
					auto fntImage = (Image*)fontGlyph->image;

					if (fntImage)
					{
						if (fntImage->id == img.id)
						{
							fntImage->rect = img.rect;
							fntImage->uvRect = img.uvRect;
							fntImage->rotated = img.rotated;
						}
					}
				}
			}
		}
	}

	setDefaultStyle();
}

}
