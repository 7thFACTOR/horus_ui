#pragma once
#include "horus.h"
#include "types.h"

namespace hui
{
void setEnabled(bool enabled);
void addWidgetItem(f32 height);
void setFocusable();
void buttonBehavior(bool menuItem = false);
void mouseDownOnlyButtonBehavior();
bool viewportImageFitSize(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& newWidth, f32& newHeight,
	bool ignoreHeight, bool ignoreWidth);
bool iconButtonInternal(HImage icon, HImage disabledIcon, f32 customHeight, bool down, ThemeElement* btnBodyElem, bool focusable = true);
bool clampValue(f32& value, f32 minVal, f32 maxVal);
template <typename T> T sgn(T val) { return (T(0) < val) - (val < T(0)); }
}