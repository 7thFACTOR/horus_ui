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
enum TimelineToolbarAction
{
	TimelineTbPlay,
	TimelineTbKeys,
	TimelineTbCurves,
	TimelineTbAddKey,
	TimelineTbDeleteKey,
	TimelineTbSnap,
	TimelineTbZoomOut,
	TimelineTbZoomIn,
	TimelineTbFit,
	TimelineTbCount
};

struct TimelineToolbarButton
{
	TimelineToolbarAction action;
	Rect rect;
	const char* label;
	bool active;
};

struct TimelineKeyHit
{
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1;
};

static f32 timelineClamp(f32 value, f32 minValue, f32 maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}

static i32 timelineComponentCount(TimelineKeyType type)
{
	switch (type)
	{
	case TimelineKeyType::Vec2: return 2;
	case TimelineKeyType::Vec3: return 3;
	case TimelineKeyType::Vec4: return 4;
	default: return 1;
	}
}

static bool timelineComponentVisible(const TimelineChannel& channel, i32 component)
{
	if (!channel.enabled)
		return false;

	if (channel.selected)
		return true;

	if (component < 0 || component >= 32)
		return false;

	return (channel.selectedComponents & (1u << component)) != 0;
}

static bool timelineChannelHasVisibleComponent(const TimelineChannel& channel)
{
	if (!channel.enabled)
		return false;

	if (channel.selected)
		return true;

	return channel.selectedComponents != 0;
}

static const char* timelineTypeBadge(TimelineKeyType type)
{
	switch (type)
	{
	case TimelineKeyType::Int: return "I";
	case TimelineKeyType::Vec2: return "V2";
	case TimelineKeyType::Vec3: return "V3";
	case TimelineKeyType::Vec4: return "V4";
	case TimelineKeyType::Bool: return "B";
	default: return "F";
	}
}

static f32 timelineSnapTime(const TimelineState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return timelineClamp(time, state.startTime, state.endTime);
}

static f32 timelineSnapTimeNoMax(const TimelineState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return std::max(time, state.startTime);
}

static f32 timelineQuantizeComponent(TimelineKeyType type, f32 value, i32 component)
{
	if (component != 0)
		return value;

	switch (type)
	{
	case TimelineKeyType::Int: return roundf(value);
	case TimelineKeyType::Bool: return value >= 0.5f ? 1.0f : 0.0f;
	default: return value;
	}
}

static f32 timelineAutoTangent(const TimelineChannel& channel, i32 keyIndex, i32 component)
{
	i32 keyCount = (i32)channel.keys.size();

	if (keyIndex < 0 || keyIndex >= keyCount || component < 0 || component >= 4 || keyCount < 2)
		return 0.0f;

	if (keyIndex == 0)
	{
		f32 duration = channel.keys[1].time - channel.keys[0].time;
		return duration > 0.00001f ? (channel.keys[1].value[component] - channel.keys[0].value[component]) / duration : 0.0f;
	}

	if (keyIndex == keyCount - 1)
	{
		f32 duration = channel.keys[keyCount - 1].time - channel.keys[keyCount - 2].time;
		return duration > 0.00001f ? (channel.keys[keyCount - 1].value[component] - channel.keys[keyCount - 2].value[component]) / duration : 0.0f;
	}

	f32 duration = channel.keys[keyIndex + 1].time - channel.keys[keyIndex - 1].time;
	return duration > 0.00001f ? (channel.keys[keyIndex + 1].value[component] - channel.keys[keyIndex - 1].value[component]) / duration : 0.0f;
}

static f32 timelineKeyTangent(const TimelineChannel& channel, i32 keyIndex, i32 component, bool outgoing)
{
	const TimelineKey& key = channel.keys[keyIndex];

	if (key.tangentAuto)
		return timelineAutoTangent(channel, keyIndex, component);

	return outgoing ? key.outTangent[component] : key.inTangent[component];
}

static void timelineMakeKeyTangentsManual(TimelineChannel& channel, i32 keyIndex)
{
	TimelineKey& key = channel.keys[keyIndex];

	if (!key.tangentAuto)
		return;

	for (i32 i = 0; i < 4; i++)
	{
		f32 tangent = timelineAutoTangent(channel, keyIndex, i);

		key.inTangent[i] = tangent;
		key.outTangent[i] = tangent;
	}

	key.tangentAuto = false;
}

static f32 timelineEvaluateComponent(const TimelineChannel& channel, i32 component, f32 time)
{
	if (channel.keys.empty() || component < 0 || component >= 4)
		return 0.0f;

	if (time <= channel.keys.front().time)
		return channel.keys.front().value[component];

	if (time >= channel.keys.back().time)
		return channel.keys.back().value[component];

	u32 lo = 0;
	u32 hi = (u32)channel.keys.size() - 1;

	while (lo < hi)
	{
		u32 mid = (lo + hi) / 2;

		if (channel.keys[mid].time <= time)
			lo = mid + 1;
		else
			hi = mid;
	}

	const TimelineKey& first = channel.keys[lo - 1];
	const TimelineKey& second = channel.keys[lo];

	if (channel.type == TimelineKeyType::Bool || channel.type == TimelineKeyType::Int)
		return first.value[component];

	f32 duration = second.time - first.time;

	if (duration <= 0.0f)
		return second.value[component];

	f32 t = (time - first.time) / duration;
	f32 t2 = t * t;
	f32 t3 = t2 * t;
	f32 tangentIn = timelineKeyTangent(channel, (i32)lo - 1, component, true);
	f32 tangentOut = timelineKeyTangent(channel, (i32)lo, component, false);
	f32 h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	f32 h10 = t3 - 2.0f * t2 + t;
	f32 h01 = -2.0f * t3 + 3.0f * t2;
	f32 h11 = t3 - t2;

	return h00 * first.value[component]
		+ h10 * duration * tangentIn
		+ h01 * second.value[component]
		+ h11 * duration * tangentOut;
}

static void timelineEvaluateChannel(const TimelineChannel& channel, f32 time, f32* outValues)
{
	i32 components = timelineComponentCount(channel.type);

	for (i32 i = 0; i < 4; i++)
		outValues[i] = i < components ? timelineEvaluateComponent(channel, i, time) : 0.0f;
}

static void timelineSortKeys(TimelineChannel& channel)
{
	std::sort(channel.keys.begin(), channel.keys.end(), [](const TimelineKey& a, const TimelineKey& b)
	{
		return a.time < b.time;
	});
}

static bool timelineHasDraggedKey(const TimelineWidgetState& runtime, i32 channelIndex)
{
	for (auto& item : runtime.dragItems)
	{
		if (item.channel == channelIndex)
			return true;
	}

	for (auto& item : runtime.rippleItems)
	{
		if (item.channel == channelIndex)
			return true;
	}

	return false;
}

static void timelineRemapDraggedKeys(TimelineWidgetState& runtime, i32 channelIndex, i32 fromIndex, i32 toIndex)
{
	for (auto& item : runtime.dragItems)
	{
		if (item.channel != channelIndex)
			continue;

		if (item.key == fromIndex)
			item.key = toIndex;
		else if (item.key == toIndex)
			item.key = fromIndex;
	}

	for (auto& item : runtime.rippleItems)
	{
		if (item.channel != channelIndex)
			continue;

		if (item.key == fromIndex)
			item.key = toIndex;
		else if (item.key == toIndex)
			item.key = fromIndex;
	}
}

static void timelineReorderDraggedKeys(TimelineWidgetState& runtime, std::vector<TimelineChannel>& channels)
{
	for (i32 channelIndex = 0; channelIndex < (i32)channels.size(); channelIndex++)
	{
		if (!timelineHasDraggedKey(runtime, channelIndex))
			continue;

		auto& keys = channels[channelIndex].keys;

		for (i32 i = 1; i < (i32)keys.size(); i++)
		{
			i32 j = i;

			while (j > 0 && keys[j - 1].time > keys[j].time)
			{
				std::swap(keys[j - 1], keys[j]);
				timelineRemapDraggedKeys(runtime, channelIndex, j - 1, j);
				j--;
			}
		}
	}
}

static f32 timelineNiceStep(f32 rawStep)
{
	if (rawStep <= 0.0f)
		return 1.0f;

	f32 exponent = floorf(log10f(rawStep));
	f32 base = powf(10.0f, exponent);
	f32 fraction = rawStep / base;
	f32 nice = fraction <= 1.0f ? 1.0f : fraction <= 2.0f ? 2.0f : fraction <= 5.0f ? 5.0f : 10.0f;

	return nice * base;
}

static Color timelineChannelColor(i32 channelIndex, i32 component = -1)
{
	f32 hue = fmodf((f32)channelIndex * 0.61803398875f, 1.0f);

	if (component >= 0)
		hue = fmodf(hue + (f32)component * 0.045f, 1.0f);

	return colorHueToRgb(hue, 1.0f);
}

static void timelineComputeAutoValueRange(const TimelineState& state, f32& valueMin, f32& valueMax)
{
	bool hasValues = false;
	f32 computedMin = FLT_MAX;
	f32 computedMax = -FLT_MAX;

	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		i32 components = timelineComponentCount(channel.type);

		for (auto& key : channel.keys)
		{
			for (i32 i = 0; i < components; i++)
			{
				computedMin = std::min(computedMin, key.value[i]);
				computedMax = std::max(computedMax, key.value[i]);
				hasValues = true;
			}
		}
	}

	if (hasValues)
	{
		f32 range = computedMax - computedMin;

		if (range < 0.00001f)
		{
			computedMin -= 0.5f;
			computedMax += 0.5f;
		}
		else
		{
			f32 padding = range * 0.1f;

			computedMin -= padding;
			computedMax += padding;
		}
	}
	else
	{
		computedMin = 0.0f;
		computedMax = 1.0f;
	}

	valueMin = computedMin;
	valueMax = computedMax;

	if (valueMax - valueMin < 0.00001f)
		valueMax = valueMin + 1.0f;
}

static void timelineComputeValueRange(const TimelineState& state, f32& valueMin, f32& valueMax)
{
	if (state.viewMode == TimelineViewMode::Curves && state.autoValueRange)
	{
		timelineComputeAutoValueRange(state, valueMin, valueMax);
		return;
	}

	valueMin = state.valueMin;
	valueMax = state.valueMax;

	if (valueMax - valueMin < 0.00001f)
		valueMax = valueMin + 1.0f;
}

static f32 timelineTimeToX(const TimelineState& state, const Rect& plotRect, f32 time)
{
	return plotRect.x + (time - state.scrollTime) * state.pixelsPerSecond * ctx->scale;
}

static f32 timelineXToTime(const TimelineState& state, const Rect& plotRect, f32 x)
{
	f32 pixelsPerSecond = state.pixelsPerSecond * ctx->scale;

	if (pixelsPerSecond <= 0.0f)
		return state.scrollTime;

	return state.scrollTime + (x - plotRect.x) / pixelsPerSecond;
}

static f32 timelineValueToY(const Rect& plotRect, f32 value, f32 valueMin, f32 valueMax)
{
	f32 range = valueMax - valueMin;

	if (range <= 0.00001f)
		range = 1.0f;

	return plotRect.bottom() - (value - valueMin) / range * plotRect.height;
}

static f32 timelineYToValue(const Rect& plotRect, f32 y, f32 valueMin, f32 valueMax)
{
	f32 range = valueMax - valueMin;

	if (range <= 0.00001f)
		range = 1.0f;

	return valueMin + (plotRect.bottom() - y) / std::max(plotRect.height, 1.0f) * range;
}

static Color timelineSampleElementColor(WidgetElementId id, WidgetStateType stateType, bool useTextColor)
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

static Color timelineShadeColor(const Color& color, f32 amount)
{
	return {
		timelineClamp(color.r + amount, 0.0f, 1.0f),
		timelineClamp(color.g + amount, 0.0f, 1.0f),
		timelineClamp(color.b + amount, 0.0f, 1.0f),
		color.a };
}

static void timelineApplyThemeColors(TimelineSettings& timelineSettings)
{
	if (!ctx->theme)
		return;

	Color windowColor = timelineSampleElementColor(WidgetElementId::WindowBody, WidgetStateType::Normal, false);
	Color textColor = timelineSampleElementColor(WidgetElementId::LabelBody, WidgetStateType::Normal, true);
	Color accentColor = timelineSampleElementColor(WidgetElementId::LinkBody, WidgetStateType::Normal, true);
	f32 luminance = windowColor.r * 0.299f + windowColor.g * 0.587f + windowColor.b * 0.114f;
	f32 rowShade = luminance < 0.5f ? 0.025f : -0.025f;

	timelineSettings.backgroundColor = windowColor;
	timelineSettings.channelListColor = timelineShadeColor(windowColor, rowShade * 1.4f);
	timelineSettings.headerColor = timelineShadeColor(windowColor, rowShade * 2.0f);
	timelineSettings.rowAlternateColor = timelineShadeColor(windowColor, rowShade);
	timelineSettings.rowSelectedColor = { accentColor.r, accentColor.g, accentColor.b, 0.5f };
	timelineSettings.gridColor = { textColor.r, textColor.g, textColor.b, 0.12f };
	timelineSettings.gridStrongColor = { textColor.r, textColor.g, textColor.b, 0.22f };
	timelineSettings.textColor = textColor;
	timelineSettings.dimTextColor = { textColor.r, textColor.g, textColor.b, 0.55f };
	timelineSettings.keySelectedColor = accentColor;
	timelineSettings.playheadColor = Color::white;
	timelineSettings.toolbarToggleColor = timelineShadeColor(
		timelineSettings.toolbarToggleColor,
		luminance < 0.3f ? 0.08f : (luminance > 0.55f ? -0.1f : 0.0f));
	timelineSettings.selectionFillColor = { accentColor.r, accentColor.g, accentColor.b, 0.14f };
	timelineSettings.selectionBorderColor = { accentColor.r, accentColor.g, accentColor.b, 0.7f };
	timelineSettings.endTimeLineColor = { textColor.r, textColor.g, textColor.b, 0.35f };
}

