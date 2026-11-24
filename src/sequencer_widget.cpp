#include <float.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
enum SequencerToolbarAction
{
	SequencerTbPlay,
	SequencerTbAddSegment,
	SequencerTbDelete,
	SequencerTbSnap,
	SequencerTbZoomOut,
	SequencerTbZoomIn,
	SequencerTbFit,
	SequencerTbCount
};

struct SequencerToolbarButton
{
	SequencerToolbarAction action;
	Rect rect;
	const char* label;
	bool active;
};

// a segment hit, edge is -1 for the start edge, 0 for the body, 1 for the end edge
struct SequencerSegmentHit
{
	i32 track = -1;
	i32 segment = -1;
	i32 edge = 0;
};

struct SequencerTrackScrollbar
{
	Rect track;
	Rect thumb;
	f32 maxScroll = 0.0f;
};

static f32 sequencerClamp(f32 value, f32 minValue, f32 maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}

static f32 sequencerSnapTime(const SequencerState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return sequencerClamp(time, state.startTime, state.endTime);
}

static f32 sequencerSnapTimeNoMax(const SequencerState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return std::max(time, state.startTime);
}

static f32 sequencerNiceStep(f32 rawStep)
{
	f32 magnitude = powf(10.0f, floorf(log10f(std::max(rawStep, 0.000001f))));
	f32 normalized = rawStep / magnitude;
	f32 nice;

	if (normalized < 1.5f)
		nice = 1.0f;
	else if (normalized < 3.5f)
		nice = 2.0f;
	else if (normalized < 7.5f)
		nice = 5.0f;
	else
		nice = 10.0f;

	return nice * magnitude;
}

static f32 sequencerTimeToX(const SequencerState& state, const Rect& canvasRect, f32 time)
{
	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);

	return canvasRect.x + (time - state.scrollTime) * pixelsPerSecond;
}

static f32 sequencerXToTime(const SequencerState& state, const Rect& canvasRect, f32 x)
{
	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);

	return state.scrollTime + (x - canvasRect.x) / pixelsPerSecond;
}

static Color sequencerSampleElementColor(WidgetElementId id, WidgetStateType stateType, bool useTextColor)
{
	auto& element = ctx->theme->getElement(id);
	auto& elementState = element.getState(stateType);
	Color tint = useTextColor ? elementState.textColor : elementState.color;
	Color result = tint;
	Image* image = (Image*)elementState.image;

	if (image && !image->pixels.empty())
	{
		u32 pixelCount = (u32)image->pixels.size();
		u32 sampleCount = std::min<u32>(pixelCount, 64);
		u32 step = std::max<u32>(1, pixelCount / sampleCount);
		f32 r = 0.0f;
		f32 g = 0.0f;
		f32 b = 0.0f;
		f32 a = 0.0f;
		u32 count = 0;

		for (u32 i = 0; i < pixelCount; i += step)
		{
			Color pixel(image->pixels[i]);

			if (pixel.a <= 0.001f)
				continue;

			r += pixel.r * pixel.a;
			g += pixel.g * pixel.a;
			b += pixel.b * pixel.a;
			a += pixel.a;
			count++;
		}

		if (count > 0 && a > 0.0001f)
		{
			Color average = { r / a, g / a, b / a, a / (f32)count };
			result = average * tint;
			result.a = tint.a;
		}
	}

	return result;
}

static Color sequencerShadeColor(const Color& color, f32 amount)
{
	return {
		sequencerClamp(color.r + amount, 0.0f, 1.0f),
		sequencerClamp(color.g + amount, 0.0f, 1.0f),
		sequencerClamp(color.b + amount, 0.0f, 1.0f),
		color.a };
}

static void sequencerApplyThemeColors(SequencerSettings& sequencerSettings)
{
	if (!ctx->theme)
		return;

	Color windowColor = sequencerSampleElementColor(WidgetElementId::WindowBody, WidgetStateType::Normal, false);
	Color textColor = sequencerSampleElementColor(WidgetElementId::LabelBody, WidgetStateType::Normal, true);
	Color accentColor = sequencerSampleElementColor(WidgetElementId::LinkBody, WidgetStateType::Normal, true);
	f32 luminance = windowColor.r * 0.299f + windowColor.g * 0.587f + windowColor.b * 0.114f;
	f32 rowShade = luminance < 0.5f ? 0.025f : -0.025f;

	sequencerSettings.backgroundColor = windowColor;
	sequencerSettings.trackListColor = sequencerShadeColor(windowColor, rowShade * 1.4f);
	sequencerSettings.headerColor = sequencerShadeColor(windowColor, rowShade * 2.0f);
	sequencerSettings.rowAlternateColor = sequencerShadeColor(windowColor, rowShade);
	sequencerSettings.rowSelectedColor = { accentColor.r, accentColor.g, accentColor.b, 0.5f };
	sequencerSettings.gridColor = { textColor.r, textColor.g, textColor.b, 0.12f };
	sequencerSettings.gridStrongColor = { textColor.r, textColor.g, textColor.b, 0.22f };
	sequencerSettings.textColor = textColor;
	sequencerSettings.dimTextColor = { textColor.r, textColor.g, textColor.b, 0.55f };
	sequencerSettings.playheadColor = Color::white;
	sequencerSettings.segmentOutlineColor = { textColor.r, textColor.g, textColor.b, 0.25f };
	sequencerSettings.toolbarToggleColor = sequencerShadeColor(
		sequencerSettings.toolbarToggleColor,
		luminance < 0.3f ? 0.08f : (luminance > 0.55f ? -0.1f : 0.0f));
	sequencerSettings.selectionFillColor = { accentColor.r, accentColor.g, accentColor.b, 0.14f };
	sequencerSettings.selectionBorderColor = { accentColor.r, accentColor.g, accentColor.b, 0.7f };
	sequencerSettings.endTimeLineColor = { textColor.r, textColor.g, textColor.b, 0.35f };
}

static Point sequencerTextSize(Font* font, const char* text)
{
	FontTextSize size = ctx->renderer.computeSizeOrDrawText(
		text ? text : "",
		Rect(0, 0, FLT_MAX, FLT_MAX),
		HAlignType::Left,
		VAlignType::Top,
		false,
		font);

	return { size.width, size.height };
}

static void sequencerDrawText(Font* font, const char* text, const Rect& rect, const Color& color, HAlignType align, bool ellipsis = true)
{
	if (!text || !text[0])
		return;

	ctx->renderer.cmdSetFont(font);
	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawTextInBox(text, rect, align, VAlignType::Center, ellipsis, true);
}

static void sequencerDrawButton(Font* font, const Rect& rect, const char* label, bool active, bool hovered, bool pressed, const Color& activeColor)
{
	auto& buttonElem = ctx->theme->getElement(WidgetElementId::ButtonBody);
	ThemeElement::State* state = &buttonElem.normalState();

	if (pressed || active)
	{
		auto& pressedState = buttonElem.getState(WidgetStateType::Pressed);
		Color backgroundColor = activeColor;

		if (pressed)
			backgroundColor = sequencerShadeColor(backgroundColor, -0.08f);
		else if (hovered)
			backgroundColor = sequencerShadeColor(backgroundColor, 0.06f);

		ctx->renderer.cmdSetColor(backgroundColor);
		ctx->renderer.cmdDrawImageBordered(pressedState.image, pressedState.border, rect, ctx->scale);
		sequencerDrawText(font, label, rect, Color::white, HAlignType::Center);
		return;
	}

	if (hovered)
		state = &buttonElem.getState(WidgetStateType::Hovered);

	ctx->renderer.cmdSetColor(state->color);
	ctx->renderer.cmdDrawImageBordered(state->image, state->border, rect, ctx->scale);
	sequencerDrawText(font, label, rect, state->textColor, HAlignType::Center);
}

static void sequencerDrawCheckbox(const Rect& rect, bool checked, bool disabled)
{
	auto& checkBodyElem = ctx->theme->getElement(WidgetElementId::CheckBody);
	auto& checkMarkElem = ctx->theme->getElement(WidgetElementId::CheckMark);
	auto& bodyState = disabled ? checkBodyElem.getState(WidgetStateType::Disabled) : checkBodyElem.normalState();

	ctx->renderer.cmdSetColor(bodyState.color);
	ctx->renderer.cmdDrawImageBordered(bodyState.image, bodyState.border, rect, ctx->scale);

	if (checked)
	{
		auto& markState = disabled ? checkMarkElem.getState(WidgetStateType::Disabled) : checkMarkElem.normalState();
		Rect markRect = rect.contract(2.0f * ctx->scale);

		ctx->renderer.cmdSetColor(markState.color);
		ctx->renderer.cmdDrawImageBordered(markState.image, markState.border, markRect, ctx->scale);
	}
}

static Rect sequencerTrackCheckboxRect(const Rect& rowRect, f32 scale)
{
	f32 size = 14.0f * scale;

	return { rowRect.x + 8.0f * scale, rowRect.y + (rowRect.height - size) * 0.5f, size, size };
}

static void sequencerSortSegments(SequencerTrack& track)
{
	std::sort(track.segments.begin(), track.segments.end(), [](const SequencerSegment& a, const SequencerSegment& b)
	{
		return a.start < b.start;
	});
}

static Color sequencerSegmentColor(const SequencerTrack& track, const SequencerSegment& segment)
{
	return segment.color.a > 0.0f ? segment.color : track.color;
}

static void sequencerClearSelection(SequencerState& state)
{
	for (auto& track : state.tracks)
	{
		for (auto& segment : track.segments)
			segment.selected = false;
	}
}

static bool sequencerIsDragged(
	const std::vector<SequencerSegmentDragItem>& dragItems,
	i32 trackIndex,
	i32 segmentIndex)
{
	for (const auto& item : dragItems)
	{
		if (item.track == trackIndex && item.segment == segmentIndex)
			return true;
	}

	return false;
}

static bool sequencerIsExcludedSnapSegment(
	const std::vector<SequencerSegmentDragItem>* excludeA,
	const std::vector<SequencerSegmentDragItem>* excludeB,
	i32 trackIndex,
	i32 segmentIndex)
{
	auto contains = [&](const std::vector<SequencerSegmentDragItem>* items)
	{
		if (!items)
			return false;

		for (const auto& item : *items)
		{
			if (item.track == trackIndex && item.segment == segmentIndex)
				return true;
		}

		return false;
	};

	return contains(excludeA) || contains(excludeB);
}

