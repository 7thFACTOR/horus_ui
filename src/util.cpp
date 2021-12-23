#include "util.h"
#include <algorithm>
#include <string.h>
#include <string>
#include <vector>

namespace hui
{
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
