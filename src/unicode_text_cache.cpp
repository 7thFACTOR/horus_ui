#include "unicode_text_cache.h"
#include "types.h"
#include <algorithm>
#include <string.h>
#include "context.h"
#include "horus_interfaces.h"

namespace hui
{
UnicodeTextCache::UnicodeTextCache()
{}

UnicodeTextCache::~UnicodeTextCache()
{
	for (auto txt : texts)
	{
		delete txt.second.text;
	}
}

Utf32String* UnicodeTextCache::getText(const char* text)
{
	auto iter = texts.find((const char*)text);

	if (iter == texts.end())
	{
		Utf32String* txt = new Utf32String();

		if (HORUS_UTF->utf8To32((char*)text, *txt))
		{
			CachedText ct;

			ct.text = txt;

			if (ctx->settings.textCachePruneMode == TextCachePruneMode::Time)
			{
				ct.lastUsedTimeOrFrame = ctx->totalTime;
			}
			else
			{
				ct.lastUsedTimeOrFrame = ctx->frameCount;
			}

			texts.insert(std::make_pair((const char*)text, ct));
			return txt;
		}
		else
		{
			delete txt;
			return false;
		}
	}

	if (ctx->settings.textCachePruneMode == TextCachePruneMode::Time)
	{
		iter->second.lastUsedTimeOrFrame = ctx->totalTime;
	}
	else
	{
		iter->second.lastUsedTimeOrFrame = ctx->frameCount;
	}

	return iter->second.text;
}

void UnicodeTextCache::pruneUnusedText()
{
	auto iter = texts.begin();

	while (iter != texts.end())
	{
		if (ctx->settings.textCachePruneMode == TextCachePruneMode::Time)
		{
			if (ctx->totalTime - iter->second.lastUsedTimeOrFrame >= ctx->settings.textCachePruneMaxTimeSec)
			{
				delete iter->second.text;
				iter = texts.erase(iter);
				continue;
			}
		}
		else
		{
			if (ctx->frameCount - iter->second.lastUsedTimeOrFrame >= ctx->settings.textCachePruneMaxFrames)
			{
				delete iter->second.text;
				iter = texts.erase(iter);
				continue;
			}
		}

		++iter;
	}
}

}
