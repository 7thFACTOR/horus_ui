#include "text_cache.h"
#include "3rdparty/utf8/source/utf8.h"
#include "types.h"
#include <algorithm>
#include <string.h>

namespace hui
{
TextCache::TextCache()
{
}

TextCache::~TextCache()
{
	for (auto txt : texts)
	{
		delete txt.second;
	}
}

UnicodeString* TextCache::getText(Utf8String text)
{
	auto iter = texts.find((const char*)text);

	if (iter == texts.end())
	{
		UnicodeString* txt = new UnicodeString();
		
		try
		{
			utf8::utf8to32((char*)text, (char*)(text + strlen((const char*)text)), std::back_inserter(*txt));
		}

		catch (utf8::invalid_utf8 ex)
		{
			delete txt;
			return nullptr;
		}

		texts.insert(std::make_pair((const char*)text, txt));

		return txt;
	}

	return iter->second;
}

}
