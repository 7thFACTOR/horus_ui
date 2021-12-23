#pragma once
#include "horus_interfaces.h"

namespace hui
{
struct UtfCppProvider : UtfProvider
{
	bool utf8To32(const char* utf8Str, Utf32String& outUtf32Str) override;
	bool utf32To16(const Utf32String& utf32Str, wchar_t** outUtf16Str, size_t& outUtf16StrLen) override;
	bool utf32To8(const Utf32String& utf32Str, char** outUtf8Str) override;
	bool utf16To8(wchar_t* utf16Str, const char** outUtf8Str) override;
	bool utf32To8NoAlloc(const Utf32String& utf32Str, const char* outUtf8Str, size_t maxUtf8StrLen) override;
	bool utf32To8NoAlloc(const u32* utf32Str, size_t utf32StrSize, char* outUtf8Str, size_t maxOutUtf8StrSize) override;
	size_t utf8Length(const char* utf8Str) override;
};

}