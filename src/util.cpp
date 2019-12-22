#include "util.h"
#include <algorithm>
#include <string.h>
#include <string>
#include <vector>
#include "libs/utfcpp/source/utf8.h"
#include "libs/stb/stb_image_write.h"

namespace hui
{
bool utf8ToUtf32(const char* text, UnicodeString& outText)
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

bool utf32ToUtf8(const UnicodeString& text, char** outString)
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

bool utf16ToUtf8(wchar_t* text, const char** outString)
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

bool utf32ToUtf8NoAlloc(const UnicodeString& text, const char* outString, size_t maxLength)
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

u32 utf8Len(const char* text)
{
	return utf8::distance(text, text + strlen(text));
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

bool unicodeToUtf8(const u32* text, u32 maxTextSize, char* outString, u32 maxOutStringSize)
{
	std::vector<char> chars;

	try
	{
		utf8::utf32to8(
			text,
			text + maxTextSize,
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

	for (i = 0; i < std::min(maxOutStringSize, (u32)chars.size()); i++)
	{
		((char*)outString)[i] = chars[i];
	}

	((char*)outString)[i] = 0;

	return true;
}

void saveImage(const char* filename, Rgba32* pixels, u32 width, u32 height)
{
	stbi_write_png(filename, width, height, 4, pixels, 0);
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

}
