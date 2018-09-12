#pragma once
#include <unordered_map>
#include <string>
#include "types.h"

namespace hui
{
class TextCache
{
public:
	TextCache();
	~TextCache();
	//TODO: warning for dynamic text that can change per frame, maybe that should not be cached
	// or just convert from utf8 to utf32 directly every frame, check speed
	UnicodeString* getText(const char* text);

protected:
	std::unordered_map<std::string, UnicodeString*> texts;
};

}