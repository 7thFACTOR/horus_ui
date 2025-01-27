#pragma once
#include "types.h"

namespace hui
{
void addWidget(f32 height);
WidgetId genId(const char* text);
WidgetId genId(u32 id);
WidgetId genId(void* ptr);
WidgetId genIdFromPosition(const char* text);
void setFocusable();
void buttonBehavior(bool menuItem = false);
void mouseDownOnlyButtonBehavior();
bool viewportImageFitSize(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& newWidth, f32& newHeight,
	bool ignoreHeight, bool ignoreWidth);
bool iconButtonInternal(HImage icon, HImage disabledIcon, f32 customHeight, bool down, ThemeElement* btnBodyElem);
bool clampValue(f32& value, f32 minVal, f32 maxVal);
template <typename T> T sgn(T val) { return (T(0) < val) - (val < T(0)); }
u64 hashString(const char* str, u64 seed = 0);
u64 hashData(const void* ptr, size_t size, u64 seed = 0);

}