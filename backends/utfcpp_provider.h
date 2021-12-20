#pragma once
#include "horus_interfaces.h"

namespace hui
{
struct UtfCppProvider : UtfProvider
{
	bool utf8To32(const char* utf8Str, UnicodeString& outUtf32Str) override;
	bool utf32To8(UnicodeString& outUtf32Str, char* outUtf8Str, size_t maxUtf8StrSize) override;
	size_t utf8Length(const char* utf8Str) override;
};

}