// returns the closest segment edge to the given time among all the tracks,
// -1 when nothing is within the snap distance, the excluded segments are the
// ones being dragged or resized
static f32 sequencerFindClosestSegmentEdge(
	const SequencerState& state,
	f32 time,
	f32 snapDistanceSeconds,
	const std::vector<SequencerSegmentDragItem>* excludeA,
	const std::vector<SequencerSegmentDragItem>* excludeB,
	i32 singleTrack,
	i32 singleSegment)
{
	f32 best = -1.0f;
	f32 bestDistance = snapDistanceSeconds;

	for (i32 trackIndex = 0; trackIndex < (i32)state.tracks.size(); trackIndex++)
	{
		const SequencerTrack& track = state.tracks[trackIndex];

		for (i32 segmentIndex = 0; segmentIndex < (i32)track.segments.size(); segmentIndex++)
		{
			if (trackIndex == singleTrack && segmentIndex == singleSegment)
				continue;

			if (sequencerIsExcludedSnapSegment(excludeA, excludeB, trackIndex, segmentIndex))
				continue;

			const SequencerSegment& segment = track.segments[segmentIndex];
			f32 edges[2] = { segment.start, segment.end };

			for (f32 edge : edges)
			{
				f32 distance = fabsf(edge - time);

				if (distance < bestDistance)
				{
					bestDistance = distance;
					best = edge;
				}
			}
		}
	}

	return best;
}

// collects the valid movement delta ranges of one dragged segment, from the
// free gaps between the non-dragged segments of its track, the deltas are
// relative to the captured drag start position
static void sequencerSegmentMoveRanges(
	const SequencerState& state,
	const std::vector<SequencerSegmentDragItem>& dragItems,
	i32 trackIndex,
	i32 segmentIndex,
	f32 itemStart,
	f32 itemEnd,
	std::vector<std::pair<f32, f32>>& outRanges)
{
	const SequencerTrack& track = state.tracks[trackIndex];
	f32 duration = itemEnd - itemStart;
	f32 gapStart = state.startTime;

	outRanges.clear();

	for (i32 i = 0; i < (i32)track.segments.size(); i++)
	{
		if (sequencerIsDragged(dragItems, trackIndex, i))
			continue;

		const SequencerSegment& segment = track.segments[i];

		// free gap [gapStart, segment.start)
		if (segment.start - gapStart >= duration)
			outRanges.push_back({ gapStart - itemStart, segment.start - itemEnd });

		gapStart = std::max(gapStart, segment.end);
	}

	// trailing gap, unbounded when moving past the end time is allowed to grow it
	if (state.extendEndTime)
		outRanges.push_back({ gapStart - itemStart, FLT_MAX });
	else if (state.endTime - gapStart >= duration)
		outRanges.push_back({ gapStart - itemStart, state.endTime - itemEnd });
}

// intersects two interval unions in place
static void sequencerIntersectRanges(
	std::vector<std::pair<f32, f32>>& ranges,
	const std::vector<std::pair<f32, f32>>& other)
{
	std::vector<std::pair<f32, f32>> result;

	for (auto& a : ranges)
	{
		for (auto& b : other)
		{
			f32 lo = std::max(a.first, b.first);
			f32 hi = std::min(a.second, b.second);

			if (lo <= hi)
				result.push_back({ lo, hi });
		}
	}

	ranges = result;
}

// clamps a delta to the nearest value allowed by the range union
static f32 sequencerClampDeltaToRanges(f32 delta, const std::vector<std::pair<f32, f32>>& ranges)
{
	if (ranges.empty())
		return 0.0f;

	f32 bestDelta = delta;
	f32 bestDistance = FLT_MAX;

	for (auto& range : ranges)
	{
		if (delta >= range.first && delta <= range.second)
			return delta;

		f32 candidate = fabsf(delta - range.first) <= fabsf(range.second - delta) ? range.first : range.second;
		f32 distance = fabsf(delta - candidate);

		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestDelta = candidate;
		}
	}

	return bestDelta;
}

// returns true when the delta is inside one of the allowed range
static bool sequencerDeltaInRanges(f32 delta, const std::vector<std::pair<f32, f32>>& ranges)
{
	for (auto& range : ranges)
	{
		if (delta >= range.first && delta <= range.second)
			return true;
	}

	return false;
}

// applies the push pass to one track: the dragged segments move by the delta
// and push the segments blocking them, either only computing the resulting
// first start and last end (apply false) or moving the segments (apply true)
static void sequencerPushTrack(
	SequencerTrack& track,
	const std::vector<SequencerSegmentDragItem>& dragItems,
	i32 trackIndex,
	f32 delta,
	bool apply,
	f32& outMinStart,
	f32& outMaxEnd)
{
	outMinStart = FLT_MAX;
	outMaxEnd = -FLT_MAX;

	if (delta == 0.0f)
		return;

	if (delta > 0.0f)
	{
		f32 cursor = -FLT_MAX;

		for (i32 i = 0; i < (i32)track.segments.size(); i++)
		{
			SequencerSegment& segment = track.segments[i];
			f32 duration = segment.end - segment.start;
			f32 newStart = segment.start + (sequencerIsDragged(dragItems, trackIndex, i) ? delta : 0.0f);

			// the segments the dragged ones reach are pushed forward
			if (newStart < cursor)
				newStart = cursor;

			f32 newEnd = newStart + duration;

			outMinStart = std::min(outMinStart, newStart);
			outMaxEnd = std::max(outMaxEnd, newEnd);

			if (apply)
			{
				segment.start = newStart;
				segment.end = newEnd;
			}

			cursor = newEnd;
		}
	}
	else
	{
		f32 cursor = FLT_MAX;

		for (i32 i = (i32)track.segments.size() - 1; i >= 0; i--)
		{
			SequencerSegment& segment = track.segments[i];
			f32 duration = segment.end - segment.start;
			f32 newEnd = segment.end + (sequencerIsDragged(dragItems, trackIndex, i) ? delta : 0.0f);

			// the segments the dragged ones reach are pushed backward
			if (newEnd > cursor)
				newEnd = cursor;

			f32 newStart = newEnd - duration;

			outMinStart = std::min(outMinStart, newStart);
			outMaxEnd = std::max(outMaxEnd, newEnd);

			if (apply)
			{
				segment.start = newStart;
				segment.end = newEnd;
			}

			cursor = newStart;
		}
	}
}

// returns how much the push delta makes the segments exceed the track time
// range, zero when everything fits
static f32 sequencerPushOverflow(
	SequencerState& state,
	const std::vector<SequencerSegmentDragItem>& dragItems,
	f32 delta)
{
	f32 overflow = 0.0f;

	if (delta == 0.0f)
		return 0.0f;

	for (i32 trackIndex = 0; trackIndex < (i32)state.tracks.size(); trackIndex++)
	{
		bool hasDragged = false;

		for (i32 i = 0; i < (i32)state.tracks[trackIndex].segments.size(); i++)
		{
			if (sequencerIsDragged(dragItems, trackIndex, i))
			{
				hasDragged = true;
				break;
			}
		}

		if (!hasDragged)
			continue;

		f32 minStart, maxEnd;

		sequencerPushTrack(state.tracks[trackIndex], dragItems, trackIndex, delta, false, minStart, maxEnd);

		if (maxEnd <= -FLT_MAX / 2.0f)
			continue;

		if (delta > 0.0f)
			overflow = std::max(overflow, maxEnd - state.endTime);
		else
			overflow = std::max(overflow, state.startTime - minStart);
	}

	return overflow;
}

// remaps the dragged segment indices after the tracks were sorted by matching
// the new start times
static void sequencerRemapDraggedSegments(
	std::vector<SequencerSegmentDragItem>& items,
	SequencerState& state,
	const std::vector<f32>& newStarts)
{
	for (size_t i = 0; i < items.size(); i++)
	{
		SequencerSegmentDragItem& item = items[i];

		if (newStarts[i] <= -FLT_MAX / 2.0f)
			continue;

		if (item.track < 0 || item.track >= (i32)state.tracks.size())
			continue;

		SequencerTrack& track = state.tracks[item.track];
		bool found = false;

		for (i32 j = 0; j < (i32)track.segments.size(); j++)
		{
			if (track.segments[j].start == newStarts[i])
			{
				item.segment = j;
				found = true;
				break;
			}
		}

		if (!found)
			item.segment = -1;
	}
}

// moves the dragged segments by the delta, pushing the segments they reach,
// used in push mode. The pass works on the current segment positions so only
// the part of the drag delta not applied yet is applied, pushAppliedDelta
// tracks that applied part. Returns true if anything moved
static bool sequencerApplyPushDelta(
	SequencerState& state,
	std::vector<SequencerSegmentDragItem>& dragItems,
	f32 rawDelta,
	f32 snapDistanceSeconds,
	f32& outSnapGuideTime,
	f32& pushAppliedDelta)
{
	outSnapGuideTime = -1.0f;

	if (dragItems.empty())
		return false;

	f32 delta = rawDelta - pushAppliedDelta;

	// snap the closest dragged edge to the closest other segment edge
	if (snapDistanceSeconds > 0.0f)
	{
		f32 bestAdjust = 0.0f;
		f32 bestTarget = -1.0f;
		f32 bestDistance = snapDistanceSeconds;

		for (auto& item : dragItems)
		{
			if (item.track < 0 || item.track >= (i32)state.tracks.size())
				continue;

			f32 edges[2] = { item.start + rawDelta, item.end + rawDelta };

			for (f32 edge : edges)
			{
				f32 target = sequencerFindClosestSegmentEdge(state, edge, snapDistanceSeconds, &dragItems, nullptr, -1, -1);

				if (target < 0.0f)
					continue;

				f32 distance = fabsf(target - edge);

				if (distance < bestDistance)
				{
					bestDistance = distance;
					bestAdjust = target - edge;
					bestTarget = target;
				}
			}
		}

		if (bestTarget >= 0.0f)
		{
			f32 candidate = delta + bestAdjust;

			// use the snapped delta when the pushed segments stay in range
			if (state.extendEndTime || sequencerPushOverflow(state, dragItems, candidate) <= 0.0001f)
			{
				delta = candidate;
				outSnapGuideTime = bestTarget;
			}
		}
	}

	// clamp the delta so the pushed segments stay in the track time range
	if (!state.extendEndTime && delta != 0.0f)
	{
		for (i32 iteration = 0; iteration < 4; iteration++)
		{
			f32 overflow = sequencerPushOverflow(state, dragItems, delta);

			if (overflow <= 0.0001f)
				break;

			delta = delta > 0.0f ? delta - overflow : delta + overflow;
		}
	}

	if (delta == 0.0f)
	{
		outSnapGuideTime = -1.0f;
		return false;
	}

	f32 pushedMaxEnd = -FLT_MAX;
	bool moved = false;

	for (i32 trackIndex = 0; trackIndex < (i32)state.tracks.size(); trackIndex++)
	{
		SequencerTrack& track = state.tracks[trackIndex];
		bool hasDragged = false;

		for (i32 i = 0; i < (i32)track.segments.size(); i++)
		{
			if (sequencerIsDragged(dragItems, trackIndex, i))
			{
				hasDragged = true;
				break;
			}
		}

		if (!hasDragged)
			continue;

		f32 minStart, maxEnd;

		sequencerPushTrack(track, dragItems, trackIndex, delta, true, minStart, maxEnd);
		pushedMaxEnd = std::max(pushedMaxEnd, maxEnd);
		moved = true;
	}

	if (!moved)
	{
		outSnapGuideTime = -1.0f;
		return false;
	}

	// remember the new starts before sorting to remap the dragged indices
	std::vector<f32> newStarts(dragItems.size(), -FLT_MAX);

	for (size_t i = 0; i < dragItems.size(); i++)
	{
		SequencerSegmentDragItem& item = dragItems[i];

		if (item.track < 0 || item.track >= (i32)state.tracks.size())
			continue;

		SequencerTrack& track = state.tracks[item.track];

		if (item.segment < 0 || item.segment >= (i32)track.segments.size())
			continue;

		newStarts[i] = track.segments[item.segment].start;
	}

	if (state.extendEndTime && pushedMaxEnd > state.endTime)
		state.endTime = pushedMaxEnd;

	pushAppliedDelta += delta;

	for (auto& track : state.tracks)
		sequencerSortSegments(track);

	sequencerRemapDraggedSegments(dragItems, state, newStarts);

	return true;
}

