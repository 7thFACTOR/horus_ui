#pragma once
#include "types.h"
#include "font.h"

namespace hui
{
struct FontCache
{
	FontCache();
	~FontCache();
	Font* createFont(const std::string& name, const std::string& filename, u32 size);
	void releaseFont(Font* font);
	void deleteFonts();
	void rescaleFonts(f32 scale);

protected:
	struct CachedFontInfo
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size = 0;
		u32 usageCount = 0;
	};

	std::unordered_map<Font*, CachedFontInfo*> cachedFonts;
};

}