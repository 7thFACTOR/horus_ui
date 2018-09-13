#include "unicode_text_cache.h"
#include "3rdparty/utf8/source/utf8.h"
#include "types.h"
#include <algorithm>
#include <string.h>
#include "ui_context.h"

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

UnicodeString* UnicodeTextCache::getText(const char* text)
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

void UnicodeTextCache::pruneUnusedTexts()
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