// moves all the given segments by the same delta, clamped so none of them
// leaves the track time range or overlaps a non-dragged segment, segments may
// cross non-dragged ones when there is free space for them, returns true if
// anything moved. With ripple items, the selection pushes them in the drag
// direction instead of crossing them. The dragged edges snap to the closest
// other segment edges within the snap distance, outSnapGuideTime receives the
// snapped time or -1. In push mode pushAppliedDelta tracks the applied part of
// the accumulated drag delta.
static bool sequencerApplyMoveDelta(
	SequencerState& state,
	std::vector<SequencerSegmentDragItem>& dragItems,
	std::vector<SequencerSegmentDragItem>& rippleItems,
	f32 rawDelta,
	f32 snapDistanceSeconds,
	f32& outSnapGuideTime,
	f32& pushAppliedDelta)
{
	bool moved = false;
	bool ripple = !rippleItems.empty();
	f32 clampedDelta = rawDelta;
	std::vector<std::pair<f32, f32>> allowedRanges;
	f32 rippleMinDelta = -FLT_MAX;
	f32 rippleMaxDelta = FLT_MAX;

	outSnapGuideTime = -1.0f;

	if (state.push)
		return sequencerApplyPushDelta(state, dragItems, rawDelta, snapDistanceSeconds, outSnapGuideTime, pushAppliedDelta);

	if (ripple)
	{
		// the moving chain depends on the drag direction: dragging right
		// pushes the right side segments, dragging left the left side ones
		f32 leftChainMinStart = FLT_MAX;
		f32 rightChainMaxEnd = -FLT_MAX;

		for (auto& item : dragItems)
		{
			leftChainMinStart = std::min(leftChainMinStart, item.start);
			rightChainMaxEnd = std::max(rightChainMaxEnd, item.end);
		}

		for (auto& item : rippleItems)
		{
			if (item.leftSide || item.bothSides)
				leftChainMinStart = std::min(leftChainMinStart, item.start);
			if (!item.leftSide || item.bothSides)
				rightChainMaxEnd = std::max(rightChainMaxEnd, item.end);
		}

		rippleMinDelta = state.startTime - leftChainMinStart;
		rippleMaxDelta = state.extendEndTime ? FLT_MAX : state.endTime - rightChainMaxEnd;

		clampedDelta = sequencerClamp(rawDelta, rippleMinDelta, rippleMaxDelta);
	}
	else
	{
		bool rangesInitialized = false;

		for (auto& item : dragItems)
		{
			if (item.track < 0 || item.track >= (i32)state.tracks.size())
				continue;

			const SequencerTrack& track = state.tracks[item.track];

			if (!track.enabled || track.locked || item.segment < 0 || item.segment >= (i32)track.segments.size())
				continue;

			std::vector<std::pair<f32, f32>> itemRanges;

			sequencerSegmentMoveRanges(state, dragItems, item.track, item.segment, item.start, item.end, itemRanges);

			if (!rangesInitialized)
			{
				allowedRanges = itemRanges;
				rangesInitialized = true;
			}
			else
			{
				sequencerIntersectRanges(allowedRanges, itemRanges);
			}
		}

		if (!rangesInitialized || allowedRanges.empty())
			return false;

		clampedDelta = sequencerClampDeltaToRanges(rawDelta, allowedRanges);
	}

	bool snapped = false;

	if (snapDistanceSeconds > 0.0f && !dragItems.empty())
	{
		// snap the closest dragged edge to the closest other segment edge
		f32 bestAdjust = 0.0f;
		f32 bestTarget = -1.0f;
		f32 bestDistance = snapDistanceSeconds;

		for (auto& item : dragItems)
		{
			if (item.track < 0 || item.track >= (i32)state.tracks.size())
				continue;

			const SequencerTrack& track = state.tracks[item.track];

			if (!track.enabled || track.locked || item.segment < 0 || item.segment >= (i32)track.segments.size())
				continue;

			f32 edges[2] = { item.start + clampedDelta, item.end + clampedDelta };

			for (f32 edge : edges)
			{
				f32 target = sequencerFindClosestSegmentEdge(state, edge, snapDistanceSeconds, &dragItems, &rippleItems, -1, -1);

				if (target < 0.0f)
					continue;

				f32 distance = fabsf(target - edge);

				if (distance < bestDistance)
				{
					bestDistance = distance;
					bestAdjust = target - edge;
					bestTarget = target;
				}
			}
		}

		if (bestTarget >= 0.0f)
		{
			f32 candidate = clampedDelta + bestAdjust;

			// only use the snapped delta when it is still a valid position
			if (ripple ? (candidate >= rippleMinDelta && candidate <= rippleMaxDelta) : sequencerDeltaInRanges(candidate, allowedRanges))
			{
				clampedDelta = candidate;
				snapped = true;
				outSnapGuideTime = bestTarget;
			}
		}
	}

	std::vector<f32> newStarts(dragItems.size(), -FLT_MAX);

	for (size_t i = 0; i < dragItems.size(); i++)
	{
		SequencerSegmentDragItem& item = dragItems[i];

		if (item.track < 0 || item.track >= (i32)state.tracks.size())
			continue;

		SequencerTrack& track = state.tracks[item.track];

		if (!track.enabled || track.locked || item.segment < 0 || item.segment >= (i32)track.segments.size())
			continue;

		SequencerSegment& segment = track.segments[item.segment];
		f32 duration = item.end - item.start;
		f32 desired = item.start + clampedDelta;
		f32 newStart;

		if (!ripple)
		{
			// find the free gap containing the desired position so the snapped
			// result never lands on a non-dragged segment
			f32 gapStart = state.startTime;
			f32 gapEnd = state.extendEndTime ? FLT_MAX : state.endTime;

			for (i32 j = 0; j < (i32)track.segments.size(); j++)
			{
				if (sequencerIsDragged(dragItems, item.track, j))
					continue;

				const SequencerSegment& other = track.segments[j];

				if (other.end <= desired)
					gapStart = std::max(gapStart, other.end);
				else if (other.start >= desired)
					gapEnd = std::min(gapEnd, other.start);
			}

			newStart = snapped
				? sequencerClamp(desired, gapStart, gapEnd - duration)
				: sequencerClamp(state.extendEndTime ? sequencerSnapTimeNoMax(state, desired) : sequencerSnapTime(state, desired), gapStart, gapEnd - duration);
		}
		else
		{
			newStart = snapped
				? sequencerClamp(desired, state.startTime, state.extendEndTime ? FLT_MAX : state.endTime - duration)
				: (state.extendEndTime
					? sequencerSnapTimeNoMax(state, desired)
					: std::min(sequencerSnapTime(state, desired), state.endTime - duration));
		}

		f32 newEnd = newStart + duration;

		newStarts[i] = newStart;

		if (newStart != segment.start || newEnd != segment.end)
		{
			segment.start = newStart;
			segment.end = newEnd;
			moved = true;
		}
	}

	std::vector<f32> rippleNewStarts(rippleItems.size(), -FLT_MAX);

	if (ripple)
	{
		for (size_t i = 0; i < rippleItems.size(); i++)
		{
			SequencerSegmentDragItem& item = rippleItems[i];

			if (item.track < 0 || item.track >= (i32)state.tracks.size())
				continue;

			SequencerTrack& track = state.tracks[item.track];

			if (!track.enabled || track.locked || item.segment < 0 || item.segment >= (i32)track.segments.size())
				continue;

			SequencerSegment& segment = track.segments[item.segment];
			rippleNewStarts[i] = item.start;

			f32 itemDelta = item.bothSides
				? clampedDelta
				: (item.leftSide ? std::min(clampedDelta, 0.0f) : std::max(clampedDelta, 0.0f));

			if (itemDelta == 0.0f)
				continue;

			f32 duration = item.end - item.start;
			f32 newStart = sequencerClamp(sequencerSnapTime(state, item.start + itemDelta), state.startTime, state.endTime - duration);
			f32 newEnd = newStart + duration;

			rippleNewStarts[i] = newStart;

			if (newStart != segment.start || newEnd != segment.end)
			{
				segment.start = newStart;
				segment.end = newEnd;
				moved = true;
			}
		}
	}

	if (moved)
	{
		for (auto& track : state.tracks)
			sequencerSortSegments(track);

		sequencerRemapDraggedSegments(dragItems, state, newStarts);
		sequencerRemapDraggedSegments(rippleItems, state, rippleNewStarts);
	}

	return moved;
}

static i32 sequencerDeleteSelectedSegments(SequencerState& state)
{
	i32 deletedCount = 0;

	for (auto& track : state.tracks)
	{
		for (size_t i = track.segments.size(); i > 0; i--)
		{
			if (track.segments[i - 1].selected)
			{
				track.segments.erase(track.segments.begin() + (i - 1));
				deletedCount++;
			}
		}
	}

	return deletedCount;
}

static bool sequencerAddSegment(
	SequencerState& state,
	i32 trackIndex,
	f32 start,
	f32 end,
	f32 minDuration,
	bool select)
{
	if (trackIndex < 0 || trackIndex >= (i32)state.tracks.size())
		return false;

	SequencerTrack& track = state.tracks[trackIndex];

	if (!track.enabled || track.locked || end - start < 0.001f)
		return false;

	// collect the free gaps that fit at least the minimum segment duration
	struct Gap
	{
		f32 start;
		f32 end;
	};

	std::vector<Gap> gaps;
	f32 gapStart = state.startTime;

	for (auto& segment : track.segments)
	{
		if (segment.start - gapStart >= minDuration)
			gaps.push_back({ gapStart, segment.start });

		gapStart = std::max(gapStart, segment.end);
	}

	if (state.endTime - gapStart >= minDuration)
		gaps.push_back({ gapStart, state.endTime });

	if (gaps.empty())
		return false;

	// prefer the gap containing the requested position, otherwise the first gap
	// after it, otherwise the last gap before it
	const Gap* chosen = nullptr;

	for (auto& gap : gaps)
	{
		if (start >= gap.start && start < gap.end)
		{
			chosen = &gap;
			break;
		}
	}

	if (!chosen)
	{
		for (auto& gap : gaps)
		{
			if (gap.start >= start)
			{
				chosen = &gap;
				break;
			}
		}
	}

	if (!chosen)
		chosen = &gaps.back();

	f32 duration = std::min(end - start, chosen->end - chosen->start);
	f32 newStart = sequencerClamp(start, chosen->start, chosen->end - duration);
	f32 newEnd = newStart + duration;

	if (select)
	{
		sequencerClearSelection(state);
		state.selectedTrack = trackIndex;
	}

	SequencerSegment segment;

	segment.start = newStart;
	segment.end = newEnd;
	segment.selected = select;

	track.segments.push_back(segment);
	sequencerSortSegments(track);

	return true;
}

