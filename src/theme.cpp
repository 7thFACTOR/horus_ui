#include "theme.h"

namespace hui
{
Theme::Theme(u32 atlasTextureSize)
{
	atlas = new Atlas(atlasTextureSize, atlasTextureSize);
}

Theme::~Theme()
{
	deleteImages();
	deleteFonts();
	delete atlas;
}

void Theme::setDefaultWidgetStyle()
{
	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		elements[i].setDefaultStyle();
	}

	for (auto& elem : userElements)
	{
		elem.second->setDefaultStyle();
	}
}

void Theme::addImagesToAtlas()
{
	atlas->addWhiteImage(32); // this is ok (with 4 doesnt work for example), we need a bigger white image since it will be trimmed by inset offsets etc.

	std::unordered_map<Image*, std::string> oldImages;

	for (auto& img : images)
	{
		oldImages[img.second.atlasImage] = img.first;

		auto image = atlas->addImage(
			img.second.imageData.pixels,
			img.second.imageData.width,
			img.second.imageData.height,
			false);
		img.second.atlasImage = image;
	}

	for (u32 i = 0; i < (int)WidgetElementId::Count; i++)
	{
		for (auto& style : elements[i].styles)
		{
			for(u32 j = 0; j < (u32)WidgetStateType::Count; j++)
			{
				style.second.states[j].image = images[oldImages[style.second.states[j].image]].atlasImage;
			}
		}
	}

	for (auto& userElem : userElements)
	{
		for (auto& style : userElem.second->styles)
		{
			for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
			{
				style.second.states[j].image = images[oldImages[style.second.states[j].image]].atlasImage;
			}
		}
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

	newFont->font.load(filename, size);
	newFont->font.precacheLatinAlphabetGlyphs();
	newFont->size = size;
	newFont->usageCount = 1;
	newFont->filename = filename;
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
		delete [] img.second.imageData.pixels;
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
			auto image = atlas->addImage(
				fontGlyph->rgbaBuffer,
				fontGlyph->pixelWidth,
				fontGlyph->pixelHeight);
			fontGlyph->image = image;
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

}
