#include "font_cache.h"
#include "atlas.h"

namespace hui
{
FontCache::FontCache(UiAtlas* newAtlas)
{
	atlas = newAtlas;
}

FontCache::~FontCache()
{
	deleteFonts();
}

UiFont* FontCache::createFont(const std::string& name, const std::string& filename, u32 size, bool packAtlasNow)
{
	for (auto fnt : cachedFonts)
	{
		if (fnt.second->name == name
			&& fnt.second->filename == filename
			&& fnt.second->size == size)
		{
			fnt.second->usageCount++;
			return fnt.first;
		}
	}

	CachedFontInfo* newFont = new CachedFontInfo();

	newFont->font.load(filename, size, atlas);
	newFont->font.precacheLatinAlphabetGlyphs();
	newFont->size = size;
	newFont->usageCount = 1;
	newFont->filename = filename;
	newFont->name = name;
	cachedFonts.insert(std::make_pair(&newFont->font, newFont));

	if (packAtlasNow)
	{
		newFont->font.atlas->pack();
	}

	return &newFont->font;
}

void FontCache::releaseFont(UiFont* font)
{
	auto iter = cachedFonts.find(font);

	if (iter == cachedFonts.end())
		return;

	iter->second->usageCount--;

	if (!iter->second->usageCount)
	{
		delete iter->second;
		cachedFonts.erase(iter);
	}
}

void FontCache::deleteFonts()
{
	for (auto font : cachedFonts)
	{
		delete font.second;
	}

	cachedFonts.clear();
}

void FontCache::rescaleFonts(f32 scale)
{
	for (auto& font : cachedFonts)
	{
		font.second->font.resetFaceSize(font.second->size * scale);
	}
}

}
