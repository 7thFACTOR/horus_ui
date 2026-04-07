#pragma once
#include "types.h"

namespace hui
{
void widgetAdd(f32 height);
WidgetId idGen(const char* text);
WidgetId idGen(u32 id);
WidgetId idGen(void* ptr);
WidgetId idFromPositionGen(const char* text);
void focusableSet();
void buttonBehavior(bool menuItem = false);
void mouseDownOnlyButtonBehavior();
bool viewportImageSizeFit(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& newWidth, f32& newHeight,
	bool ignoreHeight, bool ignoreWidth);
bool imageButtonInternal(HImage img, HImage disabledImg, f32 width, f32 height, bool down, ThemeElement* btnBodyElem);
bool clampValue(f32& value, f32 minVal, f32 maxVal);
inline f32 clampValue01(f32 value) { return fmax(0.0f, fmin(1.0f, value)); }

template <typename T> T sgn(T val) { return (T(0) < val) - (val < T(0)); }
void stringFromI32(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount = 0);
void stringFromF32(f32 value, char* outString, u32 outStringMaxSize, i32 decimalPlaces = ~0);
u64 hashString(const char* str, u64 seed = 0);
u64 hashData(const void* ptr, size_t size, u64 seed = 0);

}