static i32 sequencerTargetTrack(const SequencerState& state)
{
	if (state.selectedTrack >= 0
		&& state.selectedTrack < (i32)state.tracks.size()
		&& state.tracks[state.selectedTrack].enabled
		&& !state.tracks[state.selectedTrack].locked)
		return state.selectedTrack;

	for (i32 i = 0; i < (i32)state.tracks.size(); i++)
	{
		if (state.tracks[i].enabled && !state.tracks[i].locked)
			return i;
	}

	return -1;
}

static SequencerSegmentHit sequencerFindSegmentUnderMouse(
	const SequencerState& state,
	const SequencerSettings& settings,
	const Rect& canvasRect,
	f32 rowsTop,
	f32 rowHeight)
{
	SequencerSegmentHit result;
	Point mouse = ctx->mousePosition;

	if (!canvasRect.contains(mouse))
		return result;

	i32 trackIndex = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

	if (trackIndex < 0 || trackIndex >= (i32)state.tracks.size())
		return result;

	const SequencerTrack& track = state.tracks[trackIndex];

	if (!track.enabled || track.locked)
		return result;

	f32 edgeHitWidth = std::max(settings.segmentEdgeHitWidth * ctx->scale, 1.0f);
	f32 segmentHeight = settings.segmentHeight * ctx->scale;
	f32 rowY = rowsTop + (f32)trackIndex * rowHeight;

	// iterate in reverse so the topmost segment wins on overlaps
	for (i32 i = (i32)track.segments.size() - 1; i >= 0; i--)
	{
		const SequencerSegment& segment = track.segments[i];
		f32 x1 = sequencerTimeToX(state, canvasRect, segment.start);
		f32 x2 = sequencerTimeToX(state, canvasRect, segment.end);
		Rect segmentRect = { x1, rowY + (rowHeight - segmentHeight) * 0.5f, std::max(x2 - x1, 1.0f), segmentHeight };

		if (!segmentRect.contains(mouse))
			continue;

		result.track = trackIndex;
		result.segment = i;

		if (mouse.x - x1 <= edgeHitWidth)
			result.edge = -1;
		else if (x2 - mouse.x <= edgeHitWidth)
			result.edge = 1;
		else
			result.edge = 0;

		break;
	}

	return result;
}

static bool sequencerSegmentInsideRect(
	const SequencerState& state,
	const Rect& canvasRect,
	const Rect& selectionRect,
	f32 rowsTop,
	f32 rowHeight,
	f32 segmentHeight,
	i32 trackIndex,
	i32 segmentIndex)
{
	const SequencerTrack& track = state.tracks[trackIndex];
	const SequencerSegment& segment = track.segments[segmentIndex];
	f32 x1 = sequencerTimeToX(state, canvasRect, segment.start);
	f32 x2 = sequencerTimeToX(state, canvasRect, segment.end);
	f32 rowY = rowsTop + (f32)trackIndex * rowHeight;
	Rect segmentRect = { x1, rowY + (rowHeight - segmentHeight) * 0.5f, std::max(x2 - x1, 1.0f), segmentHeight };

	return !(segmentRect.right() < selectionRect.x
		|| segmentRect.x > selectionRect.right()
		|| segmentRect.bottom() < selectionRect.y
		|| segmentRect.y > selectionRect.bottom());
}

static void sequencerZoom(SequencerState& state, const SequencerSettings& settings, const Rect& canvasRect, f32 factor, f32 anchorX)
{
	f32 timeAtAnchor = sequencerXToTime(state, canvasRect, anchorX);

	state.pixelsPerSecond = sequencerClamp(state.pixelsPerSecond * factor, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);
	state.scrollTime = timeAtAnchor - (anchorX - canvasRect.x) / pixelsPerSecond;
}

static void sequencerFitView(SequencerState& state, const SequencerSettings& settings, const Rect& canvasRect)
{
	f32 timeRange = std::max(state.endTime - state.startTime, 0.001f);

	state.scrollTime = state.startTime;
	state.pixelsPerSecond = sequencerClamp(canvasRect.width / (timeRange * ctx->scale), settings.minPixelsPerSecond, settings.maxPixelsPerSecond);
}

static void sequencerClampView(SequencerState& state, const Rect& canvasRect, const SequencerSettings& settings)
{
	state.pixelsPerSecond = sequencerClamp(state.pixelsPerSecond, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);
	f32 visibleTime = canvasRect.width / pixelsPerSecond;
	f32 maxScrollTime = std::max(state.startTime, state.endTime - visibleTime);

	state.scrollTime = sequencerClamp(state.scrollTime, state.startTime, maxScrollTime);
	state.cursorTime = sequencerClamp(state.cursorTime, state.startTime, state.endTime);
}

static void sequencerClampTrackScroll(SequencerState& state, const Rect& listRect, const SequencerSettings& settings)
{
	f32 visibleHeight = listRect.height / ctx->scale;
	f32 totalHeight = (f32)state.tracks.size() * settings.rowHeight;
	f32 maxScroll = std::max(0.0f, totalHeight - visibleHeight);

	state.trackScroll = sequencerClamp(state.trackScroll, 0.0f, maxScroll);
}

static SequencerTrackScrollbar sequencerGetTrackScrollbar(
	const SequencerState& state,
	const SequencerSettings& settings,
	const Rect& listRect,
	f32 rowHeight)
{
	SequencerTrackScrollbar scrollbar;
	f32 contentHeight = (f32)state.tracks.size() * rowHeight;

	if (contentHeight <= listRect.height + 0.5f || listRect.height <= 0.0f)
		return scrollbar;

	f32 scale = ctx->scale;
	f32 trackWidth = settings.trackScrollbarWidth * scale;

	if (trackWidth <= 0.0f)
		return scrollbar;

	f32 maxScroll = std::max(0.0f, (contentHeight - listRect.height) / scale);
	f32 minThumbSize = std::min(settings.trackScrollbarMinThumbSize * scale, listRect.height);
	f32 thumbHeight = std::max(minThumbSize, listRect.height * (listRect.height / contentHeight));

	thumbHeight = std::min(thumbHeight, listRect.height);
	scrollbar.track = { listRect.right() - trackWidth, listRect.y, trackWidth, listRect.height };
	scrollbar.maxScroll = maxScroll;

	f32 travel = std::max(listRect.height - thumbHeight, 1.0f);
	f32 ratio = maxScroll > 0.0f ? sequencerClamp(state.trackScroll / maxScroll, 0.0f, 1.0f) : 0.0f;
	scrollbar.thumb = { scrollbar.track.x, listRect.y + travel * ratio, trackWidth, thumbHeight };

	return scrollbar;
}

static u32 sequencerBuildToolbar(
	Font* font,
	const Rect& toolbarRect,
	const SequencerState& state,
	SequencerToolbarButton* buttons)
{
	f32 scale = ctx->scale;
	f32 x = toolbarRect.x + 4.0f * scale;
	f32 y = toolbarRect.y + 3.0f * scale;
	f32 height = std::max(toolbarRect.height - 6.0f * scale, 1.0f);
	u32 count = 0;

	auto addButton = [&](SequencerToolbarAction action, const char* label, bool active, f32 gap)
	{
		x += gap * scale;
		Point textSize = sequencerTextSize(font, label);
		f32 width = textSize.x + 16.0f * scale;

		buttons[count].action = action;
		buttons[count].rect = { x, y, width, height };
		buttons[count].label = label;
		buttons[count].active = active;
		count++;
		x += width + 2.0f * scale;
	};

	addButton(SequencerTbPlay, state.playing ? "Pause" : "Play", state.playing, 0.0f);
	addButton(SequencerTbAddSegment, "+ Seg", false, 6.0f);
	addButton(SequencerTbDelete, "Del", false, 3.0f);
	addButton(SequencerTbSnap, "Snap", state.snapToFrames, 10.0f);
	addButton(SequencerTbZoomOut, "-", false, 10.0f);
	addButton(SequencerTbZoomIn, "+", false, 2.0f);
	addButton(SequencerTbFit, "Fit", false, 6.0f);

	return count;
}

