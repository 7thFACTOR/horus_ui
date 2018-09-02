#pragma once
#include "types.h"
#include "ui_font.h"

namespace hui
{
class FontCache
{
public:
	FontCache(UiAtlas* newAtlas);
	~FontCache();
	UiFont* createFont(const std::string& name, const std::string& filename, u32 size, bool packAtlasNow);
	void releaseFont(UiFont* font);
	void deleteFonts();
	void rescaleFonts(f32 scale);

protected:
	struct CachedFontInfo
	{
		UiFont font;
        std::string name;
        std::string filename;
		u32 size;
		u32 usageCount = 0;
	};

	UiAtlas* atlas = nullptr;
	std::map<UiFont*, CachedFontInfo*> cachedFonts;
};

}