#pragma once
#include "types.h"

namespace hui
{
void addWidget(f32 height);
WidgetId genId(const char* text);
WidgetId genId(u32 id);
WidgetId genId(void* ptr);
WidgetId genIdFromPosition(const char* text);
void widgetSetFocusable();
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
u64 hashString(const char* str, u64 seed = 0);
u64 hashData(const void* ptr, size_t size, u64 seed = 0);

/// Find the spline segment closest to a point, sampling the actual curve so
/// bulging segments (e.g. Catmull-Rom, Natural) are hit correctly. Writes the
/// closest point on the curve to outClosestPoint and returns the segment index
/// i (between control points i and i+1), or -1 if there is no segment.
i32 splineClosestSegment(const SplineControlPoint* points, u32 count, SplineType type, const Point& query, Point* outClosestPoint, bool closed = false);

/// Sample a spline into an output buffer of 2d points. Returns number of points written.
/// If outPoints is null, returns how many points would be generated with given params.
u32 splineSample(const SplineControlPoint* points, u32 count, SplineType type, f32 segmentSize, bool closed, Point* outPoints, u32 maxPoints);

/// Compute the single control point of a quadratic bezier segment from the
/// outgoing tangent at its start and the incoming tangent at its end. The two
/// tangents are used as rays and their intersection is the control point; when
/// they are parallel the midpoint of the two handle endpoints is used instead.
Point splineQuadraticControl(const SplineControlPoint& start, const SplineControlPoint& end);

}