#pragma once
#include <map>
#include <string>
#include <vector>
#include "types.h"

namespace hui
{
class TextCache
{
public:
	TextCache();
	~TextCache();
	//TODO: warning for dynamic text that can change per frame, maybe that should not be cached
	// or just convert from utf8 to utf32 directly everyframe, check speed
	UnicodeString* getText(Utf8String text);

protected:
	std::map<std::string, UnicodeString*> texts;
};

}