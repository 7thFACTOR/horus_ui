#include "utfcpp_provider.h"
#include "utfcpp.h"
#include "utf8.h"

namespace hui
{
bool utf8To32(const char* utf8Str, UnicodeString& outUtf32Str)
bool utf32To8(UnicodeString& outUtf32Str, char* outUtf8Str, size_t maxUtf8StrSize)
size_t utf8Length(const char* utf8Str)
}