struct TimelineTangentHit
{
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1;
	bool outgoing = false;
};

static Point timelineTangentHandlePosition(
	const TimelineState& state,
	const TimelineChannel& channel,
	i32 keyIndex,
	i32 component,
	bool outgoing,
	const Rect& plotRect,
	f32 valueMin,
	f32 valueMax,
	f32 handlePixels)
{
	const TimelineKey& key = channel.keys[keyIndex];
	f32 pixelsPerSecond = state.pixelsPerSecond * ctx->scale;
	f32 handleTime = pixelsPerSecond > 0.00001f ? handlePixels / pixelsPerSecond : 0.0f;
	f32 tangent = timelineKeyTangent(channel, keyIndex, component, outgoing);
	f32 time = outgoing ? key.time + handleTime : key.time - handleTime;
	f32 value = outgoing ? key.value[component] + tangent * handleTime : key.value[component] - tangent * handleTime;

	return { timelineTimeToX(state, plotRect, time), timelineValueToY(plotRect, value, valueMin, valueMax) };
}

static TimelineTangentHit timelineFindTangentHandle(
	const TimelineState& state,
	const TimelineSettings& settings,
	const Rect& plotRect,
	f32 valueMin,
	f32 valueMax,
	const Point& mouse)
{
	TimelineTangentHit result;
	f32 hitRadius = settings.keyHitRadius * ctx->scale;
	f32 handlePixels = settings.tangentHandleLength * ctx->scale;

	for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
	{
		const TimelineChannel& channel = state.channels[channelIndex];

		if (!channel.enabled || channel.type == TimelineKeyType::Bool || channel.type == TimelineKeyType::Int)
			continue;

		i32 components = timelineComponentCount(channel.type);

		for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
		{
			if (!channel.keys[keyIndex].selected)
				continue;

			for (i32 component = 0; component < components; component++)
			{
				if (!timelineComponentVisible(channel, component))
					continue;

				for (i32 side = 0; side < 2; side++)
				{
					bool outgoing = side == 0;
					Point handle = timelineTangentHandlePosition(state, channel, keyIndex, component, outgoing, plotRect, valueMin, valueMax, handlePixels);

					if ((handle - mouse).getLength() < hitRadius)
					{
						result.channel = channelIndex;
						result.key = keyIndex;
						result.component = component;
						result.outgoing = outgoing;
						return result;
					}
				}
			}
		}
	}

	return result;
}

struct TimelineToolbarField
{
	Rect rect;
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1; // -1 is the key time, -2 is the key tag
};

static u32 timelineBuildToolbarFields(
	const Rect& toolbarRect,
	const TimelineToolbarButton* buttons,
	u32 buttonCount,
	const TimelineSettings& settings,
	const TimelineState& state,
	TimelineToolbarField* fields,
	u32 maxFields)
{
	if (buttonCount == 0 || maxFields == 0)
		return 0;

	i32 selectedChannel = -1;
	i32 selectedKey = -1;
	i32 selectedCount = 0;

	for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
	{
		for (i32 keyIndex = 0; keyIndex < (i32)state.channels[channelIndex].keys.size(); keyIndex++)
		{
			if (!state.channels[channelIndex].keys[keyIndex].selected)
				continue;

			if (selectedCount == 0)
			{
				selectedChannel = channelIndex;
				selectedKey = keyIndex;
			}

			selectedCount++;
		}
	}

	if (selectedCount != 1)
		return 0;

	const TimelineChannel& channel = state.channels[selectedChannel];
	f32 scale = ctx->scale;
	f32 x = buttons[buttonCount - 1].rect.right() + 10.0f * scale;
	f32 y = toolbarRect.y + 3.0f * scale;
	f32 height = std::max(toolbarRect.height - 6.0f * scale, 1.0f);
	f32 gap = 4.0f * scale;
	u32 count = 0;

	auto addField = [&](i32 component, f32 width)
	{
		if (count >= maxFields || x + width > toolbarRect.right() - 4.0f * scale)
			return;

		fields[count].rect = { x, y, width, height };
		fields[count].channel = selectedChannel;
		fields[count].key = selectedKey;
		fields[count].component = component;
		count++;
		x += width + gap;
	};

	addField(-1, settings.toolbarTimeFieldWidth * scale);

	if (channel.type != TimelineKeyType::Bool)
	{
		i32 components = timelineComponentCount(channel.type);

		for (i32 component = 0; component < components; component++)
			addField(component, settings.toolbarValueFieldWidth * scale);
	}

	addField(-2, settings.toolbarTagFieldWidth * scale);

	return count;
}

static const char* timelineComponentLabel(TimelineKeyType type, i32 component)
{
	switch (type)
	{
	case TimelineKeyType::Vec2:
	case TimelineKeyType::Vec3:
	case TimelineKeyType::Vec4:
	{
		static const char* axisLabels[4] = { "X", "Y", "Z", "W" };
		return component >= 0 && component < 4 ? axisLabels[component] : "V";
	}
	case TimelineKeyType::Int:
		return "I";
	default:
		return "V";
	}
}

static Point timelineTextSize(Font* font, const char* text)
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

static void timelineDrawText(Font* font, const char* text, const Rect& rect, const Color& color, HAlignType align, bool ellipsis = true)
{
	if (!text || !text[0])
		return;

	ctx->renderer.cmdSetFont(font);
	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawTextInBox(text, rect, align, VAlignType::Center, ellipsis, true);
}

static void timelineDrawKeyTag(Font* font, const char* tag, f32 x, f32 centerY, const TimelineSettings& settings, f32 scale)
{
	Point textSize = timelineTextSize(font, tag);
	f32 padX = 3.0f * scale;
	f32 padY = 1.5f * scale;
	Color backgroundColor = settings.headerColor;
	Rect backgroundRect = {
		x,
		centerY - textSize.y * 0.5f - padY,
		textSize.x + padX * 2.0f,
		textSize.y + padY * 2.0f };

	backgroundColor.a = 0.85f;
	ctx->renderer.cmdSetColor(backgroundColor);
	ctx->renderer.cmdDrawFilledRectangle(backgroundRect);
	timelineDrawText(font, tag, { x + padX, centerY - textSize.y * 0.5f, textSize.x, textSize.y }, settings.textColor, HAlignType::Left);
}

static void timelineDrawDiamond(const Point& center, f32 radius, const Color& color)
{
	Point top = { center.x, center.y - radius };
	Point right = { center.x + radius, center.y };
	Point bottom = { center.x, center.y + radius };
	Point left = { center.x - radius, center.y };

	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawSolidTriangle(top, right, bottom, color, color, color);
	ctx->renderer.cmdDrawSolidTriangle(top, bottom, left, color, color, color);
}

static void timelineDrawButton(Font* font, const Rect& rect, const char* label, bool active, bool hovered, bool pressed, const Color& activeColor)
{
	auto& buttonElem = ctx->theme->getElement(WidgetElementId::ButtonBody);
	ThemeElement::State* state = &buttonElem.normalState();

	if (pressed || active)
	{
		auto& pressedState = buttonElem.getState(WidgetStateType::Pressed);
		Color backgroundColor = activeColor;

		if (pressed)
			backgroundColor = timelineShadeColor(backgroundColor, -0.08f);
		else if (hovered)
			backgroundColor = timelineShadeColor(backgroundColor, 0.06f);

		ctx->renderer.cmdSetColor(backgroundColor);
		ctx->renderer.cmdDrawImageBordered(pressedState.image, pressedState.border, rect, ctx->scale);
		timelineDrawText(font, label, rect, Color::white, HAlignType::Center);
		return;
	}

	if (hovered)
		state = &buttonElem.getState(WidgetStateType::Hovered);

	ctx->renderer.cmdSetColor(state->color);
	ctx->renderer.cmdDrawImageBordered(state->image, state->border, rect, ctx->scale);
	timelineDrawText(font, label, rect, state->textColor, HAlignType::Center);
}

static void timelineDrawCheckbox(const Rect& rect, bool checked, bool disabled)
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

static Rect timelineChannelCheckboxRect(const Rect& rowRect, f32 scale)
{
	f32 size = 14.0f * scale;

	return { rowRect.x + 8.0f * scale, rowRect.y + (rowRect.height - size) * 0.5f, size, size };
}

static Rect timelineChannelExpandRect(const Rect& rowRect, f32 scale)
{
	f32 size = 10.0f * scale;

	return { rowRect.x + 27.0f * scale, rowRect.y + (rowRect.height - size) * 0.5f, size, size };
}

static Rect timelineChannelBadgeRect(const Rect& rowRect, f32 scale)
{
	f32 height = 14.0f * scale;

	return { rowRect.x + 42.0f * scale, rowRect.y + (rowRect.height - height) * 0.5f, 24.0f * scale, height };
}

struct TimelineChannelRow
{
	i32 channel = -1;
	i32 component = -1;
};

static void timelineBuildChannelRows(const TimelineState& state, std::vector<TimelineChannelRow>& rows)
{
	rows.clear();

	bool showSubChannels = state.viewMode == TimelineViewMode::Curves;

	for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
	{
		const TimelineChannel& channel = state.channels[channelIndex];

		rows.push_back({ channelIndex, -1 });

		if (showSubChannels && channel.expanded)
		{
			i32 components = timelineComponentCount(channel.type);

			if (components > 1)
			{
				for (i32 component = 0; component < components; component++)
					rows.push_back({ channelIndex, component });
			}
		}
	}
}

static i32 timelineTotalChannelRows(const TimelineState& state)
{
	i32 count = 0;
	bool showSubChannels = state.viewMode == TimelineViewMode::Curves;

	for (auto& channel : state.channels)
	{
		count++;

		if (showSubChannels && channel.expanded)
		{
			i32 components = timelineComponentCount(channel.type);

			if (components > 1)
				count += components;
		}
	}

	return count;
}

static void timelineClampChannelScroll(TimelineState& state, const Rect& listRect, const TimelineSettings& settings)
{
	f32 visibleHeight = listRect.height / ctx->scale;
	f32 totalHeight = (f32)timelineTotalChannelRows(state) * settings.rowHeight;
	f32 maxScroll = std::max(0.0f, totalHeight - visibleHeight);

	state.channelScroll = timelineClamp(state.channelScroll, 0.0f, maxScroll);
}

struct TimelineChannelScrollbar
{
	Rect track;
	Rect thumb;
	f32 maxScroll = 0.0f;
};

static TimelineChannelScrollbar timelineGetChannelScrollbar(
	const TimelineState& state,
	const TimelineSettings& settings,
	const Rect& listRect,
	f32 rowHeight)
{
	TimelineChannelScrollbar scrollbar;
	f32 contentHeight = (f32)timelineTotalChannelRows(state) * rowHeight;

	if (contentHeight <= listRect.height + 0.5f || listRect.height <= 0.0f)
		return scrollbar;

	f32 scale = ctx->scale;
	f32 trackWidth = settings.channelScrollbarWidth * scale;

	if (trackWidth <= 0.0f)
		return scrollbar;

	f32 maxScroll = std::max(0.0f, (contentHeight - listRect.height) / scale);
	f32 minThumbSize = std::min(settings.channelScrollbarMinThumbSize * scale, listRect.height);
	f32 thumbHeight = std::max(minThumbSize, listRect.height * (listRect.height / contentHeight));

	thumbHeight = std::min(thumbHeight, listRect.height);
	scrollbar.track = { listRect.right() - trackWidth, listRect.y, trackWidth, listRect.height };
	scrollbar.maxScroll = maxScroll;

	f32 travel = std::max(listRect.height - thumbHeight, 1.0f);
	f32 ratio = maxScroll > 0.0f ? timelineClamp(state.channelScroll / maxScroll, 0.0f, 1.0f) : 0.0f;
	scrollbar.thumb = { scrollbar.track.x, listRect.y + travel * ratio, trackWidth, thumbHeight };

	return scrollbar;
}

static void timelineClampView(TimelineState& state, const Rect& plotRect, const TimelineSettings& settings)
{
	state.pixelsPerSecond = timelineClamp(state.pixelsPerSecond, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);
	f32 visibleTime = plotRect.width / pixelsPerSecond;
	f32 maxScrollTime = std::max(state.startTime, state.endTime - visibleTime);

	state.scrollTime = timelineClamp(state.scrollTime, state.startTime, maxScrollTime);
	state.cursorTime = timelineClamp(state.cursorTime, state.startTime, state.endTime);
}

