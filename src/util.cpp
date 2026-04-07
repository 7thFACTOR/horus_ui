#include <algorithm>
#include <string.h>
#include <string>
#include <vector>
#include "util.h"
#include "rapidhash.h"
#include "context.h"

namespace hui
{
WidgetId idGen(const char* text)
{
	return hashString(text, ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId idGen(u32 id)
{
	return hashData(&id, sizeof(id), ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId idGen(void* ptr)
{
	return hashData(&ptr, sizeof(ptr), ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId idFromPositionGen(const char* text)
{
	auto posStr = std::to_string(ctx->position.x) + std::to_string(ctx->position.y);

	return idGen((std::string(text) + posStr).c_str());
}

void stringFromI32(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount)
{
	if (fillerZeroesCount)
	{
		std::string zeroCountStr;
		zeroCountStr = std::to_string(fillerZeroesCount);
		std::string strFmt = std::string("%.") + zeroCountStr;
		strFmt += "d";
		snprintf(outString, outStringMaxSize, strFmt.c_str(), value);
	}
	else
		snprintf(outString, outStringMaxSize, "%d", value);
}

void stringFromF32(f32 value, char* outString, u32 outStringMaxSize, i32 decimalPlaces)
{
	if (decimalPlaces >= 0)
	{
		std::string decimalsStr;
		decimalsStr = std::to_string(decimalPlaces);
		std::string strFmt = std::string("%.") + decimalsStr;
		strFmt += "f";
		snprintf(outString, outStringMaxSize, strFmt.c_str(), value);
	}
	else
		snprintf(outString, outStringMaxSize, "%f", value);
}

void toHexString(int n, char* outString, u32 outStringMaxSize, bool lowercase = true)
{
	if (lowercase)
		snprintf(outString, outStringMaxSize, "%02x", n);
	else
		snprintf(outString, outStringMaxSize, "%02X", n);
}

bool clampValue(f32& value, f32 minVal, f32 maxVal)
{
	if (value < minVal)
	{
		value = minVal;
		return true;
	}

	if (value > maxVal)
	{
		value = maxVal;
		return true;
	}

	return false;
}

u64 hashString(const char* str, u64 seed)
{
	return rapidhash_withSeed_unrolled(str, strlen(str), seed ? seed : RAPID_SEED);
}

u64 hashData(const void* ptr, size_t size, u64 seed)
{
	return rapidhash_withSeed_unrolled(ptr, size, seed ? seed : RAPID_SEED);
}

}