bool sequencer(const char* id, SequencerState* state, f32 height)
{
	if (!state)
		return false;

	Settings& settings = ctx->settings;
	SequencerSettings activeSequencerSettings = settings.sequencer;

	if (activeSequencerSettings.useThemeColors)
		sequencerApplyThemeColors(activeSequencerSettings);

	const SequencerSettings& sequencerSettings = activeSequencerSettings;
	f32 scale = ctx->scale;

	if (height <= 0.0f)
	{
		height = (ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y)) / scale;

		if (height <= 0.0f)
			height = sequencerSettings.defaultHeight;
	}

	ctx->id = genId(id);
	WidgetId sequencerId = ctx->id;

	addWidget(height, false);
	widgetSetFocusable();

	ctx->widget.visible = true;

	Rect outerRect = ctx->widget.rect;
	bool disabled = ctx->widget.disabled;
	bool changed = false;

	if (state->endTime <= state->startTime)
		state->endTime = state->startTime + 1.0f;

	if (state->pixelsPerSecond <= 0.0f)
		state->pixelsPerSecond = sequencerSettings.defaultPixelsPerSecond;

	state->pixelsPerSecond = sequencerClamp(state->pixelsPerSecond, sequencerSettings.minPixelsPerSecond, sequencerSettings.maxPixelsPerSecond);

	if (state->frameRate <= 0.0f)
		state->frameRate = 30.0f;

	if (state->selectedTrack < -1 || state->selectedTrack >= (i32)state->tracks.size())
		state->selectedTrack = -1;

	state->cursorTime = sequencerClamp(state->cursorTime, state->startTime, state->endTime);

	for (auto& track : state->tracks)
	{
		for (auto& segment : track.segments)
		{
			if (segment.end < segment.start)
				std::swap(segment.start, segment.end);
		}

		sequencerSortSegments(track);
	}

	if (!disabled && state->playing)
	{
		state->cursorTime += ctx->settings.deltaTime;

		if (state->cursorTime >= state->endTime)
		{
			if (state->loopPlayback)
			{
				f32 timeRange = std::max(state->endTime - state->startTime, 0.001f);
				state->cursorTime = state->startTime + fmodf(state->cursorTime - state->endTime, timeRange);
			}
			else
			{
				state->cursorTime = state->endTime;
				state->playing = false;
			}
		}

		changed = true;
		forceRepaint();
	}

	auto& runtime = ctx->sequencerStates[sequencerId];
	f32 toolbarHeight = sequencerSettings.showToolbar ? sequencerSettings.toolbarHeight * scale : 0.0f;
	f32 rulerHeight = sequencerSettings.rulerHeight * scale;
	f32 rowHeight = sequencerSettings.rowHeight * scale;

	if (runtime.trackListWidth <= 0.0f)
		runtime.trackListWidth = sequencerSettings.trackListWidth;

	f32 minTrackWidth = std::min(80.0f * scale, outerRect.width * 0.5f);
	f32 maxTrackWidth = std::max(minTrackWidth, outerRect.width - std::min(120.0f * scale, outerRect.width * 0.5f));
	f32 trackWidth = sequencerClamp(runtime.trackListWidth * scale, minTrackWidth, maxTrackWidth);

	trackWidth = std::max(trackWidth, std::min(40.0f * scale, outerRect.width));

	Rect toolbarRect = { outerRect.x, outerRect.y, outerRect.width, toolbarHeight };
	Rect listHeaderRect = { outerRect.x, outerRect.y + toolbarHeight, trackWidth, rulerHeight };
	Rect listRect = { outerRect.x, outerRect.y + toolbarHeight + rulerHeight, trackWidth, outerRect.height - toolbarHeight - rulerHeight };
	Rect rulerRect = { outerRect.x + trackWidth, outerRect.y + toolbarHeight, outerRect.width - trackWidth, rulerHeight };
	Rect canvasRect = { outerRect.x + trackWidth, outerRect.y + toolbarHeight + rulerHeight, outerRect.width - trackWidth, outerRect.height - toolbarHeight - rulerHeight };

	listRect.height = std::max(listRect.height, 0.0f);
	rulerRect.width = std::max(rulerRect.width, 0.0f);
	canvasRect.width = std::max(canvasRect.width, 0.0f);
	canvasRect.height = std::max(canvasRect.height, 0.0f);

	Rect trackSplitterRect = {
		outerRect.x + trackWidth - 3.0f * scale,
		toolbarRect.bottom(),
		6.0f * scale,
		outerRect.bottom() - toolbarRect.bottom() };

	sequencerClampTrackScroll(*state, listRect, sequencerSettings);
	sequencerClampView(*state, canvasRect, sequencerSettings);

	f32 rowsTop = canvasRect.y - state->trackScroll * scale;

	Font* font = ctx->theme->getFont("normal");

	if (!font && !ctx->theme->fonts.empty())
		font = &ctx->theme->fonts[0]->font;

	SequencerToolbarButton toolbarButtons[SequencerTbCount];
	u32 toolbarButtonCount = sequencerSettings.showToolbar
		? sequencerBuildToolbar(font, toolbarRect, *state, toolbarButtons)
		: 0;

	Point mouse = ctx->mousePosition;
	bool mouseInSequencer = outerRect.contains(mouse) && ctx->hoveringThisWindow && ctx->isActiveLayer();
	InputEvent event = ctx->event;
	bool consumed = false;

	if (mouseInSequencer && !ctx->widget.captureId)
	{
		ctx->widget.hoveredId = sequencerId;
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = outerRect;
	}

	i32 hoveredToolbarButton = -1;

	if (mouseInSequencer && (!ctx->widget.captureId || runtime.pressedToolbarButton >= 0))
	{
		for (u32 i = 0; i < toolbarButtonCount; i++)
		{
			if (toolbarButtons[i].rect.contains(mouse))
			{
				hoveredToolbarButton = (i32)i;
				break;
			}
		}
	}

	auto focusSequencer = [&]()
	{
		ctx->widget.focusedId = sequencerId;
		ctx->widget.focused = true;
		ctx->focusChanged = true;
	};

	auto beginCapture = [&]()
	{
		focusSequencer();
		ctx->widget.captureId = sequencerId;
		windowSetCapture();
	};

	auto endCapture = [&]()
	{
		ctx->widget.captureId = 0;
		windowReleaseCapture();
	};

	auto applyToolbarAction = [&](SequencerToolbarAction action)
	{
		SequencerAction publicAction = SequencerAction::Play;
		bool mapped = true;

		switch (action)
		{
		case SequencerTbPlay: publicAction = SequencerAction::Play; break;
		case SequencerTbAddSegment: publicAction = SequencerAction::AddSegment; break;
		case SequencerTbDelete: publicAction = SequencerAction::DeleteSelected; break;
		case SequencerTbSnap: publicAction = SequencerAction::ToggleSnap; break;
		case SequencerTbZoomOut: publicAction = SequencerAction::ZoomOut; break;
		case SequencerTbZoomIn: publicAction = SequencerAction::ZoomIn; break;
		case SequencerTbFit: publicAction = SequencerAction::FitView; break;
		default: mapped = false; break;
		}

		if (mapped && sequencerAction(state, publicAction, canvasRect.width))
			changed = true;
	};

	SequencerTrackScrollbar trackScrollbar = sequencerGetTrackScrollbar(*state, sequencerSettings, listRect, rowHeight);

	if (!disabled)
	{
		if (event.type == InputEvent::Type::MouseDown
			&& event.mouse.button == MouseButton::Left
			&& hoveredToolbarButton >= 0)
		{
			runtime.pressedToolbarButton = hoveredToolbarButton;
			beginCapture();
			consumed = true;
		}
		else if (event.type == InputEvent::Type::MouseUp
			&& event.mouse.button == MouseButton::Left
			&& runtime.pressedToolbarButton >= 0)
		{
			i32 pressedButton = runtime.pressedToolbarButton;

			runtime.pressedToolbarButton = -1;
			endCapture();

			if (pressedButton == hoveredToolbarButton && pressedButton < (i32)toolbarButtonCount)
				applyToolbarAction(toolbarButtons[pressedButton].action);

			consumed = true;
		}

		if (!consumed && mouseInSequencer && event.type == InputEvent::Type::MouseWheel)
		{
			if (canvasRect.contains(mouse) || rulerRect.contains(mouse))
			{
				f32 factor = event.mouse.wheel.y > 0.0f ? sequencerSettings.zoomStep : 1.0f / sequencerSettings.zoomStep;

				sequencerZoom(*state, sequencerSettings, canvasRect, factor, mouse.x);
				sequencerClampView(*state, canvasRect, sequencerSettings);
			}
			else if (listRect.contains(mouse))
			{
				state->trackScroll -= event.mouse.wheel.y * sequencerSettings.rowHeight * 0.75f;
				sequencerClampTrackScroll(*state, listRect, sequencerSettings);
			}

			consumed = true;
		}

		if (!consumed && mouseInSequencer && event.type == InputEvent::Type::MouseDown)
		{
			if (event.mouse.button == MouseButton::Middle
				&& (canvasRect.contains(mouse) || rulerRect.contains(mouse)))
			{
				runtime.dragType = SequencerWidgetState::DragType::Pan;
				runtime.dragStartMouse = mouse;
				runtime.panStartScrollTime = state->scrollTime;
				beginCapture();
				consumed = true;
			}
			else if (event.mouse.button == MouseButton::Left)
			{
				if (trackSplitterRect.contains(mouse))
				{
					runtime.dragType = SequencerWidgetState::DragType::TrackResize;
					beginCapture();
					consumed = true;
				}
				else if (trackScrollbar.track.contains(mouse))
				{
					if (trackScrollbar.maxScroll > 0.0f)
					{
						if (!trackScrollbar.thumb.contains(mouse))
						{
							f32 travel = std::max(trackScrollbar.track.height - trackScrollbar.thumb.height, 1.0f);
							f32 ratio = sequencerClamp(
								(mouse.y - trackScrollbar.track.y - trackScrollbar.thumb.height * 0.5f) / travel,
								0.0f,
								1.0f);

							state->trackScroll = ratio * trackScrollbar.maxScroll;
						}

						runtime.dragType = SequencerWidgetState::DragType::TrackScroll;
						runtime.dragStartMouse = mouse;
						runtime.dragStartTrackScroll = state->trackScroll;
						beginCapture();
						consumed = true;
					}
				}
				else if (listRect.contains(mouse))
				{
					f32 scrollOffset = state->trackScroll * scale;
					f32 localY = mouse.y - listRect.y + scrollOffset;
					i32 trackIndex = (i32)floorf(localY / std::max(rowHeight, 1.0f));

					if (trackIndex >= 0 && trackIndex < (i32)state->tracks.size())
					{
						Rect rowRect = { listRect.x, listRect.y - scrollOffset + (f32)trackIndex * rowHeight, listRect.width, rowHeight };

						if (rowRect.contains(mouse))
						{
							SequencerTrack& track = state->tracks[trackIndex];

							if (sequencerTrackCheckboxRect(rowRect, scale).contains(mouse))
							{
								track.enabled = !track.enabled;
								changed = true;
							}
							else
							{
								state->selectedTrack = trackIndex;
								sequencerClearSelection(*state);
								changed = true;
							}

							focusSequencer();
							consumed = true;
						}
					}
				}
				else if (rulerRect.contains(mouse))
				{
					state->cursorTime = sequencerSnapTime(*state, sequencerXToTime(*state, canvasRect, mouse.x));
					runtime.dragType = SequencerWidgetState::DragType::Playhead;
					beginCapture();
					changed = true;
					consumed = true;
				}
				else if (canvasRect.contains(mouse))
				{
					SequencerSegmentHit hit = sequencerFindSegmentUnderMouse(*state, sequencerSettings, canvasRect, rowsTop, rowHeight);

					if (hit.track >= 0)
					{
						SequencerTrack& track = state->tracks[hit.track];
						SequencerSegment& segment = track.segments[hit.segment];
						bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

						if (controlDown)
						{
							segment.selected = !segment.selected;
							state->selectedTrack = hit.track;
							changed = true;
						}
						else if (!segment.selected)
						{
							sequencerClearSelection(*state);
							segment.selected = true;
							state->selectedTrack = hit.track;
							changed = true;
						}
						else
						{
							state->selectedTrack = hit.track;
						}

						if (segment.selected && track.enabled && !track.locked)
						{
							if (hit.edge == 0)
							{
								runtime.dragType = SequencerWidgetState::DragType::Segments;
								runtime.dragTrack = hit.track;
								runtime.dragSegment = hit.segment;
								runtime.dragStartMouse = mouse;
								runtime.dragStartTime = sequencerXToTime(*state, canvasRect, mouse.x);
								runtime.dragItems.clear();
								runtime.rippleItems.clear();
								runtime.snapGuideTime = -1.0f;
								runtime.pushAppliedDelta = 0.0f;

								for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
								{
									SequencerTrack& selectedTrack = state->tracks[trackIndex];

									if (!selectedTrack.enabled || selectedTrack.locked)
										continue;

									for (i32 segmentIndex = 0; segmentIndex < (i32)selectedTrack.segments.size(); segmentIndex++)
									{
										if (!selectedTrack.segments[segmentIndex].selected)
											continue;

										runtime.dragItems.push_back({
											trackIndex,
											segmentIndex,
											selectedTrack.segments[segmentIndex].start,
											selectedTrack.segments[segmentIndex].end });
									}
								}

								// in ripple mode the unselected segments follow the drag
								if (state->ripple)
								{
									for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
									{
										SequencerTrack& rippleTrack = state->tracks[trackIndex];

										if (!rippleTrack.enabled || rippleTrack.locked)
											continue;

										// the selected time range on this track
										f32 minSelectedStart = FLT_MAX;
										f32 maxSelectedEnd = -FLT_MAX;
										bool hasSelection = false;

										for (auto& selectedSegment : rippleTrack.segments)
										{
											if (!selectedSegment.selected)
												continue;

											minSelectedStart = std::min(minSelectedStart, selectedSegment.start);
											maxSelectedEnd = std::max(maxSelectedEnd, selectedSegment.end);
											hasSelection = true;
										}

										if (!hasSelection)
											continue;

										for (i32 segmentIndex = 0; segmentIndex < (i32)rippleTrack.segments.size(); segmentIndex++)
										{
											const SequencerSegment& rippleSegment = rippleTrack.segments[segmentIndex];

											if (rippleSegment.selected)
												continue;

											SequencerSegmentDragItem item;

											item.track = trackIndex;
											item.segment = segmentIndex;
											item.start = rippleSegment.start;
											item.end = rippleSegment.end;

											if (rippleSegment.end <= minSelectedStart)
												item.leftSide = true;
											else if (rippleSegment.start >= maxSelectedEnd)
												item.leftSide = false;
											else
												item.bothSides = true;

											runtime.rippleItems.push_back(item);
										}
									}
								}
							}
							else
							{
								runtime.dragType = hit.edge < 0
									? SequencerWidgetState::DragType::SegmentResizeLeft
									: SequencerWidgetState::DragType::SegmentResizeRight;
								runtime.dragTrack = hit.track;
								runtime.dragSegment = hit.segment;
								runtime.dragStartMouse = mouse;
								runtime.dragStartTime = sequencerXToTime(*state, canvasRect, mouse.x);
								runtime.snapGuideTime = -1.0f;
								runtime.pushAppliedDelta = 0.0f;
							}

							beginCapture();
						}

						consumed = true;
					}
					else if (event.mouse.clickCount == 2)
					{
						// double click on an empty lane adds a segment at the double-clicked position
						i32 trackIndex = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

						if (trackIndex >= 0 && trackIndex < (i32)state->tracks.size()
							&& state->tracks[trackIndex].enabled
							&& !state->tracks[trackIndex].locked)
						{
							f32 addStart = sequencerSnapTime(*state, sequencerXToTime(*state, canvasRect, mouse.x));
							f32 addEnd = std::min(addStart + 1.0f, state->endTime);

							if (addEnd - addStart < sequencerSettings.minSegmentDuration)
								addStart = std::max(state->startTime, state->endTime - sequencerSettings.minSegmentDuration);

							if (sequencerAddSegment(*state, trackIndex, addStart, addEnd, sequencerSettings.minSegmentDuration, true))
								changed = true;
						}

						consumed = true;
					}
					else
					{
						f32 playheadX = sequencerTimeToX(*state, canvasRect, state->cursorTime);

						if (fabsf(mouse.x - playheadX) <= 5.0f * scale)
						{
							runtime.dragType = SequencerWidgetState::DragType::Playhead;
							beginCapture();
							changed = true;
						}
						else
						{
							bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

							if (!controlDown)
							{
								sequencerClearSelection(*state);
								changed = true;
							}

							runtime.dragType = SequencerWidgetState::DragType::BoxSelect;
							runtime.boxStart = mouse;
							runtime.boxEnd = mouse;
							beginCapture();
						}

						consumed = true;
					}
				}
			}
		}

		if (event.type == InputEvent::Type::MouseMove && runtime.dragType != SequencerWidgetState::DragType::None)
		{
			switch (runtime.dragType)
			{
			case SequencerWidgetState::DragType::Playhead:
				state->cursorTime = sequencerSnapTime(*state, sequencerXToTime(*state, canvasRect, mouse.x));
				changed = true;
				consumed = true;
				break;

			case SequencerWidgetState::DragType::Pan:
				state->scrollTime = runtime.panStartScrollTime - (mouse.x - runtime.dragStartMouse.x) / (state->pixelsPerSecond * scale);
				sequencerClampView(*state, canvasRect, sequencerSettings);
				changed = true;
				consumed = true;
				break;

			case SequencerWidgetState::DragType::TrackResize:
			{
				f32 minWidth = 80.0f;
				f32 maxWidth = std::max(minWidth, outerRect.width / scale - std::min(120.0f, outerRect.width / scale * 0.5f));

				runtime.trackListWidth = sequencerClamp((mouse.x - outerRect.x) / scale, minWidth, maxWidth);
				consumed = true;
				break;
			}

			case SequencerWidgetState::DragType::TrackScroll:
			{
				SequencerTrackScrollbar scrollbar = sequencerGetTrackScrollbar(*state, sequencerSettings, listRect, rowHeight);
				f32 travel = std::max(scrollbar.track.height - scrollbar.thumb.height, 1.0f);

				if (scrollbar.maxScroll > 0.0f)
					state->trackScroll = sequencerClamp(
						runtime.dragStartTrackScroll + (mouse.y - runtime.dragStartMouse.y) / travel * scrollbar.maxScroll,
						0.0f,
						scrollbar.maxScroll);

				consumed = true;
				break;
			}

			case SequencerWidgetState::DragType::Segments:
			{
				f32 mouseTime = sequencerXToTime(*state, canvasRect, mouse.x);
				f32 rawDelta = mouseTime - runtime.dragStartTime;
				f32 snapSeconds = state->snapToSegments
					? sequencerSettings.snapDistance / std::max(state->pixelsPerSecond, 0.0001f)
					: 0.0f;

				if (rawDelta != 0.0f && !runtime.dragItems.empty())
				{
					if (sequencerApplyMoveDelta(*state, runtime.dragItems, runtime.rippleItems, rawDelta, snapSeconds, runtime.snapGuideTime, runtime.pushAppliedDelta))
						changed = true;
				}

				consumed = true;
				break;
			}

			case SequencerWidgetState::DragType::SegmentResizeLeft:
			case SequencerWidgetState::DragType::SegmentResizeRight:
			{
				if (runtime.dragTrack < 0 || runtime.dragTrack >= (i32)state->tracks.size())
					break;

				SequencerTrack& track = state->tracks[runtime.dragTrack];

				if (runtime.dragSegment < 0 || runtime.dragSegment >= (i32)track.segments.size())
					break;

				SequencerSegment& segment = track.segments[runtime.dragSegment];
				f32 mouseTime = sequencerXToTime(*state, canvasRect, mouse.x);
				f32 minDuration = sequencerSettings.minSegmentDuration;
				f32 snapSeconds = state->snapToSegments
					? sequencerSettings.snapDistance / std::max(state->pixelsPerSecond, 0.0001f)
					: 0.0f;

				// snap the resized edge to the closest other segment edge
				f32 snapTarget = snapSeconds > 0.0f
					? sequencerFindClosestSegmentEdge(*state, mouseTime, snapSeconds, nullptr, nullptr, runtime.dragTrack, runtime.dragSegment)
					: -1.0f;
				bool snapped = snapTarget >= 0.0f;
				f32 edgeTime = snapped ? snapTarget : mouseTime;

				runtime.snapGuideTime = snapped ? snapTarget : -1.0f;

				if (runtime.dragType == SequencerWidgetState::DragType::SegmentResizeLeft)
				{
					f32 maxStart = segment.end - minDuration;
					f32 newStart = snapped ? edgeTime : sequencerSnapTime(*state, edgeTime);

					if (state->push)
					{
						// clamp so the pushed previous segments stay in range
						if (!state->extendEndTime)
						{
							for (i32 iteration = 0; iteration < 4; iteration++)
							{
								f32 cursor = newStart;
								f32 overflow = 0.0f;

								for (i32 i = runtime.dragSegment - 1; i >= 0; i--)
								{
									f32 duration = track.segments[i].end - track.segments[i].start;
									f32 segEnd = std::min(track.segments[i].end, cursor);
									f32 segStart = segEnd - duration;

									overflow = std::max(overflow, state->startTime - segStart);
									cursor = segStart;
								}

								if (overflow <= 0.0001f)
									break;

								newStart += overflow;
							}
						}

						newStart = std::min(newStart, maxStart);
						newStart = std::max(newStart, state->startTime);

						if (newStart != segment.start)
						{
							segment.start = newStart;
							changed = true;
						}

						// push the segments before this one
						f32 cursor = segment.start;

						for (i32 i = runtime.dragSegment - 1; i >= 0; i--)
						{
							SequencerSegment& other = track.segments[i];
							f32 duration = other.end - other.start;
							f32 otherEnd = std::min(other.end, cursor);

							other.start = otherEnd - duration;
							other.end = otherEnd;
							cursor = other.start;
						}
					}
					else
					{
						// the previous segment limits the left edge
						f32 minBound = state->startTime;

						if (runtime.dragSegment > 0)
							minBound = std::max(minBound, track.segments[runtime.dragSegment - 1].end);

						f32 clampedStart = sequencerClamp(newStart, minBound, std::max(maxStart, minBound));

						if (clampedStart != segment.start)
						{
							segment.start = clampedStart;
							changed = true;
						}
					}
				}
				else
				{
					f32 minEnd = segment.start + minDuration;
					f32 newEnd = snapped
						? edgeTime
						: (state->extendEndTime ? sequencerSnapTimeNoMax(*state, edgeTime) : sequencerSnapTime(*state, edgeTime));

					newEnd = std::max(newEnd, minEnd);

					if (state->push)
					{
						// clamp so the pushed segments after this one stay in range
						if (!state->extendEndTime)
						{
							for (i32 iteration = 0; iteration < 4; iteration++)
							{
								f32 cursor = newEnd;
								f32 overflow = 0.0f;

								for (i32 i = runtime.dragSegment + 1; i < (i32)track.segments.size(); i++)
								{
									f32 duration = track.segments[i].end - track.segments[i].start;
									f32 segStart = std::max(track.segments[i].start, cursor);
									f32 segEnd = segStart + duration;

									overflow = std::max(overflow, segEnd - state->endTime);
									cursor = segEnd;
								}

								if (overflow <= 0.0001f)
									break;

								newEnd -= overflow;
							}

							newEnd = std::max(newEnd, minEnd);
						}

						if (newEnd != segment.end)
						{
							segment.end = newEnd;
							changed = true;
						}

						// push the segments after this one
						f32 cursor = segment.end;
						f32 lastEnd = segment.end;

						for (i32 i = runtime.dragSegment + 1; i < (i32)track.segments.size(); i++)
						{
							SequencerSegment& other = track.segments[i];
							f32 duration = other.end - other.start;
							f32 otherStart = std::max(other.start, cursor);

							other.start = otherStart;
							other.end = otherStart + duration;
							cursor = other.end;
							lastEnd = other.end;
						}

						if (state->extendEndTime && lastEnd > state->endTime)
							state->endTime = lastEnd;
					}
					else
					{
						// the next segment limits the right edge
						f32 maxBound = state->endTime;

						if (runtime.dragSegment + 1 < (i32)track.segments.size())
							maxBound = std::min(maxBound, track.segments[runtime.dragSegment + 1].start);

						f32 clampedEnd = sequencerClamp(newEnd, std::min(minEnd, maxBound), maxBound);

						if (clampedEnd != segment.end)
						{
							segment.end = clampedEnd;
							changed = true;
						}
					}
				}

				sequencerSortSegments(track);
				consumed = true;
				break;
			}

			case SequencerWidgetState::DragType::BoxSelect:
			{
				runtime.boxEnd = mouse;
				Rect selectionRect = {
					std::min(runtime.boxStart.x, runtime.boxEnd.x),
					std::min(runtime.boxStart.y, runtime.boxEnd.y),
					fabsf(runtime.boxEnd.x - runtime.boxStart.x),
					fabsf(runtime.boxEnd.y - runtime.boxStart.y) };
				bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

				if (!controlDown)
					sequencerClearSelection(*state);

				f32 segmentHeight = sequencerSettings.segmentHeight * scale;

				for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
				{
					SequencerTrack& track = state->tracks[trackIndex];

					if (!track.enabled || track.locked)
						continue;

					for (i32 segmentIndex = 0; segmentIndex < (i32)track.segments.size(); segmentIndex++)
					{
						if (sequencerSegmentInsideRect(*state, canvasRect, selectionRect, rowsTop, rowHeight, segmentHeight, trackIndex, segmentIndex))
							track.segments[segmentIndex].selected = true;
					}
				}

				changed = true;
				consumed = true;
				break;
			}

			default:
				break;
			}
		}

		if (event.type == InputEvent::Type::MouseUp && runtime.dragType != SequencerWidgetState::DragType::None)
		{
			runtime.dragType = SequencerWidgetState::DragType::None;
			runtime.pushAppliedDelta = 0.0f;
			runtime.dragItems.clear();
			runtime.rippleItems.clear();
			runtime.snapGuideTime = -1.0f;
			endCapture();
			consumed = true;
		}

		if (!consumed
			&& event.type == InputEvent::Type::Key
			&& event.key.down
			&& runtime.dragType == SequencerWidgetState::DragType::None
			&& !ctx->textInput.id
			&& (ctx->widget.focusedId == sequencerId
				|| (event.key.code == KeyCode::Space && mouseInSequencer)))
		{
			bool controlDown = has(event.key.modifiers, KeyModifiers::Control);
			bool shiftDown = has(event.key.modifiers, KeyModifiers::Shift);

			switch (event.key.code)
			{
			case KeyCode::Delete:
			case KeyCode::Backspace:
				if (sequencerDeleteSelectedSegments(*state) > 0)
					changed = true;

				consumed = true;
				break;

			case KeyCode::A:
				if (controlDown)
				{
					for (auto& track : state->tracks)
					{
						if (!track.enabled || track.locked)
							continue;

						for (auto& segment : track.segments)
							segment.selected = true;
					}

					changed = true;
					consumed = true;
				}
				break;

			case KeyCode::ArrowLeft:
			case KeyCode::ArrowRight:
			{
				f32 step = 1.0f / state->frameRate;
				f32 delta = event.key.code == KeyCode::ArrowLeft ? -step : step;

				if (shiftDown)
					delta *= 10.0f;

				std::vector<SequencerSegmentDragItem> nudgeItems;
				std::vector<SequencerSegmentDragItem> emptyRippleItems;

				for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
				{
					SequencerTrack& track = state->tracks[trackIndex];

					if (!track.enabled || track.locked)
						continue;

					for (i32 segmentIndex = 0; segmentIndex < (i32)track.segments.size(); segmentIndex++)
					{
						if (!track.segments[segmentIndex].selected)
							continue;

						nudgeItems.push_back({
							trackIndex,
							segmentIndex,
							track.segments[segmentIndex].start,
							track.segments[segmentIndex].end });
					}
				}

				f32 noSnapGuide = -1.0f;
				f32 noPushAppliedDelta = 0.0f;

				if (sequencerApplyMoveDelta(*state, nudgeItems, emptyRippleItems, delta, 0.0f, noSnapGuide, noPushAppliedDelta))
					changed = true;

				consumed = true;
				break;
			}

			case KeyCode::Space:
				state->playing = !state->playing;

				if (state->playing && state->cursorTime >= state->endTime)
					state->cursorTime = state->startTime;

				changed = true;
				consumed = true;
				break;

			case KeyCode::Home:
				state->cursorTime = state->startTime;
				changed = true;
				consumed = true;
				break;

			case KeyCode::End:
				state->cursorTime = state->endTime;
				changed = true;
				consumed = true;
				break;

			default:
				break;
			}
		}
	}

	// background and panels
	ctx->renderer.cmdSetColor(sequencerSettings.headerColor);
	ctx->renderer.cmdDrawFilledRectangle(toolbarRect);
	ctx->renderer.cmdDrawFilledRectangle(listHeaderRect);
	ctx->renderer.cmdDrawFilledRectangle(rulerRect);
	ctx->renderer.cmdSetColor(sequencerSettings.trackListColor);
	ctx->renderer.cmdDrawFilledRectangle(listRect);
	ctx->renderer.cmdSetColor(sequencerSettings.backgroundColor);
	ctx->renderer.cmdDrawFilledRectangle(canvasRect);

	// toolbar buttons
	for (u32 i = 0; i < toolbarButtonCount; i++)
	{
		const SequencerToolbarButton& button = toolbarButtons[i];

		sequencerDrawButton(
			font,
			button.rect,
			button.label,
			button.active,
			hoveredToolbarButton == (i32)i,
			runtime.pressedToolbarButton == (i32)i,
			sequencerSettings.toolbarToggleColor);
	}

	// track list rows
	{
		ctx->renderer.pushClipRect(listRect);

		f32 scrollOffset = state->trackScroll * scale;

		for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
		{
			const SequencerTrack& track = state->tracks[trackIndex];
			Rect rowRect = { listRect.x, listRect.y - scrollOffset + (f32)trackIndex * rowHeight, listRect.width, rowHeight };

			if (rowRect.bottom() < listRect.y || rowRect.y > listRect.bottom())
				continue;

			if (trackIndex == state->selectedTrack)
			{
				ctx->renderer.cmdSetColor(sequencerSettings.rowSelectedColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}
			else if (trackIndex % 2 != 0)
			{
				ctx->renderer.cmdSetColor(sequencerSettings.rowAlternateColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}

			sequencerDrawCheckbox(sequencerTrackCheckboxRect(rowRect, scale), track.enabled, false);

			// track color swatch
			Rect swatchRect = { rowRect.x + 27.0f * scale, rowRect.y + (rowRect.height - 12.0f * scale) * 0.5f, 12.0f * scale, 12.0f * scale };
			Color swatchColor = track.color;
			f32 swatchContrast = themeColorContrastFactor();

			swatchColor = { swatchColor.r * swatchContrast, swatchColor.g * swatchContrast, swatchColor.b * swatchContrast, swatchColor.a };

			if (!track.enabled)
				swatchColor.a *= 0.4f;

			ctx->renderer.cmdSetColor(swatchColor);
			ctx->renderer.cmdDrawFilledRectangle(swatchRect);

			Color nameColor = sequencerSettings.textColor;

			if (!track.enabled)
				nameColor = sequencerSettings.dimTextColor;
			else if (track.locked)
				nameColor = sequencerSettings.dimTextColor;

			const char* trackName = track.name.empty() ? "Unnamed" : track.name.c_str();

			sequencerDrawText(font, trackName, { rowRect.x + 46.0f * scale, rowRect.y, rowRect.width - 92.0f * scale, rowRect.height }, nameColor, HAlignType::Left);

			char segmentCount[32];

			snprintf(segmentCount, sizeof(segmentCount), "%u", (u32)track.segments.size());
			sequencerDrawText(font, segmentCount, { rowRect.right() - 46.0f * scale, rowRect.y, 28.0f * scale, rowRect.height }, sequencerSettings.dimTextColor, HAlignType::Right);

			ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.gridColor, scale));
			ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
		}

		ctx->renderer.popClipRect();
	}

	// track list scrollbar
	{
		SequencerTrackScrollbar scrollbar = sequencerGetTrackScrollbar(*state, sequencerSettings, listRect, rowHeight);

		if (scrollbar.maxScroll > 0.0f)
		{
			ctx->renderer.cmdSetColor(Color{ sequencerSettings.textColor.r, sequencerSettings.textColor.g, sequencerSettings.textColor.b, 0.1f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.track);
			ctx->renderer.cmdSetColor(Color{ sequencerSettings.textColor.r, sequencerSettings.textColor.g, sequencerSettings.textColor.b, 0.4f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.thumb);
		}
	}

	// track row backgrounds in the canvas
	ctx->renderer.pushClipRect(canvasRect);

	for (i32 i = 0; i < (i32)state->tracks.size(); i++)
	{
		Rect rowRect = { canvasRect.x, rowsTop + (f32)i * rowHeight, canvasRect.width, rowHeight };

		if (rowRect.bottom() < canvasRect.y || rowRect.y > canvasRect.bottom())
			continue;

		if (i == state->selectedTrack)
		{
			ctx->renderer.cmdSetColor(sequencerSettings.rowSelectedColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}
		else if (i % 2 != 0)
		{
			ctx->renderer.cmdSetColor(sequencerSettings.rowAlternateColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}

		ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
	}

	// time grid
	f32 pixelsPerSecond = state->pixelsPerSecond * scale;
	f32 timeLeft = state->scrollTime;
	f32 timeRight = timeLeft + canvasRect.width / std::max(pixelsPerSecond, 0.0001f);
	f32 majorStep = sequencerNiceStep(80.0f * scale / std::max(pixelsPerSecond, 0.0001f));
	f32 minorStep = majorStep / 5.0f;
	i32 decimals = (i32)sequencerClamp((f32)ceilf(-log10f(majorStep)), 0.0f, 4.0f);

	i32 minorStart = (i32)floorf(timeLeft / minorStep);
	i32 minorEnd = (i32)ceilf(timeRight / minorStep);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		f32 time = (f32)i * minorStep;
		f32 x = sequencerTimeToX(*state, canvasRect, time);

		if (x < canvasRect.x - 1.0f || x > canvasRect.right() + 1.0f)
			continue;

		bool major = (i % 5) == 0;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? sequencerSettings.gridStrongColor : sequencerSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, canvasRect.y }, { x, canvasRect.bottom() });
	}

	// end time line
	{
		f32 endX = sequencerTimeToX(*state, canvasRect, state->endTime);

		ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.endTimeLineColor, sequencerSettings.endTimeLineWidth * scale));
		ctx->renderer.cmdDrawLine({ endX, canvasRect.y }, { endX, canvasRect.bottom() });
	}

	ctx->renderer.popClipRect();

	// ruler ticks and labels
	ctx->renderer.pushClipRect(rulerRect);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		bool major = (i % 5) == 0;
		f32 time = (f32)i * minorStep;
		f32 x = sequencerTimeToX(*state, canvasRect, time);

		if (x < canvasRect.x - 1.0f || x > canvasRect.right() + 1.0f)
			continue;

		f32 tickLength = (major ? 6.0f : 3.0f) * scale;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? sequencerSettings.gridStrongColor : sequencerSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, rulerRect.bottom() - tickLength }, { x, rulerRect.bottom() });

		if (major)
		{
			char timeText[32];

			snprintf(timeText, sizeof(timeText), "%.*f", decimals, time);
			sequencerDrawText(font, timeText, { x + 3.0f * scale, rulerRect.y, 70.0f * scale, rulerRect.height - 4.0f * scale }, sequencerSettings.dimTextColor, HAlignType::Left);
		}
	}

	ctx->renderer.popClipRect();

	// "Tracks" label in the list header
	sequencerDrawText(font, "Tracks", listHeaderRect.contract(6.0f * scale), sequencerSettings.dimTextColor, HAlignType::Left);

	// segments
	{
		ctx->renderer.pushClipRect(canvasRect);

		f32 segmentHeight = sequencerSettings.segmentHeight * scale;

		for (i32 trackIndex = 0; trackIndex < (i32)state->tracks.size(); trackIndex++)
		{
			const SequencerTrack& track = state->tracks[trackIndex];

			if (track.segments.empty())
				continue;

			f32 rowY = rowsTop + (f32)trackIndex * rowHeight;

			for (i32 segmentIndex = 0; segmentIndex < (i32)track.segments.size(); segmentIndex++)
			{
				const SequencerSegment& segment = track.segments[segmentIndex];
				f32 x1 = sequencerTimeToX(*state, canvasRect, segment.start);
				f32 x2 = sequencerTimeToX(*state, canvasRect, segment.end);

				if (x2 < canvasRect.x || x1 > canvasRect.right())
					continue;

				// inset the drawn rect horizontally so adjacent segments get a
				// visible gap, the hit test still uses the full time span
				f32 segmentGap = sequencerSettings.segmentGap * scale;
				Rect segmentRect = {
					x1 + segmentGap * 0.5f,
					rowY + (rowHeight - segmentHeight) * 0.5f,
					std::max(x2 - x1 - segmentGap, 1.0f),
					segmentHeight };
				Color fillColor = sequencerSegmentColor(track, segment);

				// darken the fill so the white label stays readable, and more on
				// light themes so bright segment colors keep their contrast
				f32 darkness = (1.0f - sequencerClamp(sequencerSettings.segmentColorDarkness, 0.0f, 1.0f)) * themeColorContrastFactor();

				fillColor = { fillColor.r * darkness, fillColor.g * darkness, fillColor.b * darkness, fillColor.a };

				if (segment.selected)
					fillColor = sequencerShadeColor(fillColor, 0.12f);
				else if (!track.enabled)
					fillColor.a *= 0.4f;

				ctx->renderer.cmdSetColor(fillColor);
				ctx->renderer.cmdDrawFilledRectangle(segmentRect);

				// outline
				Color outlineColor = segment.selected ? sequencerSettings.playheadColor : sequencerSettings.segmentOutlineColor;
				f32 lineWidth = segment.selected ? 1.5f * scale : scale;

				if (!track.enabled)
					outlineColor.a *= 0.4f;

				ctx->renderer.cmdSetLineStyle(LineStyle(outlineColor, lineWidth));
				ctx->renderer.cmdDrawLine({ segmentRect.x, segmentRect.y }, { segmentRect.right(), segmentRect.y });
				ctx->renderer.cmdDrawLine({ segmentRect.right(), segmentRect.y }, { segmentRect.right(), segmentRect.bottom() });
				ctx->renderer.cmdDrawLine({ segmentRect.right(), segmentRect.bottom() }, { segmentRect.x, segmentRect.bottom() });
				ctx->renderer.cmdDrawLine({ segmentRect.x, segmentRect.bottom() }, { segmentRect.x, segmentRect.y });

				if (segment.tag[0])
				{
					// draw a shadow first so the label stays readable over light segments
					Rect shadowRect = {
						segmentRect.x + sequencerSettings.segmentTextShadowOffset * scale,
						segmentRect.y + sequencerSettings.segmentTextShadowOffset * scale,
						segmentRect.width,
						segmentRect.height };

					sequencerDrawText(font, segment.tag, shadowRect, sequencerSettings.segmentTextShadowColor, HAlignType::Center);
					sequencerDrawText(font, segment.tag, segmentRect, Color::white, HAlignType::Center);
				}
			}
		}

		ctx->renderer.popClipRect();
	}

	// playhead
	{
		f32 playheadX = sequencerTimeToX(*state, canvasRect, state->cursorTime);

		if (playheadX >= canvasRect.x - 1.0f && playheadX <= canvasRect.right() + 1.0f)
			drawTimeCursor(playheadX, rulerRect, canvasRect.bottom(), sequencerSettings.playheadColor, sequencerSettings.playheadWidth);
	}

	// snap guide
	if (runtime.snapGuideTime >= 0.0f
		&& (runtime.dragType == SequencerWidgetState::DragType::Segments
			|| runtime.dragType == SequencerWidgetState::DragType::SegmentResizeLeft
			|| runtime.dragType == SequencerWidgetState::DragType::SegmentResizeRight))
	{
		f32 guideX = sequencerTimeToX(*state, canvasRect, runtime.snapGuideTime);

		if (guideX >= canvasRect.x - 1.0f && guideX <= canvasRect.right() + 1.0f)
		{
			f32 dashLength = 4.0f * scale;
			f32 gapLength = 4.0f * scale;

			ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.selectionBorderColor, scale));

			for (f32 y = rulerRect.y; y < canvasRect.bottom(); y += dashLength + gapLength)
				ctx->renderer.cmdDrawLine({ guideX, y }, { guideX, std::min(y + dashLength, canvasRect.bottom()) });
		}
	}

	// box selection
	if (runtime.dragType == SequencerWidgetState::DragType::BoxSelect)
	{
		Rect selectionRect = {
			std::min(runtime.boxStart.x, runtime.boxEnd.x),
			std::min(runtime.boxStart.y, runtime.boxEnd.y),
			fabsf(runtime.boxEnd.x - runtime.boxStart.x),
			fabsf(runtime.boxEnd.y - runtime.boxStart.y) };

		ctx->renderer.cmdSetColor(sequencerSettings.selectionFillColor);
		ctx->renderer.cmdDrawFilledRectangle(selectionRect);
		ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.selectionBorderColor, scale));
		ctx->renderer.cmdDrawRectangle(selectionRect);
	}

	// disabled overlay
	if (disabled)
	{
		ctx->renderer.cmdSetColor(sequencerSettings.disabledOverlayColor);
		ctx->renderer.cmdDrawFilledRectangle(outerRect);
	}

	// cursor feedback
	if (!disabled
		&& (mouseInSequencer
			|| runtime.dragType == SequencerWidgetState::DragType::SegmentResizeLeft
			|| runtime.dragType == SequencerWidgetState::DragType::SegmentResizeRight
			|| runtime.dragType == SequencerWidgetState::DragType::BoxSelect))
	{
		SequencerSegmentHit hoveredSegment = sequencerFindSegmentUnderMouse(*state, sequencerSettings, canvasRect, rowsTop, rowHeight);

		if (runtime.dragType == SequencerWidgetState::DragType::Pan)
			mouseCursorSetType(MouseCursorType::SizeAll);
		else if (runtime.dragType == SequencerWidgetState::DragType::Playhead)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == SequencerWidgetState::DragType::SegmentResizeLeft
			|| runtime.dragType == SequencerWidgetState::DragType::SegmentResizeRight)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == SequencerWidgetState::DragType::Segments)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == SequencerWidgetState::DragType::TrackResize || trackSplitterRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == SequencerWidgetState::DragType::TrackScroll || trackScrollbar.track.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (hoveredToolbarButton >= 0 || listRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (rulerRect.contains(mouse) || fabsf(mouse.x - sequencerTimeToX(*state, canvasRect, state->cursorTime)) <= 5.0f * scale)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (hoveredSegment.track >= 0)
			mouseCursorSetType(hoveredSegment.edge != 0 ? MouseCursorType::SizeWE : MouseCursorType::HandPointing);
		else if (canvasRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::CrossHair);
	}

	return changed;
}

