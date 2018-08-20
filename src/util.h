#pragma once
#include "horus.h"
#include "types.h"

namespace hui
{
bool utf8ToUtf32(Utf8String text, UnicodeString& outText);
bool utf16ToUtf8(wchar_t* text, Utf8String* outString);
bool utf32ToUtf16(const UnicodeString& text, wchar_t** outString, size_t& length);
bool utf32ToUtf8(const UnicodeString& text, Utf8String* outString);
bool utf32ToUtf8NoAlloc(const UnicodeString& text, Utf8String outString, size_t maxLength);

void setEnabled(bool enabled);
void addWidgetItem(f32 height);
void setAsFocusable();
void buttonBehavior(bool menuItem = false);
void mouseDownOnlyButtonBehavior();
bool viewportImageFitSize(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& newWidth, f32& newHeight,
	bool ignoreHeight, bool ignoreWidth);
bool iconButtonInternal(Image icon, Image disabledIcon, f32 customHeight, bool down, UiThemeElement* btnBodyElem);

void saveImage(const char* filename, Rgba32* pixels, u32 width, u32 height);
}