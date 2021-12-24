#include "utfcpp_provider.h"
#include "utf8.h"
#include <algorithm>

namespace hui
{
bool UtfCppProvider::utf8To32(const char* utf8Str, Utf32String& outUtf32Str)
{
	if (!utf8Str)
		return false;

	outUtf32Str.clear();

	try
	{
		utf8::utf8to32(
			(char*)utf8Str,
			(char*)(utf8Str + strlen(utf8Str)),
			std::back_inserter(outUtf32Str));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	return true;
}

bool UtfCppProvider::utf32To16(const Utf32String& utf32Str, wchar_t** outUtf16Str, size_t& outUtf16StrLen)
{
	*outUtf16Str = new wchar_t[utf32Str.size()];

	for (size_t i = 0; i < utf32Str.size(); i++)
	{
		(*outUtf16Str)[i] = (wchar_t)utf32Str[i];
	}

	outUtf16StrLen = utf32Str.size();

	return true;
}

bool UtfCppProvider::utf32To8(const Utf32String& utf32Str, char** outUtf8Str)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			utf32Str.begin(),
			utf32Str.end(),
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	*outUtf8Str = new char[chars.size() + 1];
	memset((char*)*outUtf8Str, 0, chars.size() + 1);

	auto str = (char*)*outUtf8Str;

	for (int i = 0; i < chars.size(); i++)
	{
		str[i] = chars[i];
	}

	return true;
}

bool UtfCppProvider::utf16To8(const wchar_t* utf16Str, char** outUtf8Str)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			utf16Str,
			utf16Str + wcslen(utf16Str),
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	*outUtf8Str = new char[chars.size() + 1];
	memset((char*)*outUtf8Str, 0, chars.size() + 1);

	auto str = (char*)*outUtf8Str;

	for (int i = 0; i < chars.size(); i++)
	{
		str[i] = chars[i];
	}

	return true;
}

bool UtfCppProvider::utf32To8NoAlloc(const Utf32String& utf32Str, const char* outUtf8Str, size_t maxUtf8StrLen)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			utf32Str.begin(),
			utf32Str.end(),
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	if (chars.empty())
	{
		return false;
	}

	int i = 0;

	for (i = 0; i < std::min(maxUtf8StrLen, chars.size()); i++)
	{
		((char*)outUtf8Str)[i] = chars[i];
	}

	((char*)outUtf8Str)[i] = 0;

	return true;
}

size_t UtfCppProvider::utf8Length(const char* utf8Str)
{
	return utf8::distance(utf8Str, utf8Str + strlen(utf8Str));
}

bool UtfCppProvider::utf32To8NoAlloc(const u32* utf32Str, size_t utf32StrSize, const char* outUtf8Str, size_t maxOutUtf8StrSize)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			utf32Str,
			utf32Str + utf32StrSize,
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	if (chars.empty())
	{
		return false;
	}

	int i = 0;

	for (i = 0; i < std::min(maxOutUtf8StrSize, chars.size()); i++)
	{
		((char*)outUtf8Str)[i] = chars[i];
	}

	((char*)outUtf8Str)[i] = 0;

	return true;
}

}
