#pragma once
#include "types.h"

namespace hui
{
/// Use the global line height unless the widget has its own explicit or derived height
void addWidget(f32 height, bool useLineHeight = true);
WidgetId genId(const char* text);
WidgetId genId(u32 id);
WidgetId genId(void* ptr);
WidgetId genIdFromPosition(const char* text);
void widgetSetFocusable();
void buttonBehavior(bool menuItem = false, bool allowFocus = true);
void mouseDownOnlyButtonBehavior();
bool imageButtonInternal(HImage img, HImage disabledImg, f32 width, f32 height, bool down, ThemeElement* btnBodyElem);
bool clampValue(f32& value, f32 minVal, f32 maxVal);

/// Convert a utf-8 byte offset into a utf-32 character index, without ever
/// splitting a codepoint. The offset is clamped to the end of the string.
i32 utf8ByteOffsetToCharIndex(const char* text, i32 byteOffset);

/// Draw the time cursor (playhead) as a flag in the ruler with a highlighted
/// vertical line below it. \param x the cursor x position in window pixels
/// \param rulerRect the time ruler the flag is drawn in
/// \param canvasBottom the bottom of the canvas the line ends at
/// \param color the line color
/// \param lineWidth the line width at 100% ui scale
void drawTimeCursor(f32 x, const Rect& rulerRect, f32 canvasBottom, const Color& color, f32 lineWidth);

/// Returns a shading factor for bright widget colors (channel, track and
/// segment colors) so they stay visible on the active theme, it is 1.0 for
/// dark themes and darker for light themes.
f32 themeColorContrastFactor();

inline f32 clampValue01(f32 value) { return fmax(0.0f, fmin(1.0f, value)); }

template <typename T> T sgn(T val) { return (T(0) < val) - (val < T(0)); }
u64 hashString(const char* str, u64 seed = 0);
u64 hashData(const void* ptr, size_t size, u64 seed = 0);

/// Find the spline segment closest to a point, sampling the actual curve so
/// bulging segments (e.g. Catmull-Rom, Natural) are hit correctly. Writes the
/// closest point on the curve to outClosestPoint, its parameter along the
/// segment to outT and returns the segment index i (between control points i
/// and i+1), or -1 if there is no segment.
i32 splineClosestSegment(const SplineControlPoint* points, u32 count, SplineType type, const Point& query, Point* outClosestPoint, bool closed = false, f32* outT = nullptr);

/// Sample a spline into an output buffer of 2d points. Returns number of points written.
/// If outPoints is null, returns how many points would be generated with given params.
u32 splineSample(const SplineControlPoint* points, u32 count, SplineType type, f32 segmentSize, bool closed, Point* outPoints, u32 maxPoints);

/// Sample a single spline segment (between control points segment and segment+1)
/// into outPoints, including both of its end points.
u32 splineSampleSegment(const SplineControlPoint* points, u32 count, SplineType type, i32 segment, f32 segmentSize, bool closed, Point* outPoints, u32 maxPoints);

/// Compute the single control point of a quadratic bezier segment from the
/// outgoing tangent at its start and the incoming tangent at its end. The two
/// tangents are used as rays and their intersection is the control point; when
/// they are parallel the midpoint of the two handle endpoints is used instead.
Point splineQuadraticControl(const SplineControlPoint& start, const SplineControlPoint& end);

}