static i32 timelineTargetChannel(const TimelineState& state)
{
	if (state.selectedChannel >= 0
		&& state.selectedChannel < (i32)state.channels.size()
		&& state.channels[state.selectedChannel].enabled)
		return state.selectedChannel;

	for (i32 i = 0; i < (i32)state.channels.size(); i++)
		if (state.channels[i].enabled)
			return i;

	return -1;
}

static void timelineClearSelection(TimelineState& state)
{
	for (auto& channel : state.channels)
		for (auto& key : channel.keys)
			key.selected = false;
}

static bool timelineAddKey(TimelineState& state, i32 channelIndex, f32 time, const f32* values, bool select)
{
	if (channelIndex < 0 || channelIndex >= (i32)state.channels.size())
		return false;

	TimelineChannel& channel = state.channels[channelIndex];

	if (!channel.enabled)
		return false;

	time = timelineSnapTime(state, time);

	f32 newValues[4] = { 0, 0, 0, 0 };

	if (values)
	{
		for (i32 i = 0; i < 4; i++)
			newValues[i] = values[i];
	}
	else
	{
		timelineEvaluateChannel(channel, time, newValues);
	}

	i32 components = timelineComponentCount(channel.type);

	for (i32 i = 0; i < 4; i++)
		newValues[i] = i < components ? timelineQuantizeComponent(channel.type, newValues[i], i) : 0.0f;

	for (auto& key : channel.keys)
	{
		if (fabsf(key.time - time) <= 0.0001f)
		{
			if (values)
			{
				for (i32 i = 0; i < 4; i++)
					key.value[i] = newValues[i];
			}

			if (select)
			{
				timelineClearSelection(state);
				key.selected = true;
			}

			state.selectedChannel = channelIndex;
			return true;
		}
	}

	if (select)
		timelineClearSelection(state);

	TimelineKey key;
	key.time = time;
	key.selected = select;

	for (i32 i = 0; i < 4; i++)
		key.value[i] = newValues[i];

	auto insertPos = std::lower_bound(channel.keys.begin(), channel.keys.end(), time, [](const TimelineKey& a, f32 value)
	{
		return a.time < value;
	});

	channel.keys.insert(insertPos, key);
	state.selectedChannel = channelIndex;

	return true;
}

static i32 timelineDeleteSelectedKeys(TimelineState& state)
{
	i32 deleted = 0;

	for (auto& channel : state.channels)
	{
		size_t writeIndex = 0;

		for (size_t i = 0; i < channel.keys.size(); i++)
		{
			if (channel.keys[i].selected)
			{
				deleted++;
				continue;
			}

			if (writeIndex != i)
				channel.keys[writeIndex] = channel.keys[i];

			writeIndex++;
		}

		channel.keys.resize(writeIndex);
	}

	return deleted;
}

static void timelineSelectAll(TimelineState& state)
{
	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		for (auto& key : channel.keys)
			key.selected = true;
	}
}

static void timelineNudgeSelectedKeys(TimelineState& state, f32 delta)
{
	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		bool changed = false;

		for (auto& key : channel.keys)
		{
			if (!key.selected)
				continue;

			key.time = timelineClamp(key.time + delta, state.startTime, state.endTime);
			changed = true;
		}

		if (changed)
			timelineSortKeys(channel);
	}
}

static void timelineZoom(TimelineState& state, const TimelineSettings& settings, const Rect& plotRect, f32 factor, f32 anchorX)
{
	f32 oldPixelsPerSecond = std::max(state.pixelsPerSecond, 0.0001f);
	f32 anchorTime = state.scrollTime + (anchorX - plotRect.x) / (oldPixelsPerSecond * ctx->scale);
	f32 newPixelsPerSecond = timelineClamp(state.pixelsPerSecond * factor, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	state.pixelsPerSecond = newPixelsPerSecond;
	state.scrollTime = anchorTime - (anchorX - plotRect.x) / (newPixelsPerSecond * ctx->scale);
}

static void timelineFitView(TimelineState& state, const TimelineSettings& settings, const Rect& plotRect)
{
	f32 timeRange = std::max(state.endTime - state.startTime, 0.001f);

	state.scrollTime = state.startTime;
	state.pixelsPerSecond = timelineClamp(plotRect.width / (timeRange * ctx->scale), settings.minPixelsPerSecond, settings.maxPixelsPerSecond);
}

static u32 timelineBuildToolbar(
	Font* font,
	const Rect& toolbarRect,
	const TimelineState& state,
	TimelineToolbarButton* buttons)
{
	f32 scale = ctx->scale;
	f32 x = toolbarRect.x + 4.0f * scale;
	f32 y = toolbarRect.y + 3.0f * scale;
	f32 height = std::max(toolbarRect.height - 6.0f * scale, 1.0f);
	u32 count = 0;

	auto addButton = [&](TimelineToolbarAction action, const char* label, bool active, f32 gap)
	{
		x += gap * scale;
		Point textSize = timelineTextSize(font, label);
		f32 width = textSize.x + 16.0f * scale;

		buttons[count].action = action;
		buttons[count].rect = { x, y, width, height };
		buttons[count].label = label;
		buttons[count].active = active;
		count++;
		x += width + 2.0f * scale;
	};

	addButton(TimelineTbPlay, state.playing ? "Pause" : "Play", state.playing, 0.0f);
	addButton(TimelineTbKeys, "Keys", state.viewMode == TimelineViewMode::Keyframes, 6.0f);
	addButton(TimelineTbCurves, "Curves", state.viewMode == TimelineViewMode::Curves, 6.0f);
	addButton(TimelineTbAddKey, "+ Key", false, 10.0f);
	addButton(TimelineTbDeleteKey, "Del", false, 3.0f);
	addButton(TimelineTbSnap, "Snap", state.snapToFrames, 10.0f);
	addButton(TimelineTbZoomOut, "-", false, 10.0f);
	addButton(TimelineTbZoomIn, "+", false, 2.0f);
	addButton(TimelineTbFit, "Fit", false, 6.0f);

	return count;
}

static TimelineKeyHit timelineFindKeyUnderMouse(
	TimelineState& state,
	const TimelineSettings& settings,
	const Rect& plotRect,
	f32 rowsTop,
	f32 rowHeight,
	f32 valueMin,
	f32 valueMax)
{
	TimelineKeyHit result;
	f32 hitRadius = std::max(settings.keyHitRadius * ctx->scale, settings.keySize * ctx->scale + 2.0f * ctx->scale);
	f32 bestDistanceSquared = hitRadius * hitRadius;
	f32 scale = ctx->scale;
	Point mouse = ctx->mousePosition;

	if (state.viewMode == TimelineViewMode::Keyframes)
	{
		i32 row = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

		if (row < 0 || row >= (i32)state.channels.size() || !state.channels[row].enabled)
			return result;

		f32 centerY = rowsTop + (f32)row * rowHeight + rowHeight * 0.5f;
		f32 dy = mouse.y - centerY;

		for (i32 i = 0; i < (i32)state.channels[row].keys.size(); i++)
		{
			f32 x = timelineTimeToX(state, plotRect, state.channels[row].keys[i].time);
			f32 dx = mouse.x - x;
			f32 distanceSquared = dx * dx + dy * dy;

			if (distanceSquared <= bestDistanceSquared)
			{
				bestDistanceSquared = distanceSquared;
				result.channel = row;
				result.key = i;
				result.component = -1;
			}
		}
	}
	else
	{
		f32 pixelsPerSecond = state.pixelsPerSecond * scale;
		f32 timeLeft = state.scrollTime;
		f32 timeRight = timeLeft + plotRect.width / std::max(pixelsPerSecond, 0.0001f);

		for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
		{
			TimelineChannel& channel = state.channels[channelIndex];

			if (!channel.enabled)
				continue;

			i32 components = timelineComponentCount(channel.type);

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				TimelineKey& key = channel.keys[keyIndex];

				if (key.time < timeLeft - 0.001f || key.time > timeRight + 0.001f)
					continue;

				f32 x = timelineTimeToX(state, plotRect, key.time);

				for (i32 component = 0; component < components; component++)
				{
					if (!timelineComponentVisible(channel, component))
						continue;

					f32 y = timelineValueToY(plotRect, key.value[component], valueMin, valueMax);
					f32 dx = mouse.x - x;
					f32 dy = mouse.y - y;
					f32 distanceSquared = dx * dx + dy * dy;

					if (distanceSquared <= bestDistanceSquared)
					{
						bestDistanceSquared = distanceSquared;
						result.channel = channelIndex;
						result.key = keyIndex;
						result.component = component;
					}
				}
			}
		}
	}

	return result;
}

static bool timelineKeyInsideRect(
	const TimelineState& state,
	const Rect& plotRect,
	const Rect& selectionRect,
	f32 rowsTop,
	f32 rowHeight,
	f32 valueMin,
	f32 valueMax,
	i32 channelIndex,
	i32 keyIndex)
{
	const TimelineChannel& channel = state.channels[channelIndex];
	const TimelineKey& key = channel.keys[keyIndex];

	if (state.viewMode == TimelineViewMode::Keyframes)
	{
		f32 x = timelineTimeToX(state, plotRect, key.time);
		f32 y = rowsTop + (f32)channelIndex * rowHeight + rowHeight * 0.5f;

		return selectionRect.contains({ x, y });
	}

	i32 components = timelineComponentCount(channel.type);
	f32 x = timelineTimeToX(state, plotRect, key.time);

	for (i32 component = 0; component < components; component++)
	{
		if (!timelineComponentVisible(channel, component))
			continue;

		f32 y = timelineValueToY(plotRect, key.value[component], valueMin, valueMax);

		if (selectionRect.contains({ x, y }))
			return true;
	}

	return false;
}