bool sequencerAction(SequencerState* state, SequencerAction action, f32 viewWidth)
{
	if (!state)
		return false;

	SequencerSettings settings = ctx->settings.sequencer;

	if (settings.useThemeColors)
		sequencerApplyThemeColors(settings);

	bool changed = false;
	Rect viewRect = { 0.0f, 0.0f, std::max(viewWidth, 1.0f), 1.0f };

	switch (action)
	{
	case SequencerAction::Play:
		state->playing = !state->playing;

		if (state->playing && state->cursorTime >= state->endTime)
			state->cursorTime = state->startTime;

		changed = true;
		break;

	case SequencerAction::AddSegment:
	{
		i32 trackIndex = sequencerTargetTrack(*state);

		if (trackIndex >= 0)
		{
			f32 addStart = sequencerSnapTime(*state, state->cursorTime);
			f32 addEnd = std::min(addStart + 1.0f, state->endTime);

			if (addEnd - addStart < settings.minSegmentDuration)
				addStart = std::max(state->startTime, state->endTime - settings.minSegmentDuration);

			// fall back to the first other track with a fitting gap when the
			// target track has no free space
			if (!sequencerAddSegment(*state, trackIndex, addStart, addEnd, settings.minSegmentDuration, true))
			{
				for (i32 i = 0; i < (i32)state->tracks.size(); i++)
				{
					if (i == trackIndex || !state->tracks[i].enabled || state->tracks[i].locked)
						continue;

					if (sequencerAddSegment(*state, i, addStart, addEnd, settings.minSegmentDuration, true))
					{
						changed = true;
						break;
					}
				}
			}
			else
			{
				changed = true;
			}
		}
		break;
	}

	case SequencerAction::DeleteSelected:
		changed = sequencerDeleteSelectedSegments(*state) > 0;
		break;

	case SequencerAction::ToggleSnap:
		state->snapToFrames = !state->snapToFrames;
		changed = true;
		break;

	case SequencerAction::ZoomOut:
	case SequencerAction::ZoomIn:
	case SequencerAction::FitView:
		if (viewWidth > 0.0f)
		{
			if (action == SequencerAction::FitView)
			{
				sequencerFitView(*state, settings, viewRect);
			}
			else
			{
				f32 factor = action == SequencerAction::ZoomOut ? 1.0f / settings.zoomStep : settings.zoomStep;

				sequencerZoom(*state, settings, viewRect, factor, viewRect.x + viewRect.width * 0.5f);
			}

			sequencerClampView(*state, viewRect, settings);
			changed = true;
		}
		break;
	}

	return changed;
}

}
