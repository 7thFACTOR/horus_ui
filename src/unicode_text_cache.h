#pragma once
#include <unordered_map>
#include <string>
#include "types.h"

namespace hui
{
class UnicodeTextCache
{
public:
	UnicodeTextCache();
	~UnicodeTextCache();
	//TODO: warning for dynamic text that can change per frame, maybe that should not be cached
	// or just convert from utf8 to utf32 directly every frame, check speed
	Utf32String* getText(const char* text);
	void pruneUnusedText();

protected:
	struct CachedText
	{
		Utf32String* text = nullptr;
		f32 lastUsedTimeOrFrame = 0;
	};
	std::unordered_map<std::string, CachedText> texts;
};

}