bool timeline(const char* id, TimelineState* state, f32 height)
{
	if (!state)
		return false;

	Settings& settings = ctx->settings;
	TimelineSettings activeTimelineSettings = settings.timeline;

	if (activeTimelineSettings.useThemeColors)
		timelineApplyThemeColors(activeTimelineSettings);

	const TimelineSettings& timelineSettings = activeTimelineSettings;
	f32 scale = ctx->scale;

	if (height <= 0.0f)
	{
		height = (ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y)) / scale;

		if (height <= 0.0f)
			height = timelineSettings.defaultHeight;
	}

	ctx->id = genId(id);
	WidgetId timelineId = ctx->id;

	addWidget(height);
	widgetSetFocusable();

	ctx->widget.visible = true;

	Rect outerRect = ctx->widget.rect;
	bool disabled = ctx->widget.disabled;
	bool changed = false;

	if (state->endTime <= state->startTime)
		state->endTime = state->startTime + 1.0f;

	if (state->pixelsPerSecond <= 0.0f)
		state->pixelsPerSecond = timelineSettings.defaultPixelsPerSecond;

	state->pixelsPerSecond = timelineClamp(state->pixelsPerSecond, timelineSettings.minPixelsPerSecond, timelineSettings.maxPixelsPerSecond);

	if (state->frameRate <= 0.0f)
		state->frameRate = 30.0f;

	if (state->selectedChannel < -1 || state->selectedChannel >= (i32)state->channels.size())
		state->selectedChannel = -1;

	state->cursorTime = timelineClamp(state->cursorTime, state->startTime, state->endTime);

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

	auto& runtime = ctx->timelineStates[timelineId];
	f32 toolbarHeight = timelineSettings.toolbarHeight * scale;
	f32 rulerHeight = timelineSettings.rulerHeight * scale;
	f32 rowHeight = timelineSettings.rowHeight * scale;

	if (runtime.channelListWidth <= 0.0f)
		runtime.channelListWidth = timelineSettings.channelListWidth;

	f32 minChannelWidth = std::min(80.0f * scale, outerRect.width * 0.5f);
	f32 maxChannelWidth = std::max(minChannelWidth, outerRect.width - std::min(120.0f * scale, outerRect.width * 0.5f));
	f32 channelWidth = timelineClamp(runtime.channelListWidth * scale, minChannelWidth, maxChannelWidth);

	channelWidth = std::max(channelWidth, std::min(40.0f * scale, outerRect.width));

	Rect toolbarRect = { outerRect.x, outerRect.y, outerRect.width, toolbarHeight };
	Rect listHeaderRect = { outerRect.x, outerRect.y + toolbarHeight, channelWidth, rulerHeight };
	Rect listRect = { outerRect.x, outerRect.y + toolbarHeight + rulerHeight, channelWidth, outerRect.height - toolbarHeight - rulerHeight };
	Rect rulerRect = { outerRect.x + channelWidth, outerRect.y + toolbarHeight, outerRect.width - channelWidth, rulerHeight };
	Rect canvasRect = { outerRect.x + channelWidth, outerRect.y + toolbarHeight + rulerHeight, outerRect.width - channelWidth, outerRect.height - toolbarHeight - rulerHeight };

	listRect.height = std::max(listRect.height, 0.0f);
	rulerRect.width = std::max(rulerRect.width, 0.0f);
	canvasRect.width = std::max(canvasRect.width, 0.0f);
	canvasRect.height = std::max(canvasRect.height, 0.0f);

	Rect channelSplitterRect = {
		outerRect.x + channelWidth - 3.0f * scale,
		toolbarRect.bottom(),
		6.0f * scale,
		outerRect.bottom() - toolbarRect.bottom() };

	f32 axisWidth = state->viewMode == TimelineViewMode::Curves ? timelineSettings.valueAxisWidth * scale : 0.0f;

	axisWidth = std::min(axisWidth, canvasRect.width * 0.6f);

	Rect plotRect = { canvasRect.x + axisWidth, canvasRect.y, canvasRect.width - axisWidth, canvasRect.height };

	plotRect.width = std::max(plotRect.width, 1.0f);

	f32 valueMin, valueMax;

	if (runtime.valueRangeModeInitialized && !state->autoValueRange && runtime.wasAutoValueRange)
		timelineComputeAutoValueRange(*state, state->valueMin, state->valueMax);

	runtime.wasAutoValueRange = state->autoValueRange;
	runtime.valueRangeModeInitialized = true;
	timelineComputeValueRange(*state, valueMin, valueMax);

	timelineClampChannelScroll(*state, listRect, timelineSettings);
	timelineClampView(*state, plotRect, timelineSettings);

	std::vector<TimelineChannelRow> channelRows;
	timelineBuildChannelRows(*state, channelRows);

	TimelineChannelScrollbar channelScrollbar = timelineGetChannelScrollbar(*state, timelineSettings, listRect, rowHeight);

	Font* font = ctx->theme->getFont("normal");

	if (!font && !ctx->theme->fonts.empty())
		font = &ctx->theme->fonts[0]->font;

	TimelineToolbarButton toolbarButtons[TimelineTbCount];
	u32 toolbarButtonCount = timelineBuildToolbar(font, toolbarRect, *state, toolbarButtons);

	Point mouse = ctx->mousePosition;
	bool mouseInTimeline = outerRect.contains(mouse) && ctx->hoveringThisWindow && ctx->isActiveLayer();
	InputEvent event = ctx->event;
	bool consumed = false;

	if (mouseInTimeline && !ctx->widget.captureId)
	{
		ctx->widget.hoveredId = timelineId;
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = outerRect;
	}

	i32 hoveredToolbarButton = -1;

	if (mouseInTimeline && (!ctx->widget.captureId || runtime.pressedToolbarButton >= 0))
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

	TimelineToolbarField toolbarFields[8];
	u32 toolbarFieldCount = 0;

	auto focusTimeline = [&]()
	{
		ctx->widget.focusedId = timelineId;
		ctx->widget.focused = true;
		ctx->focusChanged = true;
	};

	auto beginCapture = [&]()
	{
		focusTimeline();
		ctx->widget.captureId = timelineId;
		windowSetCapture();
	};

	auto endCapture = [&]()
	{
		ctx->widget.captureId = 0;
		windowReleaseCapture();
	};

	auto applyToolbarAction = [&](TimelineToolbarAction action)
	{
		switch (action)
		{
		case TimelineTbPlay:
			state->playing = !state->playing;

			if (state->playing && state->cursorTime >= state->endTime)
				state->cursorTime = state->startTime;

			changed = true;
			break;

		case TimelineTbKeys:
			if (state->viewMode != TimelineViewMode::Keyframes)
			{
				state->viewMode = TimelineViewMode::Keyframes;
				changed = true;
			}
			break;

		case TimelineTbCurves:
			if (state->viewMode != TimelineViewMode::Curves)
			{
				state->viewMode = TimelineViewMode::Curves;
				changed = true;
			}
			break;

		case TimelineTbAddKey:
		{
			i32 channelIndex = timelineTargetChannel(*state);

			if (channelIndex >= 0)
			{
				f32 values[4];
				f32 addTime = timelineSnapTime(*state, state->cursorTime);

				timelineEvaluateChannel(state->channels[channelIndex], addTime, values);
				timelineAddKey(*state, channelIndex, addTime, values, true);
				changed = true;
			}
			break;
		}

		case TimelineTbDeleteKey:
			if (timelineDeleteSelectedKeys(*state) > 0)
				changed = true;
			break;

		case TimelineTbSnap:
			state->snapToFrames = !state->snapToFrames;
			changed = true;
			break;

		case TimelineTbZoomOut:
			timelineZoom(*state, timelineSettings, plotRect, 1.0f / timelineSettings.zoomStep, plotRect.x + plotRect.width * 0.5f);
			timelineClampView(*state, plotRect, timelineSettings);
			changed = true;
			break;

		case TimelineTbZoomIn:
			timelineZoom(*state, timelineSettings, plotRect, timelineSettings.zoomStep, plotRect.x + plotRect.width * 0.5f);
			timelineClampView(*state, plotRect, timelineSettings);
			changed = true;
			break;

		case TimelineTbFit:
			timelineFitView(*state, timelineSettings, plotRect);
			timelineClampView(*state, plotRect, timelineSettings);
			changed = true;
			break;

		default:
			break;
		}
	};

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

		if (!consumed && mouseInTimeline && event.type == InputEvent::Type::MouseWheel)
		{
			if (canvasRect.contains(mouse) || rulerRect.contains(mouse))
			{
				f32 factor = event.mouse.wheel.y > 0.0f ? timelineSettings.zoomStep : 1.0f / timelineSettings.zoomStep;

				timelineZoom(*state, timelineSettings, plotRect, factor, mouse.x);

				if (!state->autoValueRange && state->viewMode == TimelineViewMode::Curves && plotRect.height > 0.0f)
				{
					f32 range = std::max(valueMax - valueMin, 0.0001f) / factor;
					f32 ratio = timelineClamp((plotRect.bottom() - mouse.y) / plotRect.height, 0.0f, 1.0f);
					f32 valueAtMouse = timelineYToValue(plotRect, mouse.y, valueMin, valueMax);

					range = std::max(range, 0.0001f);
					state->valueMin = valueAtMouse - ratio * range;
					state->valueMax = state->valueMin + range;
				}

				timelineClampView(*state, plotRect, timelineSettings);
			}
			else if (listRect.contains(mouse))
			{
				state->channelScroll -= event.mouse.wheel.y * timelineSettings.rowHeight * 0.75f;
				timelineClampChannelScroll(*state, listRect, timelineSettings);
			}

			consumed = true;
		}

		if (!consumed && mouseInTimeline && event.type == InputEvent::Type::MouseDown)
		{
			if (event.mouse.button == MouseButton::Middle
				&& (canvasRect.contains(mouse) || rulerRect.contains(mouse)))
			{
				runtime.dragType = TimelineWidgetState::DragType::Pan;
				runtime.dragStartMouse = mouse;
				runtime.panStartScrollTime = state->scrollTime;
				runtime.panStartValueMin = valueMin;
				runtime.panStartValueMax = valueMax;
				beginCapture();
				consumed = true;
			}
			else if (event.mouse.button == MouseButton::Left)
			{
				TimelineTangentHit tangentHit;

				if (state->viewMode == TimelineViewMode::Curves && canvasRect.contains(mouse))
					tangentHit = timelineFindTangentHandle(*state, timelineSettings, plotRect, valueMin, valueMax, mouse);

				if (tangentHit.channel >= 0)
				{
					TimelineChannel& channel = state->channels[tangentHit.channel];

					timelineMakeKeyTangentsManual(channel, tangentHit.key);
					runtime.dragType = tangentHit.outgoing
						? TimelineWidgetState::DragType::TangentOut
						: TimelineWidgetState::DragType::TangentIn;
					runtime.dragChannel = tangentHit.channel;
					runtime.dragKey = tangentHit.key;
					runtime.dragComponent = tangentHit.component;
					runtime.dragValueMin = valueMin;
					runtime.dragValueMax = valueMax;
					beginCapture();
					changed = true;
					consumed = true;
				}
				else if (channelSplitterRect.contains(mouse))
				{
					runtime.dragType = TimelineWidgetState::DragType::ChannelResize;
					beginCapture();
					consumed = true;
				}
				else if (channelScrollbar.track.contains(mouse))
				{
					if (channelScrollbar.maxScroll > 0.0f)
					{
						if (!channelScrollbar.thumb.contains(mouse))
						{
							f32 travel = std::max(channelScrollbar.track.height - channelScrollbar.thumb.height, 1.0f);
							f32 ratio = timelineClamp(
								(mouse.y - channelScrollbar.track.y - channelScrollbar.thumb.height * 0.5f) / travel,
								0.0f,
								1.0f);

							state->channelScroll = ratio * channelScrollbar.maxScroll;
						}

						runtime.dragType = TimelineWidgetState::DragType::ChannelScroll;
						runtime.dragStartMouse = mouse;
						runtime.dragStartChannelScroll = state->channelScroll;
						beginCapture();
						consumed = true;
					}
				}
				else if (listRect.contains(mouse))
				{
					f32 scrollOffset = state->channelScroll * scale;
					f32 localY = mouse.y - listRect.y + scrollOffset;
					i32 rowIndex = (i32)floorf(localY / std::max(rowHeight, 1.0f));

					if (rowIndex >= 0 && rowIndex < (i32)channelRows.size())
					{
						Rect rowRect = { listRect.x, listRect.y - scrollOffset + (f32)rowIndex * rowHeight, listRect.width, rowHeight };

						if (rowRect.contains(mouse))
						{
							const TimelineChannelRow& rowInfo = channelRows[rowIndex];
							TimelineChannel& channel = state->channels[rowInfo.channel];
							bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

							if (rowInfo.component < 0 && timelineChannelCheckboxRect(rowRect, scale).contains(mouse))
							{
								channel.enabled = !channel.enabled;
								changed = true;
							}
							else if (rowInfo.component < 0
								&& state->viewMode == TimelineViewMode::Curves
								&& timelineComponentCount(channel.type) > 1
								&& timelineChannelExpandRect(rowRect, scale).contains(mouse))
							{
								channel.expanded = !channel.expanded;
								changed = true;
							}
							else if (rowInfo.component < 0)
							{
								state->selectedChannel = rowInfo.channel;
								changed = true;

								if (controlDown)
								{
									channel.selected = !channel.selected;
									channel.selectedComponents = 0;
								}
								else
								{
									for (auto& otherChannel : state->channels)
									{
										otherChannel.selected = false;
										otherChannel.selectedComponents = 0;
									}

									channel.selected = true;
									timelineClearSelection(*state);
								}
							}
							else
							{
								state->selectedChannel = rowInfo.channel;
								changed = true;

								if (controlDown)
								{
									channel.selectedComponents ^= (1u << rowInfo.component);
									channel.selected = false;
								}
								else
								{
									for (auto& otherChannel : state->channels)
									{
										otherChannel.selected = false;
										otherChannel.selectedComponents = 0;
									}

									channel.selected = false;
									channel.selectedComponents = (1u << rowInfo.component);
									timelineClearSelection(*state);
								}
							}

							focusTimeline();
							consumed = true;
						}
					}
				}
				else if (rulerRect.contains(mouse))
				{
					state->cursorTime = timelineSnapTime(*state, timelineXToTime(*state, plotRect, mouse.x));
					runtime.dragType = TimelineWidgetState::DragType::Playhead;
					beginCapture();
					changed = true;
					consumed = true;
				}
				else if (canvasRect.contains(mouse))
				{
					f32 rowsTop = canvasRect.y - state->channelScroll * scale;
					TimelineKeyHit hit = timelineFindKeyUnderMouse(*state, timelineSettings, plotRect, rowsTop, rowHeight, valueMin, valueMax);

					if (hit.channel >= 0)
					{
						TimelineChannel& channel = state->channels[hit.channel];
						bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

						if (controlDown)
						{
							channel.keys[hit.key].selected = !channel.keys[hit.key].selected;
							state->selectedChannel = hit.channel;
							changed = true;
						}
						else if (!channel.keys[hit.key].selected)
						{
							timelineClearSelection(*state);
							channel.keys[hit.key].selected = true;
							state->selectedChannel = hit.channel;
							changed = true;
						}
						else
						{
							state->selectedChannel = hit.channel;
						}

						if (channel.keys[hit.key].selected)
						{
							runtime.dragType = TimelineWidgetState::DragType::Keys;
							runtime.dragChannel = hit.channel;
							runtime.dragComponent = hit.component;
							runtime.dragStartMouse = mouse;
							runtime.dragStartTime = timelineXToTime(*state, plotRect, mouse.x);
							runtime.dragValueMin = valueMin;
							runtime.dragValueMax = valueMax;
							runtime.dragStartValue = timelineYToValue(plotRect, mouse.y, valueMin, valueMax);
							runtime.dragStartEndTime = state->endTime;
							runtime.dragItems.clear();

							for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
							{
								TimelineChannel& selectedChannel = state->channels[channelIndex];

								if (!selectedChannel.enabled)
									continue;

								for (i32 keyIndex = 0; keyIndex < (i32)selectedChannel.keys.size(); keyIndex++)
								{
									TimelineKey& key = selectedChannel.keys[keyIndex];

									if (!key.selected)
										continue;

									TimelineKeyDragItem item;

									item.channel = channelIndex;
									item.key = keyIndex;
									item.time = key.time;

									for (i32 i = 0; i < 4; i++)
										item.value[i] = key.value[i];

									runtime.dragItems.push_back(item);
								}
							}

							runtime.rippleItems.clear();

							if (state->ripple)
							{
								for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
								{
									TimelineChannel& rippleChannel = state->channels[channelIndex];
									f32 minSelectedTime = FLT_MAX;
									f32 maxSelectedTime = -FLT_MAX;
									bool hasSelectedKey = false;

									if (!rippleChannel.enabled)
										continue;

									for (auto& key : rippleChannel.keys)
									{
										if (key.selected)
										{
											hasSelectedKey = true;
											minSelectedTime = std::min(minSelectedTime, key.time);
											maxSelectedTime = std::max(maxSelectedTime, key.time);
										}
									}

									if (!hasSelectedKey)
										continue;

									for (i32 keyIndex = 0; keyIndex < (i32)rippleChannel.keys.size(); keyIndex++)
									{
										TimelineKey& key = rippleChannel.keys[keyIndex];

										if (key.selected)
											continue;

										bool leftSide = key.time < minSelectedTime;
										bool rightSide = key.time > maxSelectedTime;

										if (!leftSide && !rightSide)
											continue;

										TimelineKeyDragItem item;

										item.channel = channelIndex;
										item.key = keyIndex;
										item.time = key.time;
										item.leftSide = leftSide;
										runtime.rippleItems.push_back(item);
									}
								}
							}

							beginCapture();
						}

						consumed = true;
					}
					else if (event.mouse.clickCount >= 2)
					{
						i32 channelIndex = -1;
						i32 component = 0;

						if (state->viewMode == TimelineViewMode::Keyframes)
						{
							i32 row = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

							if (row >= 0 && row < (i32)state->channels.size() && state->channels[row].enabled)
								channelIndex = row;
						}
						else
						{
							channelIndex = timelineTargetChannel(*state);

							if (channelIndex >= 0)
							{
								TimelineChannel& channel = state->channels[channelIndex];
								i32 components = timelineComponentCount(channel.type);
								f32 bestDistance = FLT_MAX;

								for (i32 i = 0; i < components; i++)
								{
									f32 value = timelineEvaluateComponent(channel, i, timelineXToTime(*state, plotRect, mouse.x));
									f32 y = timelineValueToY(plotRect, value, valueMin, valueMax);
									f32 distance = fabsf(mouse.y - y);

									if (distance < bestDistance)
									{
										bestDistance = distance;
										component = i;
									}
								}
							}
						}

						if (channelIndex >= 0)
						{
							f32 values[4];
							f32 addTime = timelineSnapTime(*state, timelineXToTime(*state, plotRect, mouse.x));

							timelineEvaluateChannel(state->channels[channelIndex], addTime, values);

							if (state->viewMode == TimelineViewMode::Curves)
								values[component] = timelineYToValue(plotRect, mouse.y, valueMin, valueMax);

							timelineAddKey(*state, channelIndex, addTime, values, true);
							changed = true;
						}

						consumed = true;
					}
					else
					{
						f32 playheadX = timelineTimeToX(*state, plotRect, state->cursorTime);

						if (fabsf(mouse.x - playheadX) <= 5.0f * scale)
						{
							runtime.dragType = TimelineWidgetState::DragType::Playhead;
							beginCapture();
							changed = true;
						}
						else
						{
							bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

							if (!controlDown)
							{
								timelineClearSelection(*state);
								changed = true;
							}

							runtime.dragType = TimelineWidgetState::DragType::BoxSelect;
							runtime.boxStart = mouse;
							runtime.boxEnd = mouse;
							beginCapture();
						}

						consumed = true;
					}
				}
			}
		}

		if (event.type == InputEvent::Type::MouseMove && runtime.dragType != TimelineWidgetState::DragType::None)
		{
			switch (runtime.dragType)
			{
			case TimelineWidgetState::DragType::Playhead:
				state->cursorTime = timelineSnapTime(*state, timelineXToTime(*state, plotRect, mouse.x));
				changed = true;
				consumed = true;
				break;

			case TimelineWidgetState::DragType::Pan:
				state->scrollTime = runtime.panStartScrollTime - (mouse.x - runtime.dragStartMouse.x) / (state->pixelsPerSecond * scale);

				if (!state->autoValueRange && state->viewMode == TimelineViewMode::Curves && plotRect.height > 0.0f)
				{
					f32 range = runtime.panStartValueMax - runtime.panStartValueMin;
					f32 valueDelta = (mouse.y - runtime.dragStartMouse.y) / plotRect.height * range;

					state->valueMin = runtime.panStartValueMin + valueDelta;
					state->valueMax = runtime.panStartValueMax + valueDelta;
				}

				timelineClampView(*state, plotRect, timelineSettings);
				changed = true;
				consumed = true;
				break;

			case TimelineWidgetState::DragType::ChannelResize:
			{
				f32 minWidth = 80.0f;
				f32 maxWidth = std::max(minWidth, outerRect.width / scale - std::min(120.0f, outerRect.width / scale * 0.5f));

				runtime.channelListWidth = timelineClamp((mouse.x - outerRect.x) / scale, minWidth, maxWidth);
				consumed = true;
				break;
			}

			case TimelineWidgetState::DragType::ChannelScroll:
			{
				TimelineChannelScrollbar scrollbar = timelineGetChannelScrollbar(*state, timelineSettings, listRect, rowHeight);
				f32 travel = std::max(scrollbar.track.height - scrollbar.thumb.height, 1.0f);

				if (scrollbar.maxScroll > 0.0f)
					state->channelScroll = timelineClamp(
						runtime.dragStartChannelScroll + (mouse.y - runtime.dragStartMouse.y) / travel * scrollbar.maxScroll,
						0.0f,
						scrollbar.maxScroll);

				consumed = true;
				break;
			}

			case TimelineWidgetState::DragType::TangentIn:
			case TimelineWidgetState::DragType::TangentOut:
			{
				if (runtime.dragChannel < 0 || runtime.dragChannel >= (i32)state->channels.size())
					break;

				TimelineChannel& channel = state->channels[runtime.dragChannel];

				if (runtime.dragKey < 0 || runtime.dragKey >= (i32)channel.keys.size()
					|| runtime.dragComponent < 0 || runtime.dragComponent >= 4)
					break;

				TimelineKey& key = channel.keys[runtime.dragKey];
				f32 mouseTime = timelineXToTime(*state, plotRect, mouse.x);
				f32 mouseValue = timelineYToValue(plotRect, mouse.y, runtime.dragValueMin, runtime.dragValueMax);
				bool outgoing = runtime.dragType == TimelineWidgetState::DragType::TangentOut;
				f32 duration = outgoing ? mouseTime - key.time : key.time - mouseTime;
				f32 valueDelta = outgoing ? mouseValue - key.value[runtime.dragComponent] : key.value[runtime.dragComponent] - mouseValue;

				if (fabsf(duration) > 0.00001f)
				{
					f32 tangent = valueDelta / duration;

					if (outgoing)
						key.outTangent[runtime.dragComponent] = tangent;
					else
						key.inTangent[runtime.dragComponent] = tangent;

					key.tangentAuto = false;
					changed = true;
				}

				consumed = true;
				break;
			}

			case TimelineWidgetState::DragType::Keys:
			{
				f32 newTime = timelineXToTime(*state, plotRect, mouse.x);
				f32 timeDelta = newTime - runtime.dragStartTime;
				f32 valueDelta = 0.0f;

				if (state->viewMode == TimelineViewMode::Curves && runtime.dragComponent >= 0)
					valueDelta = timelineYToValue(plotRect, mouse.y, runtime.dragValueMin, runtime.dragValueMax) - runtime.dragStartValue;

				bool moved = false;
				f32 newEndTime = runtime.dragStartEndTime;

				for (auto& item : runtime.dragItems)
				{
					if (item.channel < 0 || item.channel >= (i32)state->channels.size())
						continue;

					TimelineChannel& channel = state->channels[item.channel];

					if (!channel.enabled || item.key < 0 || item.key >= (i32)channel.keys.size())
						continue;

					TimelineKey& key = channel.keys[item.key];
					f32 time = state->extendEndTime
						? timelineSnapTimeNoMax(*state, item.time + timeDelta)
						: timelineSnapTime(*state, item.time + timeDelta);

					if (state->extendEndTime)
						newEndTime = std::max(newEndTime, time);

					if (time != key.time)
					{
						key.time = time;
						moved = true;
					}

					if (valueDelta != 0.0f
						&& item.channel == runtime.dragChannel
						&& runtime.dragComponent >= 0
						&& runtime.dragComponent < timelineComponentCount(channel.type))
					{
						f32 value = timelineQuantizeComponent(channel.type, item.value[runtime.dragComponent] + valueDelta, runtime.dragComponent);

						if (value != key.value[runtime.dragComponent])
						{
							key.value[runtime.dragComponent] = value;
							moved = true;
						}
					}
				}

				for (auto& item : runtime.rippleItems)
				{
					if (item.channel < 0 || item.channel >= (i32)state->channels.size())
						continue;

					TimelineChannel& channel = state->channels[item.channel];

					if (item.key < 0 || item.key >= (i32)channel.keys.size())
						continue;

					TimelineKey& key = channel.keys[item.key];
					f32 itemDelta = item.leftSide ? std::min(timeDelta, 0.0f) : std::max(timeDelta, 0.0f);
					f32 time = state->extendEndTime
						? timelineSnapTimeNoMax(*state, item.time + itemDelta)
						: timelineSnapTime(*state, item.time + itemDelta);

					if (state->extendEndTime)
						newEndTime = std::max(newEndTime, time);

					if (time != key.time)
					{
						key.time = time;
						moved = true;
					}
				}

				if (state->extendEndTime && newEndTime != state->endTime)
				{
					state->endTime = newEndTime;
					moved = true;
				}

				if (moved)
				{
					timelineReorderDraggedKeys(runtime, state->channels);
					changed = true;
					ctx->widget.changeEnded = true;
				}

				consumed = true;
				break;
			}

			case TimelineWidgetState::DragType::BoxSelect:
				runtime.boxEnd = mouse;
				consumed = true;
				break;

			default:
				break;
			}
		}

		if (event.type == InputEvent::Type::MouseUp && runtime.dragType != TimelineWidgetState::DragType::None)
		{
			if (runtime.dragType == TimelineWidgetState::DragType::Keys)
			{
				for (auto& channel : state->channels)
					timelineSortKeys(channel);
			}
			else if (runtime.dragType == TimelineWidgetState::DragType::BoxSelect)
			{
				Rect selectionRect = {
					std::min(runtime.boxStart.x, runtime.boxEnd.x),
					std::min(runtime.boxStart.y, runtime.boxEnd.y),
					0,
					0
				};

				selectionRect.width = fabsf(runtime.boxEnd.x - runtime.boxStart.x);
				selectionRect.height = fabsf(runtime.boxEnd.y - runtime.boxStart.y);
				selectionRect = selectionRect.clipInside(canvasRect);

				if (selectionRect.width > 4.0f * scale || selectionRect.height > 4.0f * scale)
				{
					f32 rowsTop = canvasRect.y - state->channelScroll * scale;

					for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
					{
						TimelineChannel& channel = state->channels[channelIndex];

						if (!channel.enabled)
							continue;

						for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
						{
							if (timelineKeyInsideRect(*state, plotRect, selectionRect, rowsTop, rowHeight, valueMin, valueMax, channelIndex, keyIndex))
							{
								channel.keys[keyIndex].selected = true;
								state->selectedChannel = channelIndex;
							}
						}
					}

					changed = true;
				}
			}

			runtime.dragType = TimelineWidgetState::DragType::None;
			runtime.dragChannel = -1;
			runtime.dragKey = -1;
			runtime.dragComponent = -1;
			runtime.dragItems.clear();
			runtime.rippleItems.clear();
			endCapture();
			consumed = true;
		}

		if (!consumed
			&& event.type == InputEvent::Type::Key
			&& event.key.down
			&& !ctx->textInput.id
			&& (ctx->widget.focusedId == timelineId
				|| (event.key.code == KeyCode::Space && mouseInTimeline)))
		{
			bool controlDown = has(event.key.modifiers, KeyModifiers::Control);
			bool shiftDown = has(event.key.modifiers, KeyModifiers::Shift);

			switch (event.key.code)
			{
			case KeyCode::Delete:
			case KeyCode::Backspace:
				if (timelineDeleteSelectedKeys(*state) > 0)
					changed = true;

				consumed = true;
				break;

			case KeyCode::A:
				if (controlDown)
				{
					timelineSelectAll(*state);
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

				timelineNudgeSelectedKeys(*state, delta);
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

			case KeyCode::Esc:
				timelineClearSelection(*state);
				runtime.dragType = TimelineWidgetState::DragType::None;
				runtime.dragItems.clear();
				runtime.rippleItems.clear();
				endCapture();
				changed = true;
				consumed = true;
				break;

			case KeyCode::F:
				timelineFitView(*state, timelineSettings, plotRect);
				timelineClampView(*state, plotRect, timelineSettings);
				changed = true;
				consumed = true;
				break;

			default:
				break;
			}
		}
	}

	if (consumed)
	{
		if (event.type == InputEvent::Type::MouseDown)
		{
			ctx->alreadyClickedOnSomething = true;

			if (ctx->popupIndex > 0)
				ctx->popupStack[ctx->popupIndex - 1].alreadyClickedOnSomething = true;
		}

		inputEventCancel();
	}

	// update the geometry for a mode change, the toolbar active states and the curves value range after input
	axisWidth = state->viewMode == TimelineViewMode::Curves ? timelineSettings.valueAxisWidth * scale : 0.0f;
	axisWidth = std::min(axisWidth, canvasRect.width * 0.6f);
	plotRect = { canvasRect.x + axisWidth, canvasRect.y, canvasRect.width - axisWidth, canvasRect.height };
	plotRect.width = std::max(plotRect.width, 1.0f);

	timelineClampView(*state, plotRect, timelineSettings);

	toolbarButtonCount = timelineBuildToolbar(font, toolbarRect, *state, toolbarButtons);
	toolbarFieldCount = timelineBuildToolbarFields(toolbarRect, toolbarButtons, toolbarButtonCount, timelineSettings, *state, toolbarFields, 8);

	if (runtime.dragType == TimelineWidgetState::DragType::Keys && state->viewMode == TimelineViewMode::Curves)
	{
		valueMin = runtime.dragValueMin;
		valueMax = runtime.dragValueMax;
	}
	else
	{
		timelineComputeValueRange(*state, valueMin, valueMax);
	}

	// drawing
	ctx->renderer.pushClipRect(outerRect);

	Color backgroundColor = timelineSettings.backgroundColor;
	Color headerColor = timelineSettings.headerColor;
	Color channelListColor = timelineSettings.channelListColor;
	Color textColor = timelineSettings.textColor;
	Color dimTextColor = timelineSettings.dimTextColor;

	if (disabled)
	{
		backgroundColor.a *= 0.7f;
		headerColor.a *= 0.7f;
		channelListColor.a *= 0.7f;
		textColor.a *= 0.6f;
		dimTextColor.a *= 0.6f;
	}

	ctx->renderer.cmdSetColor(backgroundColor);
	ctx->renderer.cmdDrawFilledRectangle(outerRect);

	ctx->renderer.cmdSetColor(headerColor);
	ctx->renderer.cmdDrawFilledRectangle(toolbarRect);
	ctx->renderer.cmdDrawFilledRectangle(listHeaderRect);
	ctx->renderer.cmdDrawFilledRectangle(rulerRect);

	ctx->renderer.cmdSetColor(channelListColor);
	ctx->renderer.cmdDrawFilledRectangle(listRect);

	ctx->renderer.cmdSetColor(backgroundColor);
	ctx->renderer.cmdDrawFilledRectangle(canvasRect);

	if (state->viewMode == TimelineViewMode::Curves && axisWidth > 0.0f)
	{
		ctx->renderer.cmdSetColor(channelListColor);
		ctx->renderer.cmdDrawFilledRectangle({ canvasRect.x, canvasRect.y, axisWidth, canvasRect.height });
	}

	ctx->renderer.pushClipRect(toolbarRect);

	for (u32 i = 0; i < toolbarButtonCount; i++)
	{
		bool hovered = (i32)i == hoveredToolbarButton;
		bool pressed = (i32)i == runtime.pressedToolbarButton;

		timelineDrawButton(font, toolbarButtons[i].rect, toolbarButtons[i].label, toolbarButtons[i].active, hovered, pressed, timelineSettings.toolbarToggleColor);
	}

	if (toolbarFieldCount == 0 && toolbarRect.width > 520.0f * scale)
	{
		i32 totalKeys = 0;

		for (auto& channel : state->channels)
			totalKeys += (i32)channel.keys.size();

		char status[160];

		snprintf(status, sizeof(status), "t %.3fs   keys %d   zoom %.0f%%", state->cursorTime, totalKeys, state->pixelsPerSecond / timelineSettings.defaultPixelsPerSecond * 100.0f);

		Rect statusRect = { toolbarRect.right() - 330.0f * scale, toolbarRect.y, 320.0f * scale, toolbarRect.height };

		timelineDrawText(font, status, statusRect, dimTextColor, HAlignType::Right);
	}

	ctx->renderer.popClipRect();

	timelineDrawText(font, "Channels", { listHeaderRect.x + 8.0f * scale, listHeaderRect.y, std::max(listHeaderRect.width - 64.0f * scale, 0.0f), listHeaderRect.height }, textColor, HAlignType::Left);

	{
		char countText[32];

		snprintf(countText, sizeof(countText), "%u", (u32)state->channels.size());
		timelineDrawText(font, countText, { listHeaderRect.right() - 56.0f * scale, listHeaderRect.y, 48.0f * scale, listHeaderRect.height }, dimTextColor, HAlignType::Right);
	}

	f32 listScrollOffset = state->channelScroll * scale;
	f32 rowsTop = listRect.y - listScrollOffset;

	ctx->renderer.pushClipRect(listRect);

	for (u32 rowIndex = 0; rowIndex < channelRows.size(); rowIndex++)
	{
		const TimelineChannelRow& rowInfo = channelRows[rowIndex];
		TimelineChannel& channel = state->channels[rowInfo.channel];
		Rect rowRect = { listRect.x, rowsTop + (f32)rowIndex * rowHeight, listRect.width, rowHeight };

		if (rowRect.bottom() < listRect.y || rowRect.y > listRect.bottom())
			continue;

		bool isParent = rowInfo.component < 0;
		bool visible = isParent ? timelineChannelHasVisibleComponent(channel) : timelineComponentVisible(channel, rowInfo.component);

		if (visible)
		{
			ctx->renderer.cmdSetColor(timelineSettings.rowSelectedColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}
		else if (rowIndex % 2 != 0)
		{
			ctx->renderer.cmdSetColor(timelineSettings.rowAlternateColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}

		if (!channel.enabled)
		{
			Color overlay = timelineSettings.disabledOverlayColor;

			overlay.a *= 0.5f;
			ctx->renderer.cmdSetColor(overlay);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}

		Color channelColor = timelineChannelColor(rowInfo.channel);
		Color nameColor = channel.enabled ? textColor : dimTextColor;

		if (isParent)
		{
			Rect checkboxRect = timelineChannelCheckboxRect(rowRect, scale);
			Rect badgeRect = timelineChannelBadgeRect(rowRect, scale);
			Rect nameRect = { badgeRect.right() + 6.0f * scale, rowRect.y, rowRect.width - (badgeRect.right() - rowRect.x) - 56.0f * scale, rowRect.height };

			nameRect.width = std::max(nameRect.width, 0.0f);

			if (!channel.enabled)
				channelColor.a *= 0.45f;

			timelineDrawCheckbox(checkboxRect, channel.enabled, disabled);

			if (state->viewMode == TimelineViewMode::Curves && timelineComponentCount(channel.type) > 1)
			{
				Rect expandRect = timelineChannelExpandRect(rowRect, scale);
				Point center = { expandRect.x + expandRect.width * 0.5f, expandRect.y + expandRect.height * 0.5f };
				f32 s = 3.0f * scale;
				Point p1, p2, p3;

				if (channel.expanded)
				{
					p1 = { center.x - s, center.y - s * 0.5f };
					p2 = { center.x + s, center.y - s * 0.5f };
					p3 = { center.x, center.y + s * 0.5f };
				}
				else
				{
					p1 = { center.x - s * 0.5f, center.y - s };
					p2 = { center.x - s * 0.5f, center.y + s };
					p3 = { center.x + s * 0.5f, center.y };
				}

				ctx->renderer.cmdDrawSolidTriangle(p1, p2, p3, nameColor, nameColor, nameColor);
			}

			Color badgeBackground = { channelColor.r * 0.22f, channelColor.g * 0.22f, channelColor.b * 0.22f, channel.enabled ? 0.9f : 0.4f };
			ctx->renderer.cmdSetColor(badgeBackground);
			ctx->renderer.cmdDrawFilledRectangle(badgeRect);
			timelineDrawText(font, timelineTypeBadge(channel.type), badgeRect, channelColor, HAlignType::Center, false);

			const char* channelName = channel.name.empty() ? "Unnamed" : channel.name.c_str();

			timelineDrawText(font, channelName, nameRect, nameColor, HAlignType::Left);

			char keyCount[32];

			snprintf(keyCount, sizeof(keyCount), "%u", (u32)channel.keys.size());
			timelineDrawText(font, keyCount, { rowRect.right() - 46.0f * scale, rowRect.y, 28.0f * scale, rowRect.height }, dimTextColor, HAlignType::Right);
		}
		else
		{
			i32 component = rowInfo.component;
			const char* axis = timelineComponentLabel(channel.type, component);
			char label[128];

			snprintf(label, sizeof(label), "%s.%s", channel.name.empty() ? "Unnamed" : channel.name.c_str(), axis);

			Color componentColor = timelineChannelColor(rowInfo.channel, component);
			Point dot = { rowRect.x + 64.0f * scale, rowRect.y + rowRect.height * 0.5f };

			timelineDrawDiamond(dot, 3.5f * scale, componentColor);
			timelineDrawText(font, label, { rowRect.x + 74.0f * scale, rowRect.y, rowRect.width - 74.0f * scale - 110.0f * scale, rowRect.height }, nameColor, HAlignType::Left);

			char valueText[32];
			f32 value = timelineEvaluateComponent(channel, component, state->cursorTime);

			if (channel.type == TimelineKeyType::Int)
				snprintf(valueText, sizeof(valueText), "%.0f", value);
			else
				snprintf(valueText, sizeof(valueText), "%.3f", value);

			timelineDrawText(font, valueText, { rowRect.right() - 104.0f * scale, rowRect.y, 56.0f * scale, rowRect.height }, dimTextColor, HAlignType::Right);
		}

		{
			Point dot = { rowRect.right() - 14.0f * scale, rowRect.y + rowRect.height * 0.5f };
			Color dotColor = timelineChannelColor(rowInfo.channel, rowInfo.component);
			bool selected = isParent
				? timelineChannelHasVisibleComponent(channel)
				: timelineComponentVisible(channel, rowInfo.component);

			if (!selected)
			{
				dotColor = timelineSettings.dimTextColor;
				dotColor.a = 0.35f;
			}

			timelineDrawDiamond(dot, 4.0f * scale, dotColor);
		}

		ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
	}

	ctx->renderer.popClipRect();

	// channel list scrollbar
	{
		TimelineChannelScrollbar scrollbar = timelineGetChannelScrollbar(*state, timelineSettings, listRect, rowHeight);

		if (scrollbar.maxScroll > 0.0f)
		{
			ctx->renderer.cmdSetColor(Color{ timelineSettings.textColor.r, timelineSettings.textColor.g, timelineSettings.textColor.b, 0.1f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.track);
			ctx->renderer.cmdSetColor(Color{ timelineSettings.textColor.r, timelineSettings.textColor.g, timelineSettings.textColor.b, 0.4f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.thumb);
		}
	}

	// keyframe row backgrounds
	if (state->viewMode == TimelineViewMode::Keyframes)
	{
		ctx->renderer.pushClipRect(canvasRect);

		for (i32 i = 0; i < (i32)state->channels.size(); i++)
		{
			Rect rowRect = { canvasRect.x, rowsTop + (f32)i * rowHeight, canvasRect.width, rowHeight };

			if (rowRect.bottom() < canvasRect.y || rowRect.y > canvasRect.bottom())
				continue;

			if (i == state->selectedChannel)
			{
				ctx->renderer.cmdSetColor(timelineSettings.rowSelectedColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}
			else if (i % 2 != 0)
			{
				ctx->renderer.cmdSetColor(timelineSettings.rowAlternateColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}

			ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.gridColor, scale));
			ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
		}

		ctx->renderer.popClipRect();
	}

	// time grid and ruler
	f32 pixelsPerSecond = state->pixelsPerSecond * scale;
	f32 timeLeft = state->scrollTime;
	f32 timeRight = timeLeft + plotRect.width / std::max(pixelsPerSecond, 0.0001f);
	f32 majorStep = timelineNiceStep(80.0f * scale / std::max(pixelsPerSecond, 0.0001f));
	f32 minorStep = majorStep / 5.0f;
	i32 decimals = (i32)ceilf(-log10f(majorStep));

	decimals = (i32)timelineClamp((f32)decimals, 0.0f, 4.0f);

	ctx->renderer.pushClipRect(canvasRect);

	i32 minorStart = (i32)floorf(timeLeft / minorStep);
	i32 minorEnd = (i32)ceilf(timeRight / minorStep);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		f32 time = (f32)i * minorStep;
		f32 x = timelineTimeToX(*state, plotRect, time);

		if (x < plotRect.x - 1.0f || x > plotRect.right() + 1.0f)
			continue;

		bool major = (i % 5) == 0;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? timelineSettings.gridStrongColor : timelineSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, canvasRect.y }, { x, canvasRect.bottom() });
	}

	ctx->renderer.popClipRect();

	// value grid in curves mode
	if (state->viewMode == TimelineViewMode::Curves)
	{
		ctx->renderer.pushClipRect(canvasRect);

		f32 valueRange = valueMax - valueMin;
		f32 valueStep = timelineNiceStep(valueRange / std::max(2.0f, floorf(plotRect.height / (34.0f * scale))));
		i32 valueDecimals = (i32)ceilf(-log10f(valueStep));

		valueDecimals = (i32)timelineClamp((f32)valueDecimals, 0.0f, 4.0f);

		i32 valueStart = (i32)floorf(valueMin / valueStep);
		i32 valueEnd = (i32)ceilf(valueMax / valueStep);

		for (i32 i = valueStart; i <= valueEnd; i++)
		{
			f32 value = (f32)i * valueStep;
			f32 y = timelineValueToY(plotRect, value, valueMin, valueMax);

			if (y < plotRect.y - 1.0f || y > plotRect.bottom() + 1.0f)
				continue;

			Color gridColor = fabsf(value) < valueStep * 0.01f ? timelineSettings.gridStrongColor : timelineSettings.gridColor;

			ctx->renderer.cmdSetLineStyle(LineStyle(gridColor, scale));
			ctx->renderer.cmdDrawLine({ plotRect.x, y }, { plotRect.right(), y });

			char valueText[32];

			snprintf(valueText, sizeof(valueText), "%.*f", valueDecimals, value);
			timelineDrawText(font, valueText, { canvasRect.x + 2.0f * scale, y - 8.0f * scale, axisWidth - 8.0f * scale, 16.0f * scale }, dimTextColor, HAlignType::Right);
		}

		ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.gridStrongColor, scale));
		ctx->renderer.cmdDrawLine({ plotRect.x, canvasRect.y }, { plotRect.x, canvasRect.bottom() });

		ctx->renderer.popClipRect();
	}

	// ruler ticks and labels
	ctx->renderer.pushClipRect(rulerRect);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		bool major = (i % 5) == 0;
		f32 time = (f32)i * minorStep;
		f32 x = timelineTimeToX(*state, plotRect, time);

		if (x < plotRect.x - 1.0f || x > plotRect.right() + 1.0f)
			continue;

		f32 tickLength = (major ? 6.0f : 3.0f) * scale;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? timelineSettings.gridStrongColor : timelineSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, rulerRect.bottom() - tickLength }, { x, rulerRect.bottom() });

		if (major)
		{
			char timeText[32];

			snprintf(timeText, sizeof(timeText), "%.*f", decimals, time);
			timelineDrawText(font, timeText, { x + 3.0f * scale, rulerRect.y, 70.0f * scale, rulerRect.height - 4.0f * scale }, dimTextColor, HAlignType::Left);
		}
	}

	ctx->renderer.popClipRect();

	TimelineTangentHit hoveredTangent;

	if (!disabled && state->viewMode == TimelineViewMode::Curves && mouseInTimeline)
		hoveredTangent = timelineFindTangentHandle(*state, timelineSettings, plotRect, valueMin, valueMax, mouse);

	// curves and keys
	if (state->viewMode == TimelineViewMode::Curves)
	{
		ctx->renderer.pushClipRect(plotRect);

		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			TimelineChannel& channel = state->channels[channelIndex];

			if (!channel.enabled || channel.keys.empty())
				continue;

			i32 components = timelineComponentCount(channel.type);

			for (i32 component = 0; component < components; component++)
			{
				if (!timelineComponentVisible(channel, component))
					continue;

				std::vector<Point> points;

				points.reserve(channel.keys.size() + 2);

				if (channel.type == TimelineKeyType::Bool || channel.type == TimelineKeyType::Int)
				{
					f32 previousValue = timelineQuantizeComponent(channel.type, timelineEvaluateComponent(channel, component, timeLeft), component);

					points.push_back({ plotRect.x, timelineValueToY(plotRect, previousValue, valueMin, valueMax) });

					for (auto& key : channel.keys)
					{
						if (key.time < timeLeft || key.time > timeRight)
							continue;

						f32 value = timelineQuantizeComponent(channel.type, key.value[component], component);

						// a key with the same value as the previous one adds no step
						if (value == previousValue)
							continue;

						f32 x = timelineTimeToX(*state, plotRect, key.time);

						points.push_back({ x, timelineValueToY(plotRect, previousValue, valueMin, valueMax) });
						previousValue = value;
						points.push_back({ x, timelineValueToY(plotRect, previousValue, valueMin, valueMax) });
					}

					points.push_back({ plotRect.right(), timelineValueToY(plotRect, previousValue, valueMin, valueMax) });
				}
				else
				{
					// sample the hermite curve so its tangents are visible
					i32 sampleCount = std::max(2, (i32)ceilf(plotRect.width / (3.0f * scale)));

					for (i32 sample = 0; sample <= sampleCount; sample++)
					{
						f32 ratio = (f32)sample / (f32)sampleCount;
						f32 sampleTime = timeLeft + (timeRight - timeLeft) * ratio;
						f32 value = timelineEvaluateComponent(channel, component, sampleTime);

						points.push_back({ plotRect.x + plotRect.width * ratio, timelineValueToY(plotRect, value, valueMin, valueMax) });
					}
				}

				if (points.size() < 2)
					continue;

				Color color = timelineChannelColor(channelIndex, component);

				if (channelIndex == state->selectedChannel)
					color.a = 1.0f;
				else
					color.a = 0.85f;

				f32 lineWidth = timelineSettings.curveLineWidth * scale;

				ctx->renderer.cmdSetLineStyle(LineStyle(color, lineWidth));
				ctx->renderer.cmdDrawPolyLine(points.data(), (u32)points.size(), false);
			}
		}

		// tangent handles of the selected keys
		f32 handlePixels = timelineSettings.tangentHandleLength * scale;

		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			TimelineChannel& channel = state->channels[channelIndex];

			if (!channel.enabled || channel.keys.empty()
				|| channel.type == TimelineKeyType::Bool || channel.type == TimelineKeyType::Int)
				continue;

			i32 components = timelineComponentCount(channel.type);

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				TimelineKey& key = channel.keys[keyIndex];

				if (!key.selected)
					continue;

				Point keyPoint = {
					timelineTimeToX(*state, plotRect, key.time),
					timelineValueToY(plotRect, key.value[0], valueMin, valueMax) };

				for (i32 component = 0; component < components; component++)
				{
					if (!timelineComponentVisible(channel, component))
						continue;

					Color color = timelineChannelColor(channelIndex, component);
					Color lineColor = color;
					lineColor.a = 0.55f;
					Point componentPoint = {
						keyPoint.x,
						timelineValueToY(plotRect, key.value[component], valueMin, valueMax) };

					for (i32 side = 0; side < 2; side++)
					{
						bool outgoing = side == 0;
						Point handle = timelineTangentHandlePosition(*state, channel, keyIndex, component, outgoing, plotRect, valueMin, valueMax, handlePixels);
						bool hovered = hoveredTangent.channel == channelIndex
							&& hoveredTangent.key == keyIndex
							&& hoveredTangent.component == component
							&& hoveredTangent.outgoing == outgoing;

						ctx->renderer.cmdSetLineStyle(LineStyle(hovered ? color : lineColor, (hovered ? 1.5f : 1.0f) * scale));
						ctx->renderer.cmdDrawLine(componentPoint, handle);

						if (hovered)
							timelineDrawDiamond(handle, (timelineSettings.tangentHandleRadius + 3.0f) * scale, { 1, 1, 1, 0.45f });

						timelineDrawDiamond(handle, timelineSettings.tangentHandleRadius * scale * (hovered ? 1.25f : 1.0f), color);
					}
				}
			}
		}

		ctx->renderer.popClipRect();
	}

	// hovered key
	f32 rowsTopForHitTest = canvasRect.y - state->channelScroll * scale;
	TimelineKeyHit hoveredKey = mouseInTimeline && canvasRect.contains(mouse)
		? timelineFindKeyUnderMouse(*state, timelineSettings, plotRect, rowsTopForHitTest, rowHeight, valueMin, valueMax)
		: TimelineKeyHit();

	// keys
	ctx->renderer.pushClipRect(canvasRect);

	if (state->viewMode == TimelineViewMode::Keyframes)
	{
		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			TimelineChannel& channel = state->channels[channelIndex];

			if (!channel.enabled)
				continue;

			f32 centerY = rowsTop + (f32)channelIndex * rowHeight + rowHeight * 0.5f;
			Color channelColor = timelineChannelColor(channelIndex);

			if (channel.keys.size() > 1)
			{
				Color lineColor = channelColor;

				ctx->renderer.cmdSetLineStyle(LineStyle(lineColor, timelineSettings.keyframeLineWidth * scale));
				ctx->renderer.cmdDrawLine(
					{ timelineTimeToX(*state, plotRect, channel.keys.front().time), centerY },
					{ timelineTimeToX(*state, plotRect, channel.keys.back().time), centerY });
			}

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				TimelineKey& key = channel.keys[keyIndex];
				f32 x = timelineTimeToX(*state, plotRect, key.time);

				if (x < plotRect.x - timelineSettings.keySize * scale || x > plotRect.right() + timelineSettings.keySize * scale)
					continue;

				bool hovered = hoveredKey.channel == channelIndex && hoveredKey.key == keyIndex;
				f32 radius = timelineSettings.keySize * scale;

				if (hovered)
					timelineDrawDiamond({ x, centerY }, radius * 1.9f, { 1, 1, 1, 0.35f });

				if (key.selected)
				{
					timelineDrawDiamond({ x, centerY }, radius * 1.55f, timelineSettings.keySelectedColor);
					timelineDrawDiamond({ x, centerY }, radius, channelColor);
				}
				else
				{
					timelineDrawDiamond({ x, centerY }, radius, channelColor);
				}

				if (key.tag[0])
					timelineDrawKeyTag(font, key.tag, x + radius + 4.0f * scale, centerY, timelineSettings, scale);
			}
		}
	}
	else
	{
		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			TimelineChannel& channel = state->channels[channelIndex];

			if (!channel.enabled)
				continue;

			i32 components = timelineComponentCount(channel.type);

			for (i32 component = 0; component < components; component++)
			{
				if (!timelineComponentVisible(channel, component))
					continue;

				Color componentColor = timelineChannelColor(channelIndex, component);

				for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
				{
					TimelineKey& key = channel.keys[keyIndex];
					f32 x = timelineTimeToX(*state, plotRect, key.time);

					if (x < plotRect.x - timelineSettings.keySize * scale || x > plotRect.right() + timelineSettings.keySize * scale)
						continue;

					f32 y = timelineValueToY(plotRect, key.value[component], valueMin, valueMax);

					if (y < plotRect.y - timelineSettings.keySize * scale || y > plotRect.bottom() + timelineSettings.keySize * scale)
						continue;

					bool hovered = hoveredKey.channel == channelIndex && hoveredKey.key == keyIndex && hoveredKey.component == component;
					f32 radius = timelineSettings.keySize * scale * 0.8f;

					if (hovered)
						timelineDrawDiamond({ x, y }, radius * 2.2f, { 1, 1, 1, 0.35f });

					if (key.selected)
					{
						timelineDrawDiamond({ x, y }, radius * 1.7f, timelineSettings.keySelectedColor);
						timelineDrawDiamond({ x, y }, radius, componentColor);
					}
					else
					{
						timelineDrawDiamond({ x, y }, radius, componentColor);
					}

					if (key.tag[0])
						timelineDrawKeyTag(font, key.tag, x + radius + 4.0f * scale, y, timelineSettings, scale);
				}
			}
		}
	}

	ctx->renderer.popClipRect();

	// nothing is drawn past the end of the time range, except the repeated curves
	{
		f32 endTimeX = timelineTimeToX(*state, plotRect, state->endTime);

		if (endTimeX < canvasRect.right())
		{
			f32 maskX = std::max(endTimeX, canvasRect.x);

			ctx->renderer.cmdSetColor(backgroundColor);
			ctx->renderer.cmdDrawFilledRectangle({ maskX, canvasRect.y, canvasRect.right() - maskX, canvasRect.height });

			if (state->viewMode == TimelineViewMode::Curves
				&& state->repeat
				&& state->endTime > state->startTime)
			{
				f32 cycle = state->endTime - state->startTime;
				f32 repeatStartX = std::max(endTimeX, plotRect.x);
				f32 repeatEndX = std::min(plotRect.right(), canvasRect.right());

				if (repeatEndX > repeatStartX)
				{
					ctx->renderer.pushClipRect({ repeatStartX, plotRect.y, repeatEndX - repeatStartX, plotRect.height });

					Color ghostColor = timelineSettings.dimTextColor;

					ghostColor.a = 0.25f;
					ctx->renderer.cmdSetLineStyle(LineStyle(ghostColor, timelineSettings.curveLineWidth * scale));

					i32 sampleCount = std::max(2, (i32)ceilf((repeatEndX - repeatStartX) / (2.0f * scale)));

					for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
					{
						TimelineChannel& channel = state->channels[channelIndex];

						if (!channel.enabled || channel.keys.empty())
							continue;

						i32 components = timelineComponentCount(channel.type);

						for (i32 component = 0; component < components; component++)
						{
							if (!timelineComponentVisible(channel, component))
								continue;

							std::vector<Point> points;

							points.reserve(sampleCount + 1);

							for (i32 sample = 0; sample <= sampleCount; sample++)
							{
								f32 x = repeatStartX + (repeatEndX - repeatStartX) * (f32)sample / (f32)sampleCount;
								f32 sampleTime = timelineXToTime(*state, plotRect, x);
								f32 localTime = state->startTime + fmodf(sampleTime - state->startTime, cycle);
								f32 value = timelineEvaluateComponent(channel, component, localTime);

								points.push_back({ x, timelineValueToY(plotRect, value, valueMin, valueMax) });
							}

							ctx->renderer.cmdDrawPolyLine(points.data(), (u32)points.size(), false);
						}
					}

					ctx->renderer.popClipRect();
				}
			}
		}
	}

	// hovered key info
	if (hoveredKey.channel >= 0 && hoveredKey.key >= 0
		&& hoveredKey.channel < (i32)state->channels.size()
		&& hoveredKey.key < (i32)state->channels[hoveredKey.channel].keys.size()
		&& state->channels[hoveredKey.channel].keys[hoveredKey.key].time <= state->endTime
		&& runtime.dragType != TimelineWidgetState::DragType::Keys)
	{
		TimelineChannel& channel = state->channels[hoveredKey.channel];
		TimelineKey& key = channel.keys[hoveredKey.key];
		char keyInfo[128];

		if (hoveredKey.component >= 0)
			snprintf(keyInfo, sizeof(keyInfo), "t %.3fs   v %.4f", key.time, key.value[hoveredKey.component]);
		else
			snprintf(keyInfo, sizeof(keyInfo), "t %.3fs", key.time);

		Point infoSize = timelineTextSize(font, keyInfo);
		Rect infoRect = { mouse.x + 14.0f * scale, mouse.y - 24.0f * scale, infoSize.x + 10.0f * scale, 18.0f * scale };

		if (infoRect.right() > outerRect.right())
			infoRect.x = mouse.x - infoRect.width - 8.0f * scale;

		if (infoRect.y < outerRect.y)
			infoRect.y = mouse.y + 18.0f * scale;

		ctx->renderer.cmdSetColor(headerColor);
		ctx->renderer.cmdDrawFilledRectangle(infoRect);
		ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.gridStrongColor, scale));
		ctx->renderer.cmdDrawRectangle(infoRect);
		timelineDrawText(font, keyInfo, infoRect, textColor, HAlignType::Center, false);
	}

	// playhead
	{
		f32 playheadX = timelineTimeToX(*state, plotRect, state->cursorTime);

		if (playheadX >= plotRect.x - 1.0f && playheadX <= plotRect.right() + 1.0f)
		{
			f32 flagWidth = 12.0f * scale;
			f32 flagHeight = 16.0f * scale;
			f32 flagPoint = 5.0f * scale;
			f32 flagLeft = playheadX - flagWidth * 0.5f;
			f32 flagTop = rulerRect.y + 3.0f * scale;
			f32 flagBottom = flagTop + flagHeight;
			f32 flagRight = flagLeft + flagWidth;
			f32 lineTop = flagBottom + flagPoint;
			Color flagColor = Color::white;
			Color outlineColor = { 0.0f, 0.0f, 0.0f, 0.45f };
			Color glowColor = timelineSettings.playheadColor;

			glowColor.a = 0.18f;

			ctx->renderer.cmdSetLineStyle(LineStyle(glowColor, timelineSettings.playheadWidth * 5.0f * scale));
			ctx->renderer.cmdDrawLine({ playheadX, lineTop }, { playheadX, canvasRect.bottom() });
			ctx->renderer.cmdSetLineStyle(LineStyle(Color::black, (timelineSettings.playheadWidth + 2.0f) * scale));
			ctx->renderer.cmdDrawLine({ playheadX, lineTop }, { playheadX, canvasRect.bottom() });
			ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.playheadColor, timelineSettings.playheadWidth * scale));
			ctx->renderer.cmdDrawLine({ playheadX, lineTop }, { playheadX, canvasRect.bottom() });

			ctx->renderer.cmdSetColor(flagColor);
			ctx->renderer.cmdDrawFilledRectangle({ flagLeft, flagTop, flagWidth, flagHeight });
			ctx->renderer.cmdDrawSolidTriangle(
				{ flagLeft, flagBottom },
				{ flagRight, flagBottom },
				{ playheadX, flagBottom + flagPoint },
				flagColor,
				flagColor,
				flagColor);

			ctx->renderer.cmdSetLineStyle(LineStyle(outlineColor, scale));
			ctx->renderer.cmdDrawLine({ flagLeft, flagTop }, { flagRight, flagTop });
			ctx->renderer.cmdDrawLine({ flagRight, flagTop }, { flagRight, flagBottom });
			ctx->renderer.cmdDrawLine({ flagRight, flagBottom }, { playheadX, flagBottom + flagPoint });
			ctx->renderer.cmdDrawLine({ playheadX, flagBottom + flagPoint }, { flagLeft, flagBottom });
			ctx->renderer.cmdDrawLine({ flagLeft, flagBottom }, { flagLeft, flagTop });
		}
	}

	// end of the time range
	{
		f32 endTimeX = timelineTimeToX(*state, plotRect, state->endTime);

		if (endTimeX >= plotRect.x - 1.0f && endTimeX <= plotRect.right() + 1.0f)
		{
			ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.endTimeLineColor, timelineSettings.endTimeLineWidth * scale));
			ctx->renderer.cmdDrawLine({ endTimeX, rulerRect.y }, { endTimeX, canvasRect.bottom() });
		}
	}

	// box selection
	if (runtime.dragType == TimelineWidgetState::DragType::BoxSelect)
	{
		Rect selectionRect = {
			std::min(runtime.boxStart.x, runtime.boxEnd.x),
			std::min(runtime.boxStart.y, runtime.boxEnd.y),
			fabsf(runtime.boxEnd.x - runtime.boxStart.x),
			fabsf(runtime.boxEnd.y - runtime.boxStart.y)
		};

		selectionRect = selectionRect.clipInside(canvasRect);

		ctx->renderer.cmdSetColor(timelineSettings.selectionFillColor);
		ctx->renderer.cmdDrawFilledRectangle(selectionRect);
		ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.selectionBorderColor, scale));
		ctx->renderer.cmdDrawRectangle(selectionRect);
	}

	// separators
	ctx->renderer.cmdSetLineStyle(LineStyle(timelineSettings.gridStrongColor, scale));
	ctx->renderer.cmdDrawLine({ outerRect.x, toolbarRect.bottom() }, { outerRect.right(), toolbarRect.bottom() });
	ctx->renderer.cmdDrawLine({ outerRect.x, listHeaderRect.bottom() }, { outerRect.right(), listHeaderRect.bottom() });
	ctx->renderer.cmdDrawLine({ listHeaderRect.right(), toolbarRect.bottom() }, { listHeaderRect.right(), outerRect.bottom() });
	ctx->renderer.cmdDrawLine({ rulerRect.x, rulerRect.bottom() }, { rulerRect.right(), rulerRect.bottom() });

	if (disabled)
	{
		ctx->renderer.cmdSetColor(timelineSettings.disabledOverlayColor);
		ctx->renderer.cmdDrawFilledRectangle(outerRect);
	}

	ctx->renderer.popClipRect();

	// selected key value editors
	if (!disabled && toolbarFieldCount > 0 && !ctx->contextMenuActive && !ctx->contextMenuWidgetId)
	{
		Point savedPosition = ctx->position;
		WidgetId savedId = ctx->id;
		f32 timeRange = std::max(state->endTime - state->startTime, 0.001f);
		f32 valueRange = std::max(valueMax - valueMin, 0.0001f);
		f32 timeStepsPerPixel = timeRange / std::max(plotRect.width, 1.0f);
		f32 valueStepsPerPixel = valueRange / std::max(plotRect.height, 1.0f);
		f32 timeArrowStep = 1.0f / std::max(state->frameRate, 1.0f);
		f32 valueArrowStep = valueRange / 100.0f;
		f32 comboHeight = ctx->theme->getElement(WidgetElementId::ComboSliderLeftButton).normalState().height * scale;

		paddingPush(PaddingType::Layout, Point(0, 0));

		for (u32 i = 0; i < toolbarFieldCount; i++)
		{
			const TimelineToolbarField& field = toolbarFields[i];

			if (field.channel < 0 || field.channel >= (i32)state->channels.size())
				continue;

			TimelineChannel& channel = state->channels[field.channel];

			if (field.key < 0 || field.key >= (i32)channel.keys.size())
				continue;

			TimelineKey& key = channel.keys[field.key];
			Rect fieldRect = field.rect;
			bool fieldChanged = false;

			if (field.component == -2)
			{
				f32 inputHeight = ctx->theme->getElement(WidgetElementId::TextInputBody).normalState().height * scale;

				if (inputHeight > 0.0f && inputHeight < fieldRect.height)
				{
					fieldRect.y += (fieldRect.height - inputHeight) * 0.5f;
					fieldRect.height = inputHeight;
				}
			}
			else if (comboHeight > 0.0f && comboHeight < fieldRect.height)
			{
				fieldRect.y += (fieldRect.height - comboHeight) * 0.5f;
				fieldRect.height = comboHeight;
			}

			layoutBegin(fieldRect);

			if (field.component == -1)
			{
				f32 minTime = state->startTime;
				f32 maxTime = state->endTime;

				if (field.key > 0)
					minTime = std::max(minTime, channel.keys[field.key - 1].time + 0.001f);
				if (field.key + 1 < (i32)channel.keys.size())
					maxTime = std::min(maxTime, channel.keys[field.key + 1].time - 0.001f);

				if (minTime <= maxTime)
					fieldChanged = comboSliderFloatRanged(&key.time, minTime, maxTime, timeStepsPerPixel, timeArrowStep, "t %.3f");
				else
					comboSliderFloatRanged(&key.time, state->startTime, state->endTime, timeStepsPerPixel, timeArrowStep, "t %.3f");
			}
			else if (field.component == -2)
			{
				idPush((void*)&runtime);
				fieldChanged = textInput("timelineKeyTag", key.tag, TimelineKey::maxTagSize, TextInputFlags::None, "Tag");
				idPop();
			}
			else if (field.component >= 0 && field.component < timelineComponentCount(channel.type))
			{
				const char* label = timelineComponentLabel(channel.type, field.component);
				char format[32];
				f32 stepsPerPixel = valueStepsPerPixel;
				f32 arrowStep = valueArrowStep;

				if (channel.type == TimelineKeyType::Int)
				{
					stepsPerPixel = 1.0f;
					arrowStep = 1.0f;
				}

				snprintf(format, sizeof(format), "%s %s", label, channel.type == TimelineKeyType::Int ? "%.0f" : "%.3f");
				fieldChanged = comboSliderFloat(&key.value[field.component], stepsPerPixel, arrowStep, format);

				if (fieldChanged)
					key.value[field.component] = timelineQuantizeComponent(channel.type, key.value[field.component], field.component);
			}

			layoutEnd();

			if (fieldChanged)
				changed = true;
		}

		paddingPop(PaddingType::Layout);
		ctx->position = savedPosition;
		ctx->id = savedId;
	}

	// channel checkbox context menus
	if (!disabled && !state->channels.empty())
	{
		Point savedPosition = ctx->position;
		WidgetId savedId = ctx->id;
		f32 scrollOffset = state->channelScroll * scale;

		idPush((void*)&runtime);

		for (u32 rowIndex = 0; rowIndex < channelRows.size(); rowIndex++)
		{
			const TimelineChannelRow& rowInfo = channelRows[rowIndex];

			if (rowInfo.component >= 0)
				continue;

			i32 channelIndex = rowInfo.channel;
			Rect rowRect = { listRect.x, listRect.y - scrollOffset + (f32)rowIndex * rowHeight, listRect.width, rowHeight };
			Rect checkboxRect = timelineChannelCheckboxRect(rowRect, scale);

			if (checkboxRect.bottom() < listRect.y || checkboxRect.y > listRect.bottom())
				continue;

			ctx->id = genId((u32)(channelIndex + 1));
			ctx->widget.rect = checkboxRect;
			ctx->widget.disabled = false;

			if (contextMenuBegin(ContextMenuFlags::None))
			{
				if (menuItem("Uncheck All But This"))
				{
					for (i32 i = 0; i < (i32)state->channels.size(); i++)
						state->channels[i].enabled = i == channelIndex;

					changed = true;
				}

				if (menuItem("Check All"))
				{
					for (auto& channel : state->channels)
						channel.enabled = true;

					changed = true;
				}

				contextMenuEnd();
			}
		}

		idPop();
		ctx->position = savedPosition;
		ctx->id = savedId;
	}

	// cursor feedback
	if (!disabled
		&& (mouseInTimeline
			|| runtime.dragType == TimelineWidgetState::DragType::TangentIn
			|| runtime.dragType == TimelineWidgetState::DragType::TangentOut))
	{
		if (runtime.dragType == TimelineWidgetState::DragType::Pan)
			mouseCursorSetType(MouseCursorType::SizeAll);
		else if (runtime.dragType == TimelineWidgetState::DragType::Playhead)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == TimelineWidgetState::DragType::TangentIn
			|| runtime.dragType == TimelineWidgetState::DragType::TangentOut)
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (runtime.dragType == TimelineWidgetState::DragType::Keys)
			mouseCursorSetType(state->viewMode == TimelineViewMode::Curves ? MouseCursorType::SizeAll : MouseCursorType::SizeWE);
		else if (runtime.dragType == TimelineWidgetState::DragType::ChannelResize || channelSplitterRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == TimelineWidgetState::DragType::ChannelScroll || channelScrollbar.track.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (hoveredToolbarButton >= 0 || listRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (rulerRect.contains(mouse) || fabsf(mouse.x - timelineTimeToX(*state, plotRect, state->cursorTime)) <= 5.0f * scale)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (hoveredTangent.channel >= 0 || hoveredKey.channel >= 0)
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (canvasRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::CrossHair);
	}

	return changed;
}

}
