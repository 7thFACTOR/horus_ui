#include "util.h"
#include "3rdparty/utf8/source/utf8.h"
#include <algorithm>
#include <string.h>
#include <string>
#include <vector>

namespace hui
{
bool utf8ToUtf32(Utf8String text, UnicodeString& outText)
{
	if (!text)
		return false;

	outText.clear();

	try
	{
		utf8::utf8to32(
			(char*)text,
			(char*)(text + strlen(text)),
			std::back_inserter(outText));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	return true;
}

bool utf32ToUtf16(const UnicodeString& text, wchar_t** outString, size_t& length)
{
	*outString = new wchar_t[text.size()];

	for (int i = 0; i < text.size(); i++)
	{
		(*outString)[i] = text[i];
	}

	length = text.size();

	return true;
}

bool utf32ToUtf8(const UnicodeString& text, Utf8String* outString)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			text.begin(),
			text.end(),
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	*outString = new char[chars.size() + 1];
	memset((char*)*outString, 0, chars.size() + 1);

	auto str = (char*)*outString;

	for (int i = 0; i < chars.size(); i++)
	{
		str[i] = chars[i];
	}

	return true;
}

bool utf16ToUtf8(wchar_t* text, Utf8String* outString)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			text,
			text + wcslen(text),
			std::back_inserter(chars));
	}

	catch (utf8::invalid_utf8 ex)
	{
		return false;
	}

	*outString = new char[chars.size() + 1];
	memset((char*)*outString, 0, chars.size() + 1);

	auto str = (char*)*outString;

	for (int i = 0; i < chars.size(); i++)
	{
		str[i] = chars[i];
	}

	return true;
}

bool utf32ToUtf8NoAlloc(const UnicodeString& text, Utf8String outString, size_t maxLength)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			text.begin(),
			text.end(),
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

	for (i = 0; i < std::min(maxLength, chars.size()); i++)
	{
		((char*)outString)[i] = chars[i];
	}

	((char*)outString)[i] = 0;

	return true;
}



void toString(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount)
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

void toString(f32 value, char* outString, u32 outStringMaxSize, u32 decimalPlaces)
{
	if (decimalPlaces)
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

bool unicodeToUtf8(const u32* text, size_t textLength, Utf8StringBuffer outString, size_t maxOutStringLength)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			text,
			text + textLength,
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

	for (i = 0; i < std::min(maxOutStringLength, chars.size()); i++)
	{
		((char*)outString)[i] = chars[i];
	}

	((char*)outString)[i] = 0;

	return true;
}

}
