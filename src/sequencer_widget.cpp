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

	// trailing gap
	if (state.endTime - gapStart >= duration)
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

// moves all the given segments by the same delta, clamped so none of them
// leaves the track time range or overlaps a non-dragged segment, segments may
// cross non-dragged ones when there is free space for them, returns true if
// anything moved
static bool sequencerApplyMoveDelta(
	SequencerState& state,
	std::vector<SequencerSegmentDragItem>& dragItems,
	f32 rawDelta)
{
	std::vector<std::pair<f32, f32>> allowedRanges;
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

	f32 clampedDelta = sequencerClampDeltaToRanges(rawDelta, allowedRanges);
	bool moved = false;
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

		// find the free gap containing the desired position so the snapped
		// result never lands on a non-dragged segment
		f32 gapStart = state.startTime;
		f32 gapEnd = state.endTime;

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

		f32 newStart = sequencerClamp(sequencerSnapTime(state, desired), gapStart, gapEnd - duration);
		f32 newEnd = newStart + duration;

		newStarts[i] = newStart;

		if (newStart != segment.start || newEnd != segment.end)
		{
			segment.start = newStart;
			segment.end = newEnd;
			moved = true;
		}
	}

	if (moved)
	{
		for (auto& track : state.tracks)
			sequencerSortSegments(track);

		// sorting may reorder the segments, remap the dragged indices by start time
		for (size_t i = 0; i < dragItems.size(); i++)
		{
			SequencerSegmentDragItem& item = dragItems[i];

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

	addWidget(height);
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
	f32 toolbarHeight = sequencerSettings.toolbarHeight * scale;
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
	u32 toolbarButtonCount = sequencerBuildToolbar(font, toolbarRect, *state, toolbarButtons);

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
		switch (action)
		{
		case SequencerTbPlay:
			state->playing = !state->playing;

			if (state->playing && state->cursorTime >= state->endTime)
				state->cursorTime = state->startTime;

			changed = true;
			break;

		case SequencerTbAddSegment:
		{
			i32 trackIndex = sequencerTargetTrack(*state);

			if (trackIndex >= 0)
			{
				f32 addStart = sequencerSnapTime(*state, state->cursorTime);
				f32 addEnd = std::min(addStart + 1.0f, state->endTime);

				if (addEnd - addStart < sequencerSettings.minSegmentDuration)
					addStart = std::max(state->startTime, state->endTime - sequencerSettings.minSegmentDuration);

				// fall back to the first other track with a fitting gap when
				// the target track has no free space
				if (!sequencerAddSegment(*state, trackIndex, addStart, addEnd, sequencerSettings.minSegmentDuration, true))
				{
					for (i32 i = 0; i < (i32)state->tracks.size(); i++)
					{
						if (i == trackIndex || !state->tracks[i].enabled || state->tracks[i].locked)
							continue;

						if (sequencerAddSegment(*state, i, addStart, addEnd, sequencerSettings.minSegmentDuration, true))
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

		case SequencerTbDelete:
			if (sequencerDeleteSelectedSegments(*state) > 0)
				changed = true;
			break;

		case SequencerTbSnap:
			state->snapToFrames = !state->snapToFrames;
			changed = true;
			break;

		case SequencerTbZoomOut:
			sequencerZoom(*state, sequencerSettings, canvasRect, 1.0f / sequencerSettings.zoomStep, canvasRect.x + canvasRect.width * 0.5f);
			sequencerClampView(*state, canvasRect, sequencerSettings);
			changed = true;
			break;

		case SequencerTbZoomIn:
			sequencerZoom(*state, sequencerSettings, canvasRect, sequencerSettings.zoomStep, canvasRect.x + canvasRect.width * 0.5f);
			sequencerClampView(*state, canvasRect, sequencerSettings);
			changed = true;
			break;

		case SequencerTbFit:
			sequencerFitView(*state, sequencerSettings, canvasRect);
			sequencerClampView(*state, canvasRect, sequencerSettings);
			changed = true;
			break;

		default:
			break;
		}
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

				if (rawDelta != 0.0f && !runtime.dragItems.empty())
				{
					if (sequencerApplyMoveDelta(*state, runtime.dragItems, rawDelta))
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

				if (runtime.dragType == SequencerWidgetState::DragType::SegmentResizeLeft)
				{
					// the previous segment limits the left edge
					f32 minBound = state->startTime;

					if (runtime.dragSegment > 0)
						minBound = std::max(minBound, track.segments[runtime.dragSegment - 1].end);

					f32 maxStart = segment.end - minDuration;
					f32 newStart = sequencerClamp(sequencerSnapTime(*state, mouseTime), minBound, std::max(maxStart, minBound));

					if (newStart != segment.start)
					{
						segment.start = newStart;
						changed = true;
					}
				}
				else
				{
					// the next segment limits the right edge
					f32 maxBound = state->endTime;

					if (runtime.dragSegment + 1 < (i32)track.segments.size())
						maxBound = std::min(maxBound, track.segments[runtime.dragSegment + 1].start);

					f32 minEnd = segment.start + minDuration;
					f32 newEnd = sequencerClamp(sequencerSnapTime(*state, mouseTime), std::min(minEnd, maxBound), maxBound);

					if (newEnd != segment.end)
					{
						segment.end = newEnd;
						changed = true;
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
			runtime.dragItems.clear();
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

				if (sequencerApplyMoveDelta(*state, nudgeItems, delta))
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

				// darken the fill so the white label stays readable
				f32 darkness = 1.0f - sequencerClamp(sequencerSettings.segmentColorDarkness, 0.0f, 1.0f);

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

		ctx->renderer.cmdSetLineStyle(LineStyle(sequencerSettings.playheadColor, sequencerSettings.playheadWidth * scale));
		ctx->renderer.cmdDrawLine({ playheadX, rulerRect.y }, { playheadX, canvasRect.bottom() });

		// handle triangle in the ruler
		f32 handleSize = 5.0f * scale;
		Point tip = { playheadX, rulerRect.y + rulerRect.height - handleSize * 0.5f };
		Color handleColor = sequencerSettings.playheadColor;

		ctx->renderer.cmdSetColor(handleColor);
		ctx->renderer.cmdDrawSolidTriangle(
			{ tip.x - handleSize, tip.y - handleSize },
			{ tip.x + handleSize, tip.y - handleSize },
			tip,
			handleColor,
			handleColor,
			handleColor);
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
			mouseCursorSetType(MouseCursorType::SizeAll);
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

}
