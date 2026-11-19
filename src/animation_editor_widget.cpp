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
enum AnimationEditorToolbarAction
{
	AnimationEditorTbPlay,
	AnimationEditorTbKeys,
	AnimationEditorTbCurves,
	AnimationEditorTbAddKey,
	AnimationEditorTbDeleteKey,
	AnimationEditorTbSnap,
	AnimationEditorTbZoomOut,
	AnimationEditorTbZoomIn,
	AnimationEditorTbFit,
	AnimationEditorTbCount
};

struct AnimationEditorToolbarButton
{
	AnimationEditorToolbarAction action;
	Rect rect;
	const char* label;
	bool active;
};

struct AnimationEditorKeyHit
{
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1;
};

static f32 animationEditorClamp(f32 value, f32 minValue, f32 maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}

static i32 animationEditorComponentCount(AnimationEditorKeyType type)
{
	switch (type)
	{
	case AnimationEditorKeyType::Vec2: return 2;
	case AnimationEditorKeyType::Vec3: return 3;
	case AnimationEditorKeyType::Vec4: return 4;
	default: return 1;
	}
}

static bool animationEditorComponentVisible(const AnimationEditorChannel& channel, i32 component)
{
	if (!channel.enabled)
		return false;

	if (channel.selected)
		return true;

	if (component < 0 || component >= 32)
		return false;

	return (channel.selectedComponents & (1u << component)) != 0;
}

static bool animationEditorChannelHasVisibleComponent(const AnimationEditorChannel& channel)
{
	if (!channel.enabled)
		return false;

	if (channel.selected)
		return true;

	return channel.selectedComponents != 0;
}

static const char* animationEditorTypeBadge(AnimationEditorKeyType type)
{
	switch (type)
	{
	case AnimationEditorKeyType::Int: return "I";
	case AnimationEditorKeyType::Vec2: return "V2";
	case AnimationEditorKeyType::Vec3: return "V3";
	case AnimationEditorKeyType::Vec4: return "V4";
	case AnimationEditorKeyType::Bool: return "B";
	default: return "F";
	}
}

static f32 animationEditorSnapTime(const AnimationEditorState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return animationEditorClamp(time, state.startTime, state.endTime);
}

static f32 animationEditorSnapTimeNoMax(const AnimationEditorState& state, f32 time)
{
	if (state.snapToFrames && state.frameRate > 0.0f)
	{
		time = roundf(time * state.frameRate) / state.frameRate;
	}

	return std::max(time, state.startTime);
}

static f32 animationEditorQuantizeComponent(AnimationEditorKeyType type, f32 value, i32 component)
{
	if (component != 0)
		return value;

	switch (type)
	{
	case AnimationEditorKeyType::Int: return roundf(value);
	case AnimationEditorKeyType::Bool: return value >= 0.5f ? 1.0f : 0.0f;
	default: return value;
	}
}

static bool animationEditorKeyHasComponent(const AnimationEditorKey& key, i32 component)
{
	return key.components == 0 || (key.components & (1u << component));
}

// finds the nearest keys before and after the given key that belong to the component
static void animationEditorKeyNeighbors(
	const AnimationEditorChannel& channel,
	i32 keyIndex,
	i32 component,
	i32& prevIndex,
	i32& nextIndex)
{
	i32 keyCount = (i32)channel.keys.size();

	prevIndex = keyIndex - 1;

	while (prevIndex >= 0 && !animationEditorKeyHasComponent(channel.keys[prevIndex], component))
		prevIndex--;

	nextIndex = keyIndex + 1;

	while (nextIndex < keyCount && !animationEditorKeyHasComponent(channel.keys[nextIndex], component))
		nextIndex++;
}

static f32 animationEditorAutoTangent(const AnimationEditorChannel& channel, i32 keyIndex, i32 component)
{
	i32 keyCount = (i32)channel.keys.size();

	if (keyIndex < 0 || keyIndex >= keyCount || component < 0 || component >= 4 || keyCount < 2)
		return 0.0f;

	// the tangents are computed from the nearest keys that belong to this component
	i32 prevIndex, nextIndex;

	animationEditorKeyNeighbors(channel, keyIndex, component, prevIndex, nextIndex);

	// the first and last keys of a component have a flat tangent so the curve
	// settles exactly onto their value instead of overshooting
	if (prevIndex < 0 || nextIndex >= keyCount)
		return 0.0f;

	f32 duration = channel.keys[nextIndex].time - channel.keys[prevIndex].time;
	return duration > 0.00001f ? (channel.keys[nextIndex].value[component] - channel.keys[prevIndex].value[component]) / duration : 0.0f;
}

// returns the tangent of a key, clamped so the curve never overshoots the
// neighbouring key values and stays flat on flat segments
static f32 animationEditorKeyTangent(const AnimationEditorChannel& channel, i32 keyIndex, i32 component, bool outgoing)
{
	const AnimationEditorKey& key = channel.keys[keyIndex];

	if (!key.tangentAuto)
		return outgoing ? key.outTangent[component] : key.inTangent[component];

	f32 tangent = animationEditorAutoTangent(channel, keyIndex, component);

	if (tangent == 0.0f)
		return 0.0f;

	i32 prevIndex, nextIndex;

	animationEditorKeyNeighbors(channel, keyIndex, component, prevIndex, nextIndex);

	i32 neighborIndex = outgoing ? nextIndex : prevIndex;
	i32 keyCount = (i32)channel.keys.size();

	if (neighborIndex < 0 || neighborIndex >= keyCount)
		return 0.0f;

	f32 timeDelta = channel.keys[neighborIndex].time - key.time;
	f32 valueDelta = channel.keys[neighborIndex].value[component] - key.value[component];

	// flat segments get a flat tangent
	if (fabsf(valueDelta) < 0.00001f || fabsf(timeDelta) < 0.00001f)
		return 0.0f;

	f32 segmentSlope = valueDelta / timeDelta;

	// the tangent has to point in the same direction as the segment, otherwise
	// the curve would overshoot, so flatten it
	if (segmentSlope * tangent <= 0.0f)
		return 0.0f;

	// limit the tangent to three times the segment slope, the curve then never
	// goes past the neighbouring key values
	f32 limit = 3.0f * fabsf(segmentSlope);

	if (fabsf(tangent) > limit)
		tangent = tangent > 0.0f ? limit : -limit;

	return tangent;
}

static void animationEditorMakeKeyTangentsManual(AnimationEditorChannel& channel, i32 keyIndex)
{
	AnimationEditorKey& key = channel.keys[keyIndex];

	if (!key.tangentAuto)
		return;

	for (i32 i = 0; i < 4; i++)
	{
		f32 tangent = animationEditorAutoTangent(channel, keyIndex, i);

		key.inTangent[i] = tangent;
		key.outTangent[i] = tangent;
	}

	key.tangentAuto = false;
}

static f32 animationEditorEvaluateComponent(const AnimationEditorChannel& channel, i32 component, f32 time)
{
	if (channel.keys.empty() || component < 0 || component >= 4)
		return 0.0f;

	// upper bound: find the first key with a time greater than the given time,
	// it can be the key count when the time is past the last key
	u32 lo = 0;
	u32 hi = (u32)channel.keys.size();

	while (lo < hi)
	{
		u32 mid = (lo + hi) / 2;

		if (channel.keys[mid].time <= time)
			lo = mid + 1;
		else
			hi = mid;
	}

	// find the closest keys on both sides that belong to this component
	i32 firstIndex = (i32)lo - 1;
	i32 secondIndex = (i32)lo;

	while (firstIndex >= 0 && !animationEditorKeyHasComponent(channel.keys[firstIndex], component))
		firstIndex--;

	while (secondIndex < (i32)channel.keys.size() && !animationEditorKeyHasComponent(channel.keys[secondIndex], component))
		secondIndex++;

	if (firstIndex < 0 && secondIndex >= (i32)channel.keys.size())
		return 0.0f;

	if (firstIndex < 0)
		return channel.keys[secondIndex].value[component];

	if (secondIndex >= (i32)channel.keys.size())
		return channel.keys[firstIndex].value[component];

	const AnimationEditorKey& first = channel.keys[firstIndex];
	const AnimationEditorKey& second = channel.keys[secondIndex];

	if (channel.type == AnimationEditorKeyType::Bool || channel.type == AnimationEditorKeyType::Int)
		return first.value[component];

	f32 duration = second.time - first.time;

	if (duration <= 0.0f)
		return second.value[component];

	f32 t = (time - first.time) / duration;
	f32 t2 = t * t;
	f32 t3 = t2 * t;
	f32 tangentIn = animationEditorKeyTangent(channel, firstIndex, component, true);
	f32 tangentOut = animationEditorKeyTangent(channel, secondIndex, component, false);
	f32 h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	f32 h10 = t3 - 2.0f * t2 + t;
	f32 h01 = -2.0f * t3 + 3.0f * t2;
	f32 h11 = t3 - t2;

	return h00 * first.value[component]
		+ h10 * duration * tangentIn
		+ h01 * second.value[component]
		+ h11 * duration * tangentOut;
}

static void animationEditorEvaluateChannel(const AnimationEditorChannel& channel, f32 time, f32* outValues)
{
	i32 components = animationEditorComponentCount(channel.type);

	for (i32 i = 0; i < 4; i++)
		outValues[i] = i < components ? animationEditorEvaluateComponent(channel, i, time) : 0.0f;
}

static void animationEditorSortKeys(AnimationEditorChannel& channel)
{
	std::sort(channel.keys.begin(), channel.keys.end(), [](const AnimationEditorKey& a, const AnimationEditorKey& b)
	{
		return a.time < b.time;
	});
}

static bool animationEditorHasDraggedKey(const AnimationEditorWidgetState& runtime, i32 channelIndex)
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

static void animationEditorRemapDraggedKeys(AnimationEditorWidgetState& runtime, i32 channelIndex, i32 fromIndex, i32 toIndex)
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

static void animationEditorReorderDraggedKeys(AnimationEditorWidgetState& runtime, std::vector<AnimationEditorChannel>& channels)
{
	for (i32 channelIndex = 0; channelIndex < (i32)channels.size(); channelIndex++)
	{
		if (!animationEditorHasDraggedKey(runtime, channelIndex))
			continue;

		auto& keys = channels[channelIndex].keys;

		for (i32 i = 1; i < (i32)keys.size(); i++)
		{
			i32 j = i;

			while (j > 0 && keys[j - 1].time > keys[j].time)
			{
				std::swap(keys[j - 1], keys[j]);
				animationEditorRemapDraggedKeys(runtime, channelIndex, j - 1, j);
				j--;
			}
		}
	}
}

static f32 animationEditorNiceStep(f32 rawStep)
{
	if (rawStep <= 0.0f)
		return 1.0f;

	f32 exponent = floorf(log10f(rawStep));
	f32 base = powf(10.0f, exponent);
	f32 fraction = rawStep / base;
	f32 nice = fraction <= 1.0f ? 1.0f : fraction <= 2.0f ? 2.0f : fraction <= 5.0f ? 5.0f : 10.0f;

	return nice * base;
}

static Color animationEditorChannelColor(i32 channelIndex, i32 component = -1)
{
	f32 hue = fmodf((f32)channelIndex * 0.61803398875f, 1.0f);

	if (component >= 0)
		hue = fmodf(hue + (f32)component * 0.045f, 1.0f);

	// darken the bright hue colors on light themes so the curves and markers
	// stay visible
	Color color = colorHueToRgb(hue, 1.0f);
	f32 contrast = themeColorContrastFactor();

	color = { color.r * contrast, color.g * contrast, color.b * contrast, color.a };

	return color;
}

static void animationEditorComputeAutoValueRange(const AnimationEditorState& state, f32& valueMin, f32& valueMax)
{
	bool hasValues = false;
	f32 computedMin = FLT_MAX;
	f32 computedMax = -FLT_MAX;

	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		i32 components = animationEditorComponentCount(channel.type);

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

static void animationEditorComputeValueRange(const AnimationEditorState& state, f32& valueMin, f32& valueMax)
{
	if (state.viewMode == AnimationEditorViewMode::Curves && state.autoValueRange)
	{
		animationEditorComputeAutoValueRange(state, valueMin, valueMax);
		return;
	}

	valueMin = state.valueMin;
	valueMax = state.valueMax;

	if (valueMax - valueMin < 0.00001f)
		valueMax = valueMin + 1.0f;
}

static f32 animationEditorTimeToX(const AnimationEditorState& state, const Rect& plotRect, f32 time)
{
	return plotRect.x + (time - state.scrollTime) * state.pixelsPerSecond * ctx->scale;
}

static f32 animationEditorXToTime(const AnimationEditorState& state, const Rect& plotRect, f32 x)
{
	f32 pixelsPerSecond = state.pixelsPerSecond * ctx->scale;

	if (pixelsPerSecond <= 0.0f)
		return state.scrollTime;

	return state.scrollTime + (x - plotRect.x) / pixelsPerSecond;
}

static f32 animationEditorValueToY(const Rect& plotRect, f32 value, f32 valueMin, f32 valueMax)
{
	f32 range = valueMax - valueMin;

	if (range <= 0.00001f)
		range = 1.0f;

	return plotRect.bottom() - (value - valueMin) / range * plotRect.height;
}

static f32 animationEditorYToValue(const Rect& plotRect, f32 y, f32 valueMin, f32 valueMax)
{
	f32 range = valueMax - valueMin;

	if (range <= 0.00001f)
		range = 1.0f;

	return valueMin + (plotRect.bottom() - y) / std::max(plotRect.height, 1.0f) * range;
}

static Color animationEditorSampleElementColor(WidgetElementId id, WidgetStateType stateType, bool useTextColor)
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

static Color animationEditorShadeColor(const Color& color, f32 amount)
{
	return {
		animationEditorClamp(color.r + amount, 0.0f, 1.0f),
		animationEditorClamp(color.g + amount, 0.0f, 1.0f),
		animationEditorClamp(color.b + amount, 0.0f, 1.0f),
		color.a };
}

static void animationEditorApplyThemeColors(AnimationEditorSettings& animationEditorSettings)
{
	if (!ctx->theme)
		return;

	Color windowColor = animationEditorSampleElementColor(WidgetElementId::WindowBody, WidgetStateType::Normal, false);
	Color textColor = animationEditorSampleElementColor(WidgetElementId::LabelBody, WidgetStateType::Normal, true);
	Color accentColor = animationEditorSampleElementColor(WidgetElementId::LinkBody, WidgetStateType::Normal, true);
	f32 luminance = windowColor.r * 0.299f + windowColor.g * 0.587f + windowColor.b * 0.114f;
	f32 rowShade = luminance < 0.5f ? 0.025f : -0.025f;

	animationEditorSettings.backgroundColor = windowColor;
	animationEditorSettings.channelListColor = animationEditorShadeColor(windowColor, rowShade * 1.4f);
	animationEditorSettings.headerColor = animationEditorShadeColor(windowColor, rowShade * 2.0f);
	animationEditorSettings.rowAlternateColor = animationEditorShadeColor(windowColor, rowShade);
	animationEditorSettings.rowSelectedColor = { accentColor.r, accentColor.g, accentColor.b, 0.5f };
	animationEditorSettings.gridColor = { textColor.r, textColor.g, textColor.b, 0.12f };
	animationEditorSettings.gridStrongColor = { textColor.r, textColor.g, textColor.b, 0.22f };
	animationEditorSettings.textColor = textColor;
	animationEditorSettings.dimTextColor = { textColor.r, textColor.g, textColor.b, 0.55f };
	animationEditorSettings.keySelectedColor = accentColor;
	animationEditorSettings.playheadColor = Color::white;
	animationEditorSettings.toolbarToggleColor = animationEditorShadeColor(
		animationEditorSettings.toolbarToggleColor,
		luminance < 0.3f ? 0.08f : (luminance > 0.55f ? -0.1f : 0.0f));
	animationEditorSettings.selectionFillColor = { accentColor.r, accentColor.g, accentColor.b, 0.14f };
	animationEditorSettings.selectionBorderColor = { accentColor.r, accentColor.g, accentColor.b, 0.7f };
	animationEditorSettings.endTimeLineColor = { textColor.r, textColor.g, textColor.b, 0.35f };
}

struct AnimationEditorTangentHit
{
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1;
	bool outgoing = false;
};

static Point animationEditorTangentHandlePosition(
	const AnimationEditorState& state,
	const AnimationEditorChannel& channel,
	i32 keyIndex,
	i32 component,
	bool outgoing,
	const Rect& plotRect,
	f32 valueMin,
	f32 valueMax,
	f32 handlePixels)
{
	const AnimationEditorKey& key = channel.keys[keyIndex];
	f32 pixelsPerSecond = state.pixelsPerSecond * ctx->scale;
	f32 handleTime = pixelsPerSecond > 0.00001f ? handlePixels / pixelsPerSecond : 0.0f;
	f32 tangent = animationEditorKeyTangent(channel, keyIndex, component, outgoing);
	f32 time = outgoing ? key.time + handleTime : key.time - handleTime;
	f32 value = outgoing ? key.value[component] + tangent * handleTime : key.value[component] - tangent * handleTime;

	return { animationEditorTimeToX(state, plotRect, time), animationEditorValueToY(plotRect, value, valueMin, valueMax) };
}

static AnimationEditorTangentHit animationEditorFindTangentHandle(
	const AnimationEditorState& state,
	const AnimationEditorSettings& settings,
	const Rect& plotRect,
	f32 valueMin,
	f32 valueMax,
	const Point& mouse)
{
	AnimationEditorTangentHit result;
	f32 hitRadius = settings.keyHitRadius * ctx->scale;
	f32 handlePixels = settings.tangentHandleLength * ctx->scale;

	for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
	{
		const AnimationEditorChannel& channel = state.channels[channelIndex];

		if (!channel.enabled || channel.type == AnimationEditorKeyType::Bool || channel.type == AnimationEditorKeyType::Int)
			continue;

		i32 components = animationEditorComponentCount(channel.type);

		for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
		{
			if (!channel.keys[keyIndex].selected && channel.keys[keyIndex].selectedComponents == 0)
				continue;

			for (i32 component = 0; component < components; component++)
			{
				if (!animationEditorComponentVisible(channel, component)
					|| !animationEditorKeyHasComponent(channel.keys[keyIndex], component))
					continue;

				for (i32 side = 0; side < 2; side++)
				{
					bool outgoing = side == 0;
					Point handle = animationEditorTangentHandlePosition(state, channel, keyIndex, component, outgoing, plotRect, valueMin, valueMax, handlePixels);

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

struct AnimationEditorToolbarField
{
	Rect rect;
	i32 channel = -1;
	i32 key = -1;
	i32 component = -1; // -1 is the key time, -2 is the key tag
};

static u32 animationEditorBuildToolbarFields(
	const Rect& toolbarRect,
	const AnimationEditorToolbarButton* buttons,
	u32 buttonCount,
	const AnimationEditorSettings& settings,
	const AnimationEditorState& state,
	AnimationEditorToolbarField* fields,
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
			if (!state.channels[channelIndex].keys[keyIndex].selected
				&& state.channels[channelIndex].keys[keyIndex].selectedComponents == 0)
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

	const AnimationEditorChannel& channel = state.channels[selectedChannel];
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

	if (channel.type != AnimationEditorKeyType::Bool)
	{
		const AnimationEditorKey& key = channel.keys[selectedKey];
		i32 components = animationEditorComponentCount(channel.type);

		for (i32 component = 0; component < components; component++)
		{
			if (!animationEditorKeyHasComponent(key, component))
				continue;

			addField(component, settings.toolbarValueFieldWidth * scale);
		}
	}

	addField(-2, settings.toolbarTagFieldWidth * scale);

	return count;
}

static const char* animationEditorComponentLabel(AnimationEditorKeyType type, i32 component)
{
	switch (type)
	{
	case AnimationEditorKeyType::Vec2:
	case AnimationEditorKeyType::Vec3:
	case AnimationEditorKeyType::Vec4:
	{
		static const char* axisLabels[4] = { "X", "Y", "Z", "W" };
		return component >= 0 && component < 4 ? axisLabels[component] : "V";
	}
	case AnimationEditorKeyType::Int:
		return "I";
	default:
		return "V";
	}
}

static Point animationEditorTextSize(Font* font, const char* text)
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

static void animationEditorDrawText(Font* font, const char* text, const Rect& rect, const Color& color, HAlignType align, bool ellipsis = true)
{
	if (!text || !text[0])
		return;

	ctx->renderer.cmdSetFont(font);
	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawTextInBox(text, rect, align, VAlignType::Center, ellipsis, true);
}

static void animationEditorDrawKeyTag(Font* font, const char* tag, f32 x, f32 centerY, const AnimationEditorSettings& settings, f32 scale)
{
	Point textSize = animationEditorTextSize(font, tag);
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
	animationEditorDrawText(font, tag, { x + padX, centerY - textSize.y * 0.5f, textSize.x, textSize.y }, settings.textColor, HAlignType::Left);
}

static void animationEditorDrawDiamond(const Point& center, f32 radius, const Color& color)
{
	Point top = { center.x, center.y - radius };
	Point right = { center.x + radius, center.y };
	Point bottom = { center.x, center.y + radius };
	Point left = { center.x - radius, center.y };

	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawSolidTriangle(top, right, bottom, color, color, color);
	ctx->renderer.cmdDrawSolidTriangle(top, bottom, left, color, color, color);
}

static void animationEditorDrawButton(Font* font, const Rect& rect, const char* label, bool active, bool hovered, bool pressed, const Color& activeColor)
{
	auto& buttonElem = ctx->theme->getElement(WidgetElementId::ButtonBody);
	ThemeElement::State* state = &buttonElem.normalState();

	if (pressed || active)
	{
		auto& pressedState = buttonElem.getState(WidgetStateType::Pressed);
		Color backgroundColor = activeColor;

		if (pressed)
			backgroundColor = animationEditorShadeColor(backgroundColor, -0.08f);
		else if (hovered)
			backgroundColor = animationEditorShadeColor(backgroundColor, 0.06f);

		ctx->renderer.cmdSetColor(backgroundColor);
		ctx->renderer.cmdDrawImageBordered(pressedState.image, pressedState.border, rect, ctx->scale);
		animationEditorDrawText(font, label, rect, Color::white, HAlignType::Center);
		return;
	}

	if (hovered)
		state = &buttonElem.getState(WidgetStateType::Hovered);

	ctx->renderer.cmdSetColor(state->color);
	ctx->renderer.cmdDrawImageBordered(state->image, state->border, rect, ctx->scale);
	animationEditorDrawText(font, label, rect, state->textColor, HAlignType::Center);
}

static void animationEditorDrawCheckbox(const Rect& rect, bool checked, bool disabled)
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

static Rect animationEditorChannelCheckboxRect(const Rect& rowRect, f32 scale)
{
	f32 size = 14.0f * scale;

	return { rowRect.x + 8.0f * scale, rowRect.y + (rowRect.height - size) * 0.5f, size, size };
}

static Rect animationEditorChannelExpandRect(const Rect& rowRect, f32 scale)
{
	f32 size = 10.0f * scale;

	return { rowRect.x + 27.0f * scale, rowRect.y + (rowRect.height - size) * 0.5f, size, size };
}

static Rect animationEditorChannelBadgeRect(const Rect& rowRect, f32 scale)
{
	f32 height = 14.0f * scale;

	return { rowRect.x + 42.0f * scale, rowRect.y + (rowRect.height - height) * 0.5f, 24.0f * scale, height };
}

struct AnimationEditorChannelRow
{
	i32 channel = -1;
	i32 component = -1;
};

static void animationEditorBuildChannelRows(const AnimationEditorState& state, std::vector<AnimationEditorChannelRow>& rows)
{
	rows.clear();

	for (i32 channelIndex = 0; channelIndex < (i32)state.channels.size(); channelIndex++)
	{
		const AnimationEditorChannel& channel = state.channels[channelIndex];

		rows.push_back({ channelIndex, -1 });

		if (channel.expanded)
		{
			i32 components = animationEditorComponentCount(channel.type);

			if (components > 1)
			{
				for (i32 component = 0; component < components; component++)
					rows.push_back({ channelIndex, component });
			}
		}
	}
}

static i32 animationEditorTotalChannelRows(const AnimationEditorState& state)
{
	i32 count = 0;

	for (auto& channel : state.channels)
	{
		count++;

		if (channel.expanded)
		{
			i32 components = animationEditorComponentCount(channel.type);

			if (components > 1)
				count += components;
		}
	}

	return count;
}

static void animationEditorClampChannelScroll(AnimationEditorState& state, const Rect& listRect, const AnimationEditorSettings& settings)
{
	f32 visibleHeight = listRect.height / ctx->scale;
	f32 totalHeight = (f32)animationEditorTotalChannelRows(state) * settings.rowHeight;
	f32 maxScroll = std::max(0.0f, totalHeight - visibleHeight);

	state.channelScroll = animationEditorClamp(state.channelScroll, 0.0f, maxScroll);
}

struct AnimationEditorChannelScrollbar
{
	Rect track;
	Rect thumb;
	f32 maxScroll = 0.0f;
};

static AnimationEditorChannelScrollbar animationEditorGetChannelScrollbar(
	const AnimationEditorState& state,
	const AnimationEditorSettings& settings,
	const Rect& listRect,
	f32 rowHeight)
{
	AnimationEditorChannelScrollbar scrollbar;
	f32 contentHeight = (f32)animationEditorTotalChannelRows(state) * rowHeight;

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
	f32 ratio = maxScroll > 0.0f ? animationEditorClamp(state.channelScroll / maxScroll, 0.0f, 1.0f) : 0.0f;
	scrollbar.thumb = { scrollbar.track.x, listRect.y + travel * ratio, trackWidth, thumbHeight };

	return scrollbar;
}

static void animationEditorClampView(AnimationEditorState& state, const Rect& plotRect, const AnimationEditorSettings& settings)
{
	state.pixelsPerSecond = animationEditorClamp(state.pixelsPerSecond, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	f32 pixelsPerSecond = std::max(state.pixelsPerSecond * ctx->scale, 0.0001f);
	f32 visibleTime = plotRect.width / pixelsPerSecond;
	f32 maxScrollTime = std::max(state.startTime, state.endTime - visibleTime);

	state.scrollTime = animationEditorClamp(state.scrollTime, state.startTime, maxScrollTime);
	state.cursorTime = animationEditorClamp(state.cursorTime, state.startTime, state.endTime);
}

static i32 animationEditorTargetChannel(const AnimationEditorState& state)
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

static void animationEditorClearSelection(AnimationEditorState& state)
{
	for (auto& channel : state.channels)
	{
		for (auto& key : channel.keys)
		{
			key.selected = false;
			key.selectedComponents = 0;
		}
	}
}

// splits the key so the given component becomes an independent key at the same
// time, returns the index of the key belonging to the component
static i32 animationEditorSplitKey(AnimationEditorChannel& channel, i32 keyIndex, i32 component)
{
	if (keyIndex < 0 || keyIndex >= (i32)channel.keys.size())
		return keyIndex;

	i32 componentCount = animationEditorComponentCount(channel.type);
	u32 allBits = (1u << componentCount) - 1u;
	AnimationEditorKey& key = channel.keys[keyIndex];
	u32 oldComponents = key.components == 0 ? allBits : key.components;

	// the key already belongs only to this component
	if ((oldComponents & ~(1u << component)) == 0)
		return keyIndex;

	key.components = oldComponents & ~(1u << component);
	key.selected = false;
	key.selectedComponents = 0;

	AnimationEditorKey newKey = key;

	newKey.components = (1u << component);
	newKey.selected = false;
	newKey.selectedComponents = 0;

	channel.keys.insert(channel.keys.begin() + keyIndex + 1, newKey);

	return keyIndex + 1;
}

static bool animationEditorAddKey(AnimationEditorState& state, i32 channelIndex, f32 time, const f32* values, u32 componentMask, bool select)
{
	if (channelIndex < 0 || channelIndex >= (i32)state.channels.size())
		return false;

	AnimationEditorChannel& channel = state.channels[channelIndex];

	if (!channel.enabled)
		return false;

	time = animationEditorSnapTime(state, time);

	f32 newValues[4] = { 0, 0, 0, 0 };

	if (values)
	{
		for (i32 i = 0; i < 4; i++)
			newValues[i] = values[i];
	}
	else
	{
		animationEditorEvaluateChannel(channel, time, newValues);
	}

	i32 components = animationEditorComponentCount(channel.type);

	for (i32 i = 0; i < 4; i++)
		newValues[i] = i < components ? animationEditorQuantizeComponent(channel.type, newValues[i], i) : 0.0f;

	for (auto& key : channel.keys)
	{
		if (fabsf(key.time - time) <= 0.0001f)
		{
			// merge the masks, zero means the key covers all the components
			if (componentMask == 0)
				key.components = 0;
			else if (key.components != 0)
				key.components |= componentMask;

			for (i32 i = 0; i < 4; i++)
			{
				if (componentMask == 0 || (componentMask & (1u << i)))
					key.value[i] = newValues[i];
			}

			if (select)
			{
				animationEditorClearSelection(state);
				key.selected = true;
			}

			state.selectedChannel = channelIndex;
			return true;
		}
	}

	if (select)
		animationEditorClearSelection(state);

	AnimationEditorKey key;
	key.time = time;
	key.selected = select;
	key.components = componentMask;

	for (i32 i = 0; i < 4; i++)
		key.value[i] = newValues[i];

	auto insertPos = std::lower_bound(channel.keys.begin(), channel.keys.end(), time, [](const AnimationEditorKey& a, f32 value)
	{
		return a.time < value;
	});

	channel.keys.insert(insertPos, key);
	state.selectedChannel = channelIndex;

	return true;
}

static i32 animationEditorDeleteSelectedKeys(AnimationEditorState& state)
{
	i32 deleted = 0;

	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		u32 componentBits = (1u << animationEditorComponentCount(channel.type)) - 1u;
		size_t writeIndex = 0;

		for (size_t i = 0; i < channel.keys.size(); i++)
		{
			AnimationEditorKey& key = channel.keys[i];

			if (!key.selected && key.selectedComponents == 0)
			{
				if (writeIndex != i)
					channel.keys[writeIndex] = key;

				writeIndex++;
				continue;
			}

			// a whole key is deleted, a lane selected key only loses the
			// selected components and disappears when none are left
			if (!key.selected)
			{
				u32 oldComponents = key.components == 0 ? componentBits : key.components;
				u32 newComponents = oldComponents & ~key.selectedComponents;

				if (newComponents != 0)
				{
					key.components = newComponents;
					key.selectedComponents = 0;

					if (writeIndex != i)
						channel.keys[writeIndex] = key;

					writeIndex++;
					deleted++;
					continue;
				}
			}

			deleted++;
		}

		channel.keys.resize(writeIndex);
	}

	return deleted;
}

static void animationEditorSelectAll(AnimationEditorState& state)
{
	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		for (auto& key : channel.keys)
			key.selected = true;
	}
}

static bool animationEditorIsDraggedKey(
	const std::vector<AnimationEditorKeyDragItem>& dragItems,
	i32 channelIndex,
	i32 keyIndex)
{
	for (const auto& item : dragItems)
	{
		if (item.channel == channelIndex && item.key == keyIndex)
			return true;
	}

	return false;
}

// applies the push pass to a channel: the dragged keys move by the delta and
// push the keys they reach, either only computing the resulting first and last
// key times (apply false) or moving the keys (apply true)
static void animationEditorPushChannel(
	AnimationEditorChannel& channel,
	const std::vector<AnimationEditorKeyDragItem>& dragItems,
	i32 channelIndex,
	f32 timeDelta,
	bool apply,
	f32& outMinTime,
	f32& outMaxTime)
{
	i32 keyCount = (i32)channel.keys.size();

	outMinTime = FLT_MAX;
	outMaxTime = -FLT_MAX;

	if (timeDelta == 0.0f || keyCount == 0)
		return;

	if (timeDelta > 0.0f)
	{
		f32 cursor = -FLT_MAX;

		for (i32 i = 0; i < keyCount; i++)
		{
			AnimationEditorKey& key = channel.keys[i];
			f32 newTime = key.time + (animationEditorIsDraggedKey(dragItems, channelIndex, i) ? timeDelta : 0.0f);

			// the keys the dragged ones reach are pushed forward
			if (newTime < cursor)
				newTime = cursor;

			outMinTime = std::min(outMinTime, newTime);
			outMaxTime = std::max(outMaxTime, newTime);

			if (apply)
				key.time = newTime;

			cursor = newTime;
		}
	}
	else
	{
		f32 cursor = FLT_MAX;

		for (i32 i = keyCount - 1; i >= 0; i--)
		{
			AnimationEditorKey& key = channel.keys[i];
			f32 newTime = key.time + (animationEditorIsDraggedKey(dragItems, channelIndex, i) ? timeDelta : 0.0f);

			// the keys the dragged ones reach are pushed backward
			if (newTime > cursor)
				newTime = cursor;

			outMinTime = std::min(outMinTime, newTime);
			outMaxTime = std::max(outMaxTime, newTime);

			if (apply)
				key.time = newTime;

			cursor = newTime;
		}
	}
}

static void animationEditorNudgeSelectedKeys(AnimationEditorState& state, f32 delta)
{
	for (auto& channel : state.channels)
	{
		if (!channel.enabled)
			continue;

		bool changed = false;

		for (auto& key : channel.keys)
		{
			if (!key.selected && key.selectedComponents == 0)
				continue;

			key.time = animationEditorClamp(key.time + delta, state.startTime, state.endTime);
			changed = true;
		}

		if (changed)
			animationEditorSortKeys(channel);
	}
}

static void animationEditorZoom(AnimationEditorState& state, const AnimationEditorSettings& settings, const Rect& plotRect, f32 factor, f32 anchorX)
{
	f32 oldPixelsPerSecond = std::max(state.pixelsPerSecond, 0.0001f);
	f32 anchorTime = state.scrollTime + (anchorX - plotRect.x) / (oldPixelsPerSecond * ctx->scale);
	f32 newPixelsPerSecond = animationEditorClamp(state.pixelsPerSecond * factor, settings.minPixelsPerSecond, settings.maxPixelsPerSecond);

	state.pixelsPerSecond = newPixelsPerSecond;
	state.scrollTime = anchorTime - (anchorX - plotRect.x) / (newPixelsPerSecond * ctx->scale);
}

static void animationEditorFitView(AnimationEditorState& state, const AnimationEditorSettings& settings, const Rect& plotRect)
{
	f32 timeRange = std::max(state.endTime - state.startTime, 0.001f);

	state.scrollTime = state.startTime;
	state.pixelsPerSecond = animationEditorClamp(plotRect.width / (timeRange * ctx->scale), settings.minPixelsPerSecond, settings.maxPixelsPerSecond);
}

static u32 animationEditorBuildToolbar(
	Font* font,
	const Rect& toolbarRect,
	const AnimationEditorState& state,
	AnimationEditorToolbarButton* buttons)
{
	f32 scale = ctx->scale;
	f32 x = toolbarRect.x + 4.0f * scale;
	f32 y = toolbarRect.y + 3.0f * scale;
	f32 height = std::max(toolbarRect.height - 6.0f * scale, 1.0f);
	u32 count = 0;

	auto addButton = [&](AnimationEditorToolbarAction action, const char* label, bool active, f32 gap)
	{
		x += gap * scale;
		Point textSize = animationEditorTextSize(font, label);
		f32 width = textSize.x + 16.0f * scale;

		buttons[count].action = action;
		buttons[count].rect = { x, y, width, height };
		buttons[count].label = label;
		buttons[count].active = active;
		count++;
		x += width + 2.0f * scale;
	};

	addButton(AnimationEditorTbPlay, state.playing ? "Pause" : "Play", state.playing, 0.0f);
	addButton(AnimationEditorTbKeys, "Keys", state.viewMode == AnimationEditorViewMode::Keyframes, 6.0f);
	addButton(AnimationEditorTbCurves, "Curves", state.viewMode == AnimationEditorViewMode::Curves, 6.0f);
	addButton(AnimationEditorTbAddKey, "+ Key", false, 10.0f);
	addButton(AnimationEditorTbDeleteKey, "Del", false, 3.0f);
	addButton(AnimationEditorTbSnap, "Snap", state.snapToFrames, 10.0f);
	addButton(AnimationEditorTbZoomOut, "-", false, 10.0f);
	addButton(AnimationEditorTbZoomIn, "+", false, 2.0f);
	addButton(AnimationEditorTbFit, "Fit", false, 6.0f);

	return count;
}

static AnimationEditorKeyHit animationEditorFindKeyUnderMouse(
	AnimationEditorState& state,
	const AnimationEditorSettings& settings,
	const Rect& plotRect,
	const std::vector<AnimationEditorChannelRow>& rows,
	f32 rowsTop,
	f32 rowHeight,
	f32 valueMin,
	f32 valueMax)
{
	AnimationEditorKeyHit result;
	f32 hitRadius = std::max(settings.keyHitRadius * ctx->scale, settings.keySize * ctx->scale + 2.0f * ctx->scale);
	f32 bestDistanceSquared = hitRadius * hitRadius;
	f32 scale = ctx->scale;
	Point mouse = ctx->mousePosition;

	if (state.viewMode == AnimationEditorViewMode::Keyframes)
	{
		i32 row = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

		if (row < 0 || row >= (i32)rows.size())
			return result;

		const AnimationEditorChannelRow& rowInfo = rows[row];
		const AnimationEditorChannel& channel = state.channels[rowInfo.channel];

		if (!channel.enabled)
			return result;

		f32 centerY = rowsTop + (f32)row * rowHeight + rowHeight * 0.5f;
		f32 dy = mouse.y - centerY;

		for (i32 i = 0; i < (i32)channel.keys.size(); i++)
		{
			if (rowInfo.component >= 0 && !animationEditorKeyHasComponent(channel.keys[i], rowInfo.component))
				continue;

			f32 x = animationEditorTimeToX(state, plotRect, channel.keys[i].time);
			f32 dx = mouse.x - x;
			f32 distanceSquared = dx * dx + dy * dy;

			if (distanceSquared <= bestDistanceSquared)
			{
				bestDistanceSquared = distanceSquared;
				result.channel = rowInfo.channel;
				result.key = i;
				result.component = rowInfo.component;
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
			AnimationEditorChannel& channel = state.channels[channelIndex];

			if (!channel.enabled)
				continue;

			i32 components = animationEditorComponentCount(channel.type);

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				AnimationEditorKey& key = channel.keys[keyIndex];

				if (key.time < timeLeft - 0.001f || key.time > timeRight + 0.001f)
					continue;

				f32 x = animationEditorTimeToX(state, plotRect, key.time);

				for (i32 component = 0; component < components; component++)
				{
					if (!animationEditorComponentVisible(channel, component)
						|| !animationEditorKeyHasComponent(key, component))
						continue;

					f32 y = animationEditorValueToY(plotRect, key.value[component], valueMin, valueMax);
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

static bool animationEditorKeyInsideRect(
	const AnimationEditorState& state,
	const Rect& plotRect,
	const Rect& selectionRect,
	const std::vector<AnimationEditorChannelRow>& rows,
	f32 rowsTop,
	f32 rowHeight,
	f32 valueMin,
	f32 valueMax,
	i32 channelIndex,
	i32 keyIndex)
{
	const AnimationEditorChannel& channel = state.channels[channelIndex];
	const AnimationEditorKey& key = channel.keys[keyIndex];

	if (state.viewMode == AnimationEditorViewMode::Keyframes)
	{
		f32 x = animationEditorTimeToX(state, plotRect, key.time);

		for (u32 rowIndex = 0; rowIndex < rows.size(); rowIndex++)
		{
			const AnimationEditorChannelRow& rowInfo = rows[rowIndex];

			if (rowInfo.channel != channelIndex)
				continue;

			if (rowInfo.component >= 0 && !animationEditorKeyHasComponent(key, rowInfo.component))
				continue;

			f32 y = rowsTop + (f32)rowIndex * rowHeight + rowHeight * 0.5f;

			if (selectionRect.contains({ x, y }))
				return true;
		}

		return false;
	}

	i32 components = animationEditorComponentCount(channel.type);
	f32 x = animationEditorTimeToX(state, plotRect, key.time);

	for (i32 component = 0; component < components; component++)
	{
		if (!animationEditorComponentVisible(channel, component)
			|| !animationEditorKeyHasComponent(key, component))
			continue;

		f32 y = animationEditorValueToY(plotRect, key.value[component], valueMin, valueMax);

		if (selectionRect.contains({ x, y }))
			return true;
	}

	return false;
}

bool animationEditor(const char* id, AnimationEditorState* state, f32 height)
{
	if (!state)
		return false;

	Settings& settings = ctx->settings;
	AnimationEditorSettings activeAnimationEditorSettings = settings.animationEditor;

	if (activeAnimationEditorSettings.useThemeColors)
		animationEditorApplyThemeColors(activeAnimationEditorSettings);

	const AnimationEditorSettings& animationEditorSettings = activeAnimationEditorSettings;
	f32 scale = ctx->scale;

	if (height <= 0.0f)
	{
		height = (ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y)) / scale;

		if (height <= 0.0f)
			height = animationEditorSettings.defaultHeight;
	}

	ctx->id = genId(id);
	WidgetId animationEditorId = ctx->id;

	addWidget(height);
	widgetSetFocusable();

	ctx->widget.visible = true;

	Rect outerRect = ctx->widget.rect;
	bool disabled = ctx->widget.disabled;
	bool changed = false;

	if (state->endTime <= state->startTime)
		state->endTime = state->startTime + 1.0f;

	if (state->pixelsPerSecond <= 0.0f)
		state->pixelsPerSecond = animationEditorSettings.defaultPixelsPerSecond;

	state->pixelsPerSecond = animationEditorClamp(state->pixelsPerSecond, animationEditorSettings.minPixelsPerSecond, animationEditorSettings.maxPixelsPerSecond);

	if (state->frameRate <= 0.0f)
		state->frameRate = 30.0f;

	if (state->selectedChannel < -1 || state->selectedChannel >= (i32)state->channels.size())
		state->selectedChannel = -1;

	state->cursorTime = animationEditorClamp(state->cursorTime, state->startTime, state->endTime);

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

	auto& runtime = ctx->animationEditorStates[animationEditorId];
	f32 toolbarHeight = animationEditorSettings.showToolbar ? animationEditorSettings.toolbarHeight * scale : 0.0f;
	f32 rulerHeight = animationEditorSettings.rulerHeight * scale;
	f32 rowHeight = animationEditorSettings.rowHeight * scale;

	if (runtime.channelListWidth <= 0.0f)
		runtime.channelListWidth = animationEditorSettings.channelListWidth;

	f32 minChannelWidth = std::min(80.0f * scale, outerRect.width * 0.5f);
	f32 maxChannelWidth = std::max(minChannelWidth, outerRect.width - std::min(120.0f * scale, outerRect.width * 0.5f));
	f32 channelWidth = animationEditorClamp(runtime.channelListWidth * scale, minChannelWidth, maxChannelWidth);

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

	f32 axisWidth = state->viewMode == AnimationEditorViewMode::Curves ? animationEditorSettings.valueAxisWidth * scale : 0.0f;

	axisWidth = std::min(axisWidth, canvasRect.width * 0.6f);

	Rect plotRect = { canvasRect.x + axisWidth, canvasRect.y, canvasRect.width - axisWidth, canvasRect.height };

	plotRect.width = std::max(plotRect.width, 1.0f);

	f32 valueMin, valueMax;

	if (runtime.valueRangeModeInitialized && !state->autoValueRange && runtime.wasAutoValueRange)
		animationEditorComputeAutoValueRange(*state, state->valueMin, state->valueMax);

	runtime.wasAutoValueRange = state->autoValueRange;
	runtime.valueRangeModeInitialized = true;
	animationEditorComputeValueRange(*state, valueMin, valueMax);

	animationEditorClampChannelScroll(*state, listRect, animationEditorSettings);
	animationEditorClampView(*state, plotRect, animationEditorSettings);

	std::vector<AnimationEditorChannelRow> channelRows;
	animationEditorBuildChannelRows(*state, channelRows);

	AnimationEditorChannelScrollbar channelScrollbar = animationEditorGetChannelScrollbar(*state, animationEditorSettings, listRect, rowHeight);

	Font* font = ctx->theme->getFont("normal");

	if (!font && !ctx->theme->fonts.empty())
		font = &ctx->theme->fonts[0]->font;

	AnimationEditorToolbarButton toolbarButtons[AnimationEditorTbCount];
	u32 toolbarButtonCount = animationEditorSettings.showToolbar
		? animationEditorBuildToolbar(font, toolbarRect, *state, toolbarButtons)
		: 0;

	Point mouse = ctx->mousePosition;
	bool mouseInAnimationEditor = outerRect.contains(mouse) && ctx->hoveringThisWindow && ctx->isActiveLayer();
	InputEvent event = ctx->event;
	bool consumed = false;

	if (mouseInAnimationEditor && !ctx->widget.captureId)
	{
		ctx->widget.hoveredId = animationEditorId;
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = outerRect;
	}

	i32 hoveredToolbarButton = -1;

	if (mouseInAnimationEditor && (!ctx->widget.captureId || runtime.pressedToolbarButton >= 0))
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

	AnimationEditorToolbarField toolbarFields[8];
	u32 toolbarFieldCount = 0;

	auto focusAnimationEditor = [&]()
	{
		ctx->widget.focusedId = animationEditorId;
		ctx->widget.focused = true;
		ctx->focusChanged = true;
	};

	auto beginCapture = [&]()
	{
		focusAnimationEditor();
		ctx->widget.captureId = animationEditorId;
		windowSetCapture();
	};

	auto endCapture = [&]()
	{
		ctx->widget.captureId = 0;
		windowReleaseCapture();
	};

	auto applyToolbarAction = [&](AnimationEditorToolbarAction action)
	{
		AnimationEditorAction publicAction = AnimationEditorAction::Play;
		bool mapped = true;

		switch (action)
		{
		case AnimationEditorTbPlay: publicAction = AnimationEditorAction::Play; break;
		case AnimationEditorTbKeys: publicAction = AnimationEditorAction::KeyframesMode; break;
		case AnimationEditorTbCurves: publicAction = AnimationEditorAction::CurvesMode; break;
		case AnimationEditorTbAddKey: publicAction = AnimationEditorAction::AddKey; break;
		case AnimationEditorTbDeleteKey: publicAction = AnimationEditorAction::DeleteSelected; break;
		case AnimationEditorTbSnap: publicAction = AnimationEditorAction::ToggleSnap; break;
		case AnimationEditorTbZoomOut: publicAction = AnimationEditorAction::ZoomOut; break;
		case AnimationEditorTbZoomIn: publicAction = AnimationEditorAction::ZoomIn; break;
		case AnimationEditorTbFit: publicAction = AnimationEditorAction::FitView; break;
		default: mapped = false; break;
		}

		if (mapped && animationEditorAction(state, publicAction, plotRect.width))
			changed = true;
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

		if (!consumed && mouseInAnimationEditor && event.type == InputEvent::Type::MouseWheel)
		{
			if (canvasRect.contains(mouse) || rulerRect.contains(mouse))
			{
				f32 factor = event.mouse.wheel.y > 0.0f ? animationEditorSettings.zoomStep : 1.0f / animationEditorSettings.zoomStep;

				animationEditorZoom(*state, animationEditorSettings, plotRect, factor, mouse.x);

				if (!state->autoValueRange && state->viewMode == AnimationEditorViewMode::Curves && plotRect.height > 0.0f)
				{
					f32 range = std::max(valueMax - valueMin, 0.0001f) / factor;
					f32 ratio = animationEditorClamp((plotRect.bottom() - mouse.y) / plotRect.height, 0.0f, 1.0f);
					f32 valueAtMouse = animationEditorYToValue(plotRect, mouse.y, valueMin, valueMax);

					range = std::max(range, 0.0001f);
					state->valueMin = valueAtMouse - ratio * range;
					state->valueMax = state->valueMin + range;
				}

				animationEditorClampView(*state, plotRect, animationEditorSettings);
			}
			else if (listRect.contains(mouse))
			{
				state->channelScroll -= event.mouse.wheel.y * animationEditorSettings.rowHeight * 0.75f;
				animationEditorClampChannelScroll(*state, listRect, animationEditorSettings);
			}

			consumed = true;
		}

		if (!consumed && mouseInAnimationEditor && event.type == InputEvent::Type::MouseDown)
		{
			if (event.mouse.button == MouseButton::Middle
				&& (canvasRect.contains(mouse) || rulerRect.contains(mouse)))
			{
				runtime.dragType = AnimationEditorWidgetState::DragType::Pan;
				runtime.dragStartMouse = mouse;
				runtime.panStartScrollTime = state->scrollTime;
				runtime.panStartValueMin = valueMin;
				runtime.panStartValueMax = valueMax;
				beginCapture();
				consumed = true;
			}
			else if (event.mouse.button == MouseButton::Left)
			{
				AnimationEditorTangentHit tangentHit;

				if (state->viewMode == AnimationEditorViewMode::Curves && canvasRect.contains(mouse))
					tangentHit = animationEditorFindTangentHandle(*state, animationEditorSettings, plotRect, valueMin, valueMax, mouse);

				if (tangentHit.channel >= 0)
				{
					AnimationEditorChannel& channel = state->channels[tangentHit.channel];

					animationEditorMakeKeyTangentsManual(channel, tangentHit.key);
					runtime.dragType = tangentHit.outgoing
						? AnimationEditorWidgetState::DragType::TangentOut
						: AnimationEditorWidgetState::DragType::TangentIn;
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
					runtime.dragType = AnimationEditorWidgetState::DragType::ChannelResize;
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
							f32 ratio = animationEditorClamp(
								(mouse.y - channelScrollbar.track.y - channelScrollbar.thumb.height * 0.5f) / travel,
								0.0f,
								1.0f);

							state->channelScroll = ratio * channelScrollbar.maxScroll;
						}

						runtime.dragType = AnimationEditorWidgetState::DragType::ChannelScroll;
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
							const AnimationEditorChannelRow& rowInfo = channelRows[rowIndex];
							AnimationEditorChannel& channel = state->channels[rowInfo.channel];
							bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

							if (rowInfo.component < 0 && animationEditorChannelCheckboxRect(rowRect, scale).contains(mouse))
							{
								channel.enabled = !channel.enabled;
								changed = true;
							}
							else if (rowInfo.component < 0
								&& animationEditorComponentCount(channel.type) > 1
								&& animationEditorChannelExpandRect(rowRect, scale).contains(mouse))
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
									animationEditorClearSelection(*state);
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
									animationEditorClearSelection(*state);
								}
							}

							focusAnimationEditor();
							consumed = true;
						}
					}
				}
				else if (rulerRect.contains(mouse))
				{
					state->cursorTime = animationEditorSnapTime(*state, animationEditorXToTime(*state, plotRect, mouse.x));
					runtime.dragType = AnimationEditorWidgetState::DragType::Playhead;
					beginCapture();
					changed = true;
					consumed = true;
				}
				else if (canvasRect.contains(mouse))
				{
					f32 rowsTop = canvasRect.y - state->channelScroll * scale;
					AnimationEditorKeyHit hit = animationEditorFindKeyUnderMouse(*state, animationEditorSettings, plotRect, channelRows, rowsTop, rowHeight, valueMin, valueMax);

					if (hit.channel >= 0)
					{
						AnimationEditorChannel& channel = state->channels[hit.channel];
						bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);
						bool onComponentLane = state->viewMode == AnimationEditorViewMode::Keyframes && hit.component >= 0;
						i32 keyIndex = hit.key;

						// clicking a key on a component lane splits it so the
						// component gets its own independent key
						if (onComponentLane)
							keyIndex = animationEditorSplitKey(channel, keyIndex, hit.component);

						AnimationEditorKey& key = channel.keys[keyIndex];

						if (onComponentLane)
						{
							// select just the lane component
							if (controlDown)
							{
								key.selectedComponents ^= (1u << hit.component);
								changed = true;
							}
							else if (!(key.selectedComponents & (1u << hit.component)))
							{
								animationEditorClearSelection(*state);
								key.selectedComponents = (1u << hit.component);
								changed = true;
							}

							state->selectedChannel = hit.channel;
						}
						else
						{
							// whole key selection (parent lane or curves view)
							if (controlDown)
							{
								key.selected = !key.selected;
								state->selectedChannel = hit.channel;
								changed = true;
							}
							else if (!key.selected)
							{
								animationEditorClearSelection(*state);
								key.selected = true;
								state->selectedChannel = hit.channel;
								changed = true;
							}
							else
							{
								state->selectedChannel = hit.channel;
							}
						}

						if (key.selected || key.selectedComponents != 0)
						{
							runtime.dragType = AnimationEditorWidgetState::DragType::Keys;
							runtime.dragChannel = hit.channel;
							runtime.dragComponent = hit.component;
							runtime.dragStartMouse = mouse;
							runtime.dragStartTime = animationEditorXToTime(*state, plotRect, mouse.x);
							runtime.dragValueMin = valueMin;
							runtime.dragValueMax = valueMax;
							runtime.dragStartValue = animationEditorYToValue(plotRect, mouse.y, valueMin, valueMax);
							runtime.dragStartEndTime = state->endTime;
							runtime.pushAppliedDelta = 0.0f;
							runtime.dragItems.clear();

							for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
							{
								AnimationEditorChannel& selectedChannel = state->channels[channelIndex];

								if (!selectedChannel.enabled)
									continue;

								for (i32 selectedKeyIndex = 0; selectedKeyIndex < (i32)selectedChannel.keys.size(); selectedKeyIndex++)
								{
									AnimationEditorKey& selectedKey = selectedChannel.keys[selectedKeyIndex];

									if (!selectedKey.selected && selectedKey.selectedComponents == 0)
										continue;

									AnimationEditorKeyDragItem item;

									item.channel = channelIndex;
									item.key = selectedKeyIndex;
									item.time = selectedKey.time;

									for (i32 i = 0; i < 4; i++)
										item.value[i] = selectedKey.value[i];

									runtime.dragItems.push_back(item);
								}
							}

							runtime.rippleItems.clear();

							if (state->ripple)
							{
								for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
								{
									AnimationEditorChannel& rippleChannel = state->channels[channelIndex];
									f32 minSelectedTime = FLT_MAX;
									f32 maxSelectedTime = -FLT_MAX;
									bool hasSelectedKey = false;

									if (!rippleChannel.enabled)
										continue;

									for (auto& rippleKey : rippleChannel.keys)
									{
										if (rippleKey.selected || rippleKey.selectedComponents != 0)
										{
											hasSelectedKey = true;
											minSelectedTime = std::min(minSelectedTime, rippleKey.time);
											maxSelectedTime = std::max(maxSelectedTime, rippleKey.time);
										}
									}

									if (!hasSelectedKey)
										continue;

									for (i32 keyIndex = 0; keyIndex < (i32)rippleChannel.keys.size(); keyIndex++)
									{
										AnimationEditorKey& key = rippleChannel.keys[keyIndex];

										if (key.selected || key.selectedComponents != 0)
											continue;

										bool leftSide = key.time < minSelectedTime;
										bool rightSide = key.time > maxSelectedTime;

										if (!leftSide && !rightSide)
											continue;

										AnimationEditorKeyDragItem item;

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
						i32 component = -1;

						if (state->viewMode == AnimationEditorViewMode::Keyframes)
						{
							i32 row = (i32)floorf((mouse.y - rowsTop) / std::max(rowHeight, 1.0f));

							if (row >= 0 && row < (i32)channelRows.size())
							{
								const AnimationEditorChannelRow& rowInfo = channelRows[row];

								if (state->channels[rowInfo.channel].enabled)
								{
									channelIndex = rowInfo.channel;
									component = rowInfo.component;
								}
							}
						}
						else
						{
							channelIndex = animationEditorTargetChannel(*state);

							if (channelIndex >= 0)
							{
								AnimationEditorChannel& channel = state->channels[channelIndex];
								i32 components = animationEditorComponentCount(channel.type);
								f32 bestDistance = FLT_MAX;

								for (i32 i = 0; i < components; i++)
								{
									f32 value = animationEditorEvaluateComponent(channel, i, animationEditorXToTime(*state, plotRect, mouse.x));
									f32 y = animationEditorValueToY(plotRect, value, valueMin, valueMax);
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
							f32 addTime = animationEditorSnapTime(*state, animationEditorXToTime(*state, plotRect, mouse.x));
							u32 componentMask = 0;

							animationEditorEvaluateChannel(state->channels[channelIndex], addTime, values);

							if (component >= 0)
							{
								if (state->viewMode == AnimationEditorViewMode::Curves)
									values[component] = animationEditorYToValue(plotRect, mouse.y, valueMin, valueMax);

								componentMask = (1u << component);
							}

							animationEditorAddKey(*state, channelIndex, addTime, values, componentMask, true);
							changed = true;
						}

						consumed = true;
					}
					else
					{
						f32 playheadX = animationEditorTimeToX(*state, plotRect, state->cursorTime);

						if (fabsf(mouse.x - playheadX) <= 5.0f * scale)
						{
							runtime.dragType = AnimationEditorWidgetState::DragType::Playhead;
							beginCapture();
							changed = true;
						}
						else
						{
							bool controlDown = has(event.mouse.modifiers, KeyModifiers::Control);

							if (!controlDown)
							{
								animationEditorClearSelection(*state);
								changed = true;
							}

							runtime.dragType = AnimationEditorWidgetState::DragType::BoxSelect;
							runtime.boxStart = mouse;
							runtime.boxEnd = mouse;
							beginCapture();
						}

						consumed = true;
					}
				}
			}
		}

		if (event.type == InputEvent::Type::MouseMove && runtime.dragType != AnimationEditorWidgetState::DragType::None)
		{
			switch (runtime.dragType)
			{
			case AnimationEditorWidgetState::DragType::Playhead:
				state->cursorTime = animationEditorSnapTime(*state, animationEditorXToTime(*state, plotRect, mouse.x));
				changed = true;
				consumed = true;
				break;

			case AnimationEditorWidgetState::DragType::Pan:
				state->scrollTime = runtime.panStartScrollTime - (mouse.x - runtime.dragStartMouse.x) / (state->pixelsPerSecond * scale);

				if (!state->autoValueRange && state->viewMode == AnimationEditorViewMode::Curves && plotRect.height > 0.0f)
				{
					f32 range = runtime.panStartValueMax - runtime.panStartValueMin;
					f32 valueDelta = (mouse.y - runtime.dragStartMouse.y) / plotRect.height * range;

					state->valueMin = runtime.panStartValueMin + valueDelta;
					state->valueMax = runtime.panStartValueMax + valueDelta;
				}

				animationEditorClampView(*state, plotRect, animationEditorSettings);
				changed = true;
				consumed = true;
				break;

			case AnimationEditorWidgetState::DragType::ChannelResize:
			{
				f32 minWidth = 80.0f;
				f32 maxWidth = std::max(minWidth, outerRect.width / scale - std::min(120.0f, outerRect.width / scale * 0.5f));

				runtime.channelListWidth = animationEditorClamp((mouse.x - outerRect.x) / scale, minWidth, maxWidth);
				consumed = true;
				break;
			}

			case AnimationEditorWidgetState::DragType::ChannelScroll:
			{
				AnimationEditorChannelScrollbar scrollbar = animationEditorGetChannelScrollbar(*state, animationEditorSettings, listRect, rowHeight);
				f32 travel = std::max(scrollbar.track.height - scrollbar.thumb.height, 1.0f);

				if (scrollbar.maxScroll > 0.0f)
					state->channelScroll = animationEditorClamp(
						runtime.dragStartChannelScroll + (mouse.y - runtime.dragStartMouse.y) / travel * scrollbar.maxScroll,
						0.0f,
						scrollbar.maxScroll);

				consumed = true;
				break;
			}

			case AnimationEditorWidgetState::DragType::TangentIn:
			case AnimationEditorWidgetState::DragType::TangentOut:
			{
				if (runtime.dragChannel < 0 || runtime.dragChannel >= (i32)state->channels.size())
					break;

				AnimationEditorChannel& channel = state->channels[runtime.dragChannel];

				if (runtime.dragKey < 0 || runtime.dragKey >= (i32)channel.keys.size()
					|| runtime.dragComponent < 0 || runtime.dragComponent >= 4)
					break;

				AnimationEditorKey& key = channel.keys[runtime.dragKey];
				f32 mouseTime = animationEditorXToTime(*state, plotRect, mouse.x);
				f32 mouseValue = animationEditorYToValue(plotRect, mouse.y, runtime.dragValueMin, runtime.dragValueMax);
				bool outgoing = runtime.dragType == AnimationEditorWidgetState::DragType::TangentOut;
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

			case AnimationEditorWidgetState::DragType::Keys:
			{
				f32 newTime = animationEditorXToTime(*state, plotRect, mouse.x);
				f32 timeDelta = newTime - runtime.dragStartTime;
				f32 valueDelta = 0.0f;

				if (state->viewMode == AnimationEditorViewMode::Curves && runtime.dragComponent >= 0)
					valueDelta = animationEditorYToValue(plotRect, mouse.y, runtime.dragValueMin, runtime.dragValueMax) - runtime.dragStartValue;

				bool moved = false;
				f32 newEndTime = runtime.dragStartEndTime;

				if (state->push && !runtime.dragItems.empty())
				{
					// push mode: the dragged keys push the keys they reach, the
					// pass works on the current key times so only the delta since
					// the last frame is applied, otherwise the drag would speed up
					f32 requestedDelta = timeDelta;

					if (state->snapToFrames && state->frameRate > 0.0f)
						requestedDelta = roundf(requestedDelta * state->frameRate) / state->frameRate;

					f32 pushDelta = requestedDelta - runtime.pushAppliedDelta;

					// clamp the delta so the pushed keys stay in the time range
					if (!state->extendEndTime && pushDelta != 0.0f)
					{
						for (i32 iteration = 0; iteration < 4; iteration++)
						{
							f32 overflow = 0.0f;

							for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
							{
								AnimationEditorChannel& channel = state->channels[channelIndex];

								if (!channel.enabled)
									continue;

								f32 minTime, maxTime;

								animationEditorPushChannel(channel, runtime.dragItems, channelIndex, pushDelta, false, minTime, maxTime);

								if (maxTime <= -FLT_MAX / 2.0f)
									continue;

								if (pushDelta > 0.0f)
									overflow = std::max(overflow, maxTime - state->endTime);
								else
									overflow = std::max(overflow, state->startTime - minTime);
							}

							if (overflow <= 0.0001f)
								break;

							pushDelta = pushDelta > 0.0f ? pushDelta - overflow : pushDelta + overflow;
						}
					}

					if (pushDelta != 0.0f)
					{
						f32 pushedMaxTime = -FLT_MAX;

						for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
						{
							AnimationEditorChannel& channel = state->channels[channelIndex];

							if (!channel.enabled)
								continue;

							f32 minTime, maxTime;

							animationEditorPushChannel(channel, runtime.dragItems, channelIndex, pushDelta, true, minTime, maxTime);
							pushedMaxTime = std::max(pushedMaxTime, maxTime);
						}

						moved = true;
						runtime.pushAppliedDelta += pushDelta;

						if (state->extendEndTime && pushedMaxTime > state->endTime)
							state->endTime = pushedMaxTime;
					}

					// the value delta still applies to the dragged keys in curves view
					if (valueDelta != 0.0f
						&& runtime.dragChannel >= 0
						&& runtime.dragChannel < (i32)state->channels.size()
						&& runtime.dragComponent >= 0)
					{
						AnimationEditorChannel& channel = state->channels[runtime.dragChannel];

						if (runtime.dragComponent < animationEditorComponentCount(channel.type))
						{
							for (auto& item : runtime.dragItems)
							{
								if (item.channel != runtime.dragChannel || item.key < 0 || item.key >= (i32)channel.keys.size())
									continue;

								AnimationEditorKey& key = channel.keys[item.key];
								f32 value = animationEditorQuantizeComponent(channel.type, item.value[runtime.dragComponent] + valueDelta, runtime.dragComponent);

								if (value != key.value[runtime.dragComponent])
								{
									key.value[runtime.dragComponent] = value;
									moved = true;
								}
							}
						}
					}
				}
				else
				{
					for (auto& item : runtime.dragItems)
					{
						if (item.channel < 0 || item.channel >= (i32)state->channels.size())
							continue;

						AnimationEditorChannel& channel = state->channels[item.channel];

						if (!channel.enabled || item.key < 0 || item.key >= (i32)channel.keys.size())
							continue;

						AnimationEditorKey& key = channel.keys[item.key];
						f32 time = state->extendEndTime
							? animationEditorSnapTimeNoMax(*state, item.time + timeDelta)
							: animationEditorSnapTime(*state, item.time + timeDelta);

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
							&& runtime.dragComponent < animationEditorComponentCount(channel.type))
						{
							f32 value = animationEditorQuantizeComponent(channel.type, item.value[runtime.dragComponent] + valueDelta, runtime.dragComponent);

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

						AnimationEditorChannel& channel = state->channels[item.channel];

						if (item.key < 0 || item.key >= (i32)channel.keys.size())
							continue;

						AnimationEditorKey& key = channel.keys[item.key];
						f32 itemDelta = item.leftSide ? std::min(timeDelta, 0.0f) : std::max(timeDelta, 0.0f);
						f32 time = state->extendEndTime
							? animationEditorSnapTimeNoMax(*state, item.time + itemDelta)
							: animationEditorSnapTime(*state, item.time + itemDelta);

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
				}

				if (moved)
				{
					animationEditorReorderDraggedKeys(runtime, state->channels);
					changed = true;
					ctx->widget.changeEnded = true;
				}

				consumed = true;
				break;
			}

			case AnimationEditorWidgetState::DragType::BoxSelect:
				runtime.boxEnd = mouse;
				consumed = true;
				break;

			default:
				break;
			}
		}

		if (event.type == InputEvent::Type::MouseUp && runtime.dragType != AnimationEditorWidgetState::DragType::None)
		{
			if (runtime.dragType == AnimationEditorWidgetState::DragType::Keys)
			{
				for (auto& channel : state->channels)
					animationEditorSortKeys(channel);
			}
			else if (runtime.dragType == AnimationEditorWidgetState::DragType::BoxSelect)
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
						AnimationEditorChannel& channel = state->channels[channelIndex];

						if (!channel.enabled)
							continue;

						for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
						{
							if (animationEditorKeyInsideRect(*state, plotRect, selectionRect, channelRows, rowsTop, rowHeight, valueMin, valueMax, channelIndex, keyIndex))
							{
								channel.keys[keyIndex].selected = true;
								state->selectedChannel = channelIndex;
							}
						}
					}

					changed = true;
				}
			}

			runtime.dragType = AnimationEditorWidgetState::DragType::None;
			runtime.dragChannel = -1;
			runtime.dragKey = -1;
			runtime.dragComponent = -1;
			runtime.pushAppliedDelta = 0.0f;
			runtime.dragItems.clear();
			runtime.rippleItems.clear();
			endCapture();
			consumed = true;
		}

		if (!consumed
			&& event.type == InputEvent::Type::Key
			&& event.key.down
			&& !ctx->textInput.id
			&& (ctx->widget.focusedId == animationEditorId
				|| (event.key.code == KeyCode::Space && mouseInAnimationEditor)))
		{
			bool controlDown = has(event.key.modifiers, KeyModifiers::Control);
			bool shiftDown = has(event.key.modifiers, KeyModifiers::Shift);

			switch (event.key.code)
			{
			case KeyCode::Delete:
			case KeyCode::Backspace:
				if (animationEditorDeleteSelectedKeys(*state) > 0)
					changed = true;

				consumed = true;
				break;

			case KeyCode::A:
				if (controlDown)
				{
					animationEditorSelectAll(*state);
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

				animationEditorNudgeSelectedKeys(*state, delta);
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
				animationEditorClearSelection(*state);
				runtime.dragType = AnimationEditorWidgetState::DragType::None;
				runtime.pushAppliedDelta = 0.0f;
				runtime.dragItems.clear();
				runtime.rippleItems.clear();
				endCapture();
				changed = true;
				consumed = true;
				break;

			case KeyCode::F:
				animationEditorFitView(*state, animationEditorSettings, plotRect);
				animationEditorClampView(*state, plotRect, animationEditorSettings);
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
	axisWidth = state->viewMode == AnimationEditorViewMode::Curves ? animationEditorSettings.valueAxisWidth * scale : 0.0f;
	axisWidth = std::min(axisWidth, canvasRect.width * 0.6f);
	plotRect = { canvasRect.x + axisWidth, canvasRect.y, canvasRect.width - axisWidth, canvasRect.height };
	plotRect.width = std::max(plotRect.width, 1.0f);

	animationEditorClampView(*state, plotRect, animationEditorSettings);

	toolbarButtonCount = animationEditorBuildToolbar(font, toolbarRect, *state, toolbarButtons);
	toolbarFieldCount = animationEditorBuildToolbarFields(toolbarRect, toolbarButtons, toolbarButtonCount, animationEditorSettings, *state, toolbarFields, 8);

	if (runtime.dragType == AnimationEditorWidgetState::DragType::Keys && state->viewMode == AnimationEditorViewMode::Curves)
	{
		valueMin = runtime.dragValueMin;
		valueMax = runtime.dragValueMax;
	}
	else
	{
		animationEditorComputeValueRange(*state, valueMin, valueMax);
	}

	// drawing
	ctx->renderer.pushClipRect(outerRect);

	Color backgroundColor = animationEditorSettings.backgroundColor;
	Color headerColor = animationEditorSettings.headerColor;
	Color channelListColor = animationEditorSettings.channelListColor;
	Color textColor = animationEditorSettings.textColor;
	Color dimTextColor = animationEditorSettings.dimTextColor;

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

	if (state->viewMode == AnimationEditorViewMode::Curves && axisWidth > 0.0f)
	{
		ctx->renderer.cmdSetColor(channelListColor);
		ctx->renderer.cmdDrawFilledRectangle({ canvasRect.x, canvasRect.y, axisWidth, canvasRect.height });
	}

	ctx->renderer.pushClipRect(toolbarRect);

	for (u32 i = 0; i < toolbarButtonCount; i++)
	{
		bool hovered = (i32)i == hoveredToolbarButton;
		bool pressed = (i32)i == runtime.pressedToolbarButton;

		animationEditorDrawButton(font, toolbarButtons[i].rect, toolbarButtons[i].label, toolbarButtons[i].active, hovered, pressed, animationEditorSettings.toolbarToggleColor);
	}

	if (toolbarFieldCount == 0 && toolbarRect.width > 520.0f * scale)
	{
		i32 totalKeys = 0;

		for (auto& channel : state->channels)
			totalKeys += (i32)channel.keys.size();

		char status[160];

		snprintf(status, sizeof(status), "t %.3fs   keys %d   zoom %.0f%%", state->cursorTime, totalKeys, state->pixelsPerSecond / animationEditorSettings.defaultPixelsPerSecond * 100.0f);

		Rect statusRect = { toolbarRect.right() - 330.0f * scale, toolbarRect.y, 320.0f * scale, toolbarRect.height };

		animationEditorDrawText(font, status, statusRect, dimTextColor, HAlignType::Right);
	}

	ctx->renderer.popClipRect();

	animationEditorDrawText(font, "Channels", { listHeaderRect.x + 8.0f * scale, listHeaderRect.y, std::max(listHeaderRect.width - 64.0f * scale, 0.0f), listHeaderRect.height }, textColor, HAlignType::Left);

	{
		char countText[32];

		snprintf(countText, sizeof(countText), "%u", (u32)state->channels.size());
		animationEditorDrawText(font, countText, { listHeaderRect.right() - 56.0f * scale, listHeaderRect.y, 48.0f * scale, listHeaderRect.height }, dimTextColor, HAlignType::Right);
	}

	f32 listScrollOffset = state->channelScroll * scale;
	f32 rowsTop = listRect.y - listScrollOffset;

	ctx->renderer.pushClipRect(listRect);

	for (u32 rowIndex = 0; rowIndex < channelRows.size(); rowIndex++)
	{
		const AnimationEditorChannelRow& rowInfo = channelRows[rowIndex];
		AnimationEditorChannel& channel = state->channels[rowInfo.channel];
		Rect rowRect = { listRect.x, rowsTop + (f32)rowIndex * rowHeight, listRect.width, rowHeight };

		if (rowRect.bottom() < listRect.y || rowRect.y > listRect.bottom())
			continue;

		bool isParent = rowInfo.component < 0;
		bool visible = isParent ? channel.selected : animationEditorComponentVisible(channel, rowInfo.component);

		if (visible)
		{
			ctx->renderer.cmdSetColor(animationEditorSettings.rowSelectedColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}
		else if (rowIndex % 2 != 0)
		{
			ctx->renderer.cmdSetColor(animationEditorSettings.rowAlternateColor);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}

		if (!channel.enabled)
		{
			Color overlay = animationEditorSettings.disabledOverlayColor;

			overlay.a *= 0.5f;
			ctx->renderer.cmdSetColor(overlay);
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
		}

		Color channelColor = animationEditorChannelColor(rowInfo.channel);
		Color nameColor = channel.enabled ? textColor : dimTextColor;

		if (isParent)
		{
			Rect checkboxRect = animationEditorChannelCheckboxRect(rowRect, scale);
			Rect badgeRect = animationEditorChannelBadgeRect(rowRect, scale);
			Rect nameRect = { badgeRect.right() + 6.0f * scale, rowRect.y, rowRect.width - (badgeRect.right() - rowRect.x) - 56.0f * scale, rowRect.height };

			nameRect.width = std::max(nameRect.width, 0.0f);

			if (!channel.enabled)
				channelColor.a *= 0.45f;

			animationEditorDrawCheckbox(checkboxRect, channel.enabled, disabled);

			if (animationEditorComponentCount(channel.type) > 1)
			{
				Rect expandRect = animationEditorChannelExpandRect(rowRect, scale);
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
			animationEditorDrawText(font, animationEditorTypeBadge(channel.type), badgeRect, channelColor, HAlignType::Center, false);

			const char* channelName = channel.name.empty() ? "Unnamed" : channel.name.c_str();

			animationEditorDrawText(font, channelName, nameRect, nameColor, HAlignType::Left);

			char keyCount[32];

			snprintf(keyCount, sizeof(keyCount), "%u", (u32)channel.keys.size());
			animationEditorDrawText(font, keyCount, { rowRect.right() - 46.0f * scale, rowRect.y, 28.0f * scale, rowRect.height }, dimTextColor, HAlignType::Right);
		}
		else
		{
			i32 component = rowInfo.component;
			const char* axis = animationEditorComponentLabel(channel.type, component);
			char label[128];

			snprintf(label, sizeof(label), "%s.%s", channel.name.empty() ? "Unnamed" : channel.name.c_str(), axis);

			Color componentColor = animationEditorChannelColor(rowInfo.channel, component);
			Point dot = { rowRect.x + 64.0f * scale, rowRect.y + rowRect.height * 0.5f };

			animationEditorDrawDiamond(dot, 3.5f * scale, componentColor);
			animationEditorDrawText(font, label, { rowRect.x + 74.0f * scale, rowRect.y, rowRect.width - 74.0f * scale - 110.0f * scale, rowRect.height }, nameColor, HAlignType::Left);

			char valueText[32];
			f32 value = animationEditorEvaluateComponent(channel, component, state->cursorTime);

			if (channel.type == AnimationEditorKeyType::Int)
				snprintf(valueText, sizeof(valueText), "%.0f", value);
			else
				snprintf(valueText, sizeof(valueText), "%.3f", value);

			animationEditorDrawText(font, valueText, { rowRect.right() - 104.0f * scale, rowRect.y, 56.0f * scale, rowRect.height }, dimTextColor, HAlignType::Right);
		}

		{
			Point dot = { rowRect.right() - 14.0f * scale, rowRect.y + rowRect.height * 0.5f };
			Color dotColor = animationEditorChannelColor(rowInfo.channel, rowInfo.component);
			bool selected = isParent
				? channel.selected
				: animationEditorComponentVisible(channel, rowInfo.component);

			if (!selected)
			{
				dotColor = animationEditorSettings.dimTextColor;
				dotColor.a = 0.35f;
			}

			animationEditorDrawDiamond(dot, 4.0f * scale, dotColor);
		}

		ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
	}

	ctx->renderer.popClipRect();

	// channel list scrollbar
	{
		AnimationEditorChannelScrollbar scrollbar = animationEditorGetChannelScrollbar(*state, animationEditorSettings, listRect, rowHeight);

		if (scrollbar.maxScroll > 0.0f)
		{
			ctx->renderer.cmdSetColor(Color{ animationEditorSettings.textColor.r, animationEditorSettings.textColor.g, animationEditorSettings.textColor.b, 0.1f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.track);
			ctx->renderer.cmdSetColor(Color{ animationEditorSettings.textColor.r, animationEditorSettings.textColor.g, animationEditorSettings.textColor.b, 0.4f });
			ctx->renderer.cmdDrawFilledRectangle(scrollbar.thumb);
		}
	}

	// keyframe row backgrounds
	if (state->viewMode == AnimationEditorViewMode::Keyframes)
	{
		ctx->renderer.pushClipRect(canvasRect);

		for (u32 rowIndex = 0; rowIndex < channelRows.size(); rowIndex++)
		{
			const AnimationEditorChannelRow& rowInfo = channelRows[rowIndex];
			const AnimationEditorChannel& rowChannel = state->channels[rowInfo.channel];
			Rect rowRect = { canvasRect.x, rowsTop + (f32)rowIndex * rowHeight, canvasRect.width, rowHeight };

			if (rowRect.bottom() < canvasRect.y || rowRect.y > canvasRect.bottom())
				continue;

			bool rowSelected = rowInfo.component < 0
				? rowChannel.selected
				: animationEditorComponentVisible(rowChannel, rowInfo.component);

			if (rowSelected)
			{
				ctx->renderer.cmdSetColor(animationEditorSettings.rowSelectedColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}
			else if (rowIndex % 2 != 0)
			{
				ctx->renderer.cmdSetColor(animationEditorSettings.rowAlternateColor);
				ctx->renderer.cmdDrawFilledRectangle(rowRect);
			}

			ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.gridColor, scale));
			ctx->renderer.cmdDrawLine({ rowRect.x, rowRect.bottom() }, { rowRect.right(), rowRect.bottom() });
		}

		ctx->renderer.popClipRect();
	}

	// time grid and ruler
	f32 pixelsPerSecond = state->pixelsPerSecond * scale;
	f32 timeLeft = state->scrollTime;
	f32 timeRight = timeLeft + plotRect.width / std::max(pixelsPerSecond, 0.0001f);
	f32 majorStep = animationEditorNiceStep(80.0f * scale / std::max(pixelsPerSecond, 0.0001f));
	f32 minorStep = majorStep / 5.0f;
	i32 decimals = (i32)ceilf(-log10f(majorStep));

	decimals = (i32)animationEditorClamp((f32)decimals, 0.0f, 4.0f);

	ctx->renderer.pushClipRect(canvasRect);

	i32 minorStart = (i32)floorf(timeLeft / minorStep);
	i32 minorEnd = (i32)ceilf(timeRight / minorStep);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		f32 time = (f32)i * minorStep;
		f32 x = animationEditorTimeToX(*state, plotRect, time);

		if (x < plotRect.x - 1.0f || x > plotRect.right() + 1.0f)
			continue;

		bool major = (i % 5) == 0;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? animationEditorSettings.gridStrongColor : animationEditorSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, canvasRect.y }, { x, canvasRect.bottom() });
	}

	ctx->renderer.popClipRect();

	// value grid in curves mode
	if (state->viewMode == AnimationEditorViewMode::Curves)
	{
		ctx->renderer.pushClipRect(canvasRect);

		f32 valueRange = valueMax - valueMin;
		f32 valueStep = animationEditorNiceStep(valueRange / std::max(2.0f, floorf(plotRect.height / (34.0f * scale))));
		i32 valueDecimals = (i32)ceilf(-log10f(valueStep));

		valueDecimals = (i32)animationEditorClamp((f32)valueDecimals, 0.0f, 4.0f);

		i32 valueStart = (i32)floorf(valueMin / valueStep);
		i32 valueEnd = (i32)ceilf(valueMax / valueStep);

		for (i32 i = valueStart; i <= valueEnd; i++)
		{
			f32 value = (f32)i * valueStep;
			f32 y = animationEditorValueToY(plotRect, value, valueMin, valueMax);

			if (y < plotRect.y - 1.0f || y > plotRect.bottom() + 1.0f)
				continue;

			Color gridColor = fabsf(value) < valueStep * 0.01f ? animationEditorSettings.gridStrongColor : animationEditorSettings.gridColor;

			ctx->renderer.cmdSetLineStyle(LineStyle(gridColor, scale));
			ctx->renderer.cmdDrawLine({ plotRect.x, y }, { plotRect.right(), y });

			char valueText[32];

			snprintf(valueText, sizeof(valueText), "%.*f", valueDecimals, value);
			animationEditorDrawText(font, valueText, { canvasRect.x + 2.0f * scale, y - 8.0f * scale, axisWidth - 8.0f * scale, 16.0f * scale }, dimTextColor, HAlignType::Right);
		}

		ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.gridStrongColor, scale));
		ctx->renderer.cmdDrawLine({ plotRect.x, canvasRect.y }, { plotRect.x, canvasRect.bottom() });

		ctx->renderer.popClipRect();
	}

	// ruler ticks and labels
	ctx->renderer.pushClipRect(rulerRect);

	for (i32 i = minorStart; i <= minorEnd; i++)
	{
		bool major = (i % 5) == 0;
		f32 time = (f32)i * minorStep;
		f32 x = animationEditorTimeToX(*state, plotRect, time);

		if (x < plotRect.x - 1.0f || x > plotRect.right() + 1.0f)
			continue;

		f32 tickLength = (major ? 6.0f : 3.0f) * scale;

		ctx->renderer.cmdSetLineStyle(LineStyle(major ? animationEditorSettings.gridStrongColor : animationEditorSettings.gridColor, scale));
		ctx->renderer.cmdDrawLine({ x, rulerRect.bottom() - tickLength }, { x, rulerRect.bottom() });

		if (major)
		{
			char timeText[32];

			snprintf(timeText, sizeof(timeText), "%.*f", decimals, time);
			animationEditorDrawText(font, timeText, { x + 3.0f * scale, rulerRect.y, 70.0f * scale, rulerRect.height - 4.0f * scale }, dimTextColor, HAlignType::Left);
		}
	}

	ctx->renderer.popClipRect();

	AnimationEditorTangentHit hoveredTangent;

	if (!disabled && state->viewMode == AnimationEditorViewMode::Curves && mouseInAnimationEditor)
		hoveredTangent = animationEditorFindTangentHandle(*state, animationEditorSettings, plotRect, valueMin, valueMax, mouse);

	// curves and keys
	if (state->viewMode == AnimationEditorViewMode::Curves)
	{
		ctx->renderer.pushClipRect(plotRect);

		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			AnimationEditorChannel& channel = state->channels[channelIndex];

			if (!channel.enabled || channel.keys.empty())
				continue;

			i32 components = animationEditorComponentCount(channel.type);

			for (i32 component = 0; component < components; component++)
			{
				if (!animationEditorComponentVisible(channel, component))
					continue;

				std::vector<Point> points;

				points.reserve(channel.keys.size() + 2);

				if (channel.type == AnimationEditorKeyType::Bool || channel.type == AnimationEditorKeyType::Int)
				{
					f32 previousValue = animationEditorQuantizeComponent(channel.type, animationEditorEvaluateComponent(channel, component, timeLeft), component);

					points.push_back({ plotRect.x, animationEditorValueToY(plotRect, previousValue, valueMin, valueMax) });

					for (auto& key : channel.keys)
					{
						if (key.time < timeLeft || key.time > timeRight)
							continue;

						if (!animationEditorKeyHasComponent(key, component))
							continue;

						f32 value = animationEditorQuantizeComponent(channel.type, key.value[component], component);

						// a key with the same value as the previous one adds no step
						if (value == previousValue)
							continue;

						f32 x = animationEditorTimeToX(*state, plotRect, key.time);

						points.push_back({ x, animationEditorValueToY(plotRect, previousValue, valueMin, valueMax) });
						previousValue = value;
						points.push_back({ x, animationEditorValueToY(plotRect, previousValue, valueMin, valueMax) });
					}

					points.push_back({ plotRect.right(), animationEditorValueToY(plotRect, previousValue, valueMin, valueMax) });
				}
				else
				{
					// sample the hermite curve so its tangents are visible
					i32 sampleCount = std::max(2, (i32)ceilf(plotRect.width / (3.0f * scale)));

					for (i32 sample = 0; sample <= sampleCount; sample++)
					{
						f32 ratio = (f32)sample / (f32)sampleCount;
						f32 sampleTime = timeLeft + (timeRight - timeLeft) * ratio;
						f32 value = animationEditorEvaluateComponent(channel, component, sampleTime);

						points.push_back({ plotRect.x + plotRect.width * ratio, animationEditorValueToY(plotRect, value, valueMin, valueMax) });
					}
				}

				if (points.size() < 2)
					continue;

				Color color = animationEditorChannelColor(channelIndex, component);

				if (channelIndex == state->selectedChannel)
					color.a = 1.0f;
				else
					color.a = 0.85f;

				f32 lineWidth = animationEditorSettings.curveLineWidth * scale;

				ctx->renderer.cmdSetLineStyle(LineStyle(color, lineWidth));
				ctx->renderer.cmdDrawPolyLine(points.data(), (u32)points.size(), false);
			}
		}

		// tangent handles of the selected keys
		f32 handlePixels = animationEditorSettings.tangentHandleLength * scale;

		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			AnimationEditorChannel& channel = state->channels[channelIndex];

			if (!channel.enabled || channel.keys.empty()
				|| channel.type == AnimationEditorKeyType::Bool || channel.type == AnimationEditorKeyType::Int)
				continue;

			i32 components = animationEditorComponentCount(channel.type);

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				AnimationEditorKey& key = channel.keys[keyIndex];

				if (!key.selected && key.selectedComponents == 0)
					continue;

				Point keyPoint = {
					animationEditorTimeToX(*state, plotRect, key.time),
					animationEditorValueToY(plotRect, key.value[0], valueMin, valueMax) };

				for (i32 component = 0; component < components; component++)
				{
					if (!animationEditorComponentVisible(channel, component)
						|| !animationEditorKeyHasComponent(key, component))
						continue;

					Color color = animationEditorChannelColor(channelIndex, component);
					Color lineColor = color;
					lineColor.a = 0.55f;
					Point componentPoint = {
						keyPoint.x,
						animationEditorValueToY(plotRect, key.value[component], valueMin, valueMax) };

					for (i32 side = 0; side < 2; side++)
					{
						bool outgoing = side == 0;
						Point handle = animationEditorTangentHandlePosition(*state, channel, keyIndex, component, outgoing, plotRect, valueMin, valueMax, handlePixels);
						bool hovered = hoveredTangent.channel == channelIndex
							&& hoveredTangent.key == keyIndex
							&& hoveredTangent.component == component
							&& hoveredTangent.outgoing == outgoing;

						ctx->renderer.cmdSetLineStyle(LineStyle(hovered ? color : lineColor, (hovered ? 1.5f : 1.0f) * scale));
						ctx->renderer.cmdDrawLine(componentPoint, handle);

						if (hovered)
							animationEditorDrawDiamond(handle, (animationEditorSettings.tangentHandleRadius + 3.0f) * scale, { 1, 1, 1, 0.45f });

						animationEditorDrawDiamond(handle, animationEditorSettings.tangentHandleRadius * scale * (hovered ? 1.25f : 1.0f), color);
					}
				}
			}
		}

		ctx->renderer.popClipRect();
	}

	// hovered key
	f32 rowsTopForHitTest = canvasRect.y - state->channelScroll * scale;
	AnimationEditorKeyHit hoveredKey = mouseInAnimationEditor && canvasRect.contains(mouse)
		? animationEditorFindKeyUnderMouse(*state, animationEditorSettings, plotRect, channelRows, rowsTopForHitTest, rowHeight, valueMin, valueMax)
		: AnimationEditorKeyHit();

	// keys
	ctx->renderer.pushClipRect(canvasRect);

	if (state->viewMode == AnimationEditorViewMode::Keyframes)
	{
		for (u32 rowIndex = 0; rowIndex < channelRows.size(); rowIndex++)
		{
			const AnimationEditorChannelRow& rowInfo = channelRows[rowIndex];
			AnimationEditorChannel& channel = state->channels[rowInfo.channel];

			if (!channel.enabled)
				continue;

			f32 centerY = rowsTop + (f32)rowIndex * rowHeight + rowHeight * 0.5f;
			Color channelColor = animationEditorChannelColor(rowInfo.channel, rowInfo.component);

			// a parent row shows all the channel keys, a component row only its own
			i32 firstKeyIndex = -1;
			i32 lastKeyIndex = -1;

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				if (rowInfo.component >= 0 && !animationEditorKeyHasComponent(channel.keys[keyIndex], rowInfo.component))
					continue;

				if (firstKeyIndex < 0)
					firstKeyIndex = keyIndex;

				lastKeyIndex = keyIndex;
			}

			if (firstKeyIndex >= 0 && lastKeyIndex > firstKeyIndex)
			{
				Color lineColor = channelColor;
				lineColor.a = 0.45f;

				ctx->renderer.cmdSetLineStyle(LineStyle(lineColor, animationEditorSettings.keyframeLineWidth * scale));
				ctx->renderer.cmdDrawLine(
					{ animationEditorTimeToX(*state, plotRect, channel.keys[firstKeyIndex].time), centerY },
					{ animationEditorTimeToX(*state, plotRect, channel.keys[lastKeyIndex].time), centerY });
			}

			for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
			{
				AnimationEditorKey& key = channel.keys[keyIndex];

				if (rowInfo.component >= 0 && !animationEditorKeyHasComponent(key, rowInfo.component))
					continue;

				f32 x = animationEditorTimeToX(*state, plotRect, key.time);

				if (x < plotRect.x - animationEditorSettings.keySize * scale || x > plotRect.right() + animationEditorSettings.keySize * scale)
					continue;

				bool hovered = hoveredKey.channel == rowInfo.channel
					&& hoveredKey.key == keyIndex
					&& hoveredKey.component == rowInfo.component;
				bool selected = key.selected
					|| (rowInfo.component >= 0 && (key.selectedComponents & (1u << rowInfo.component)));
				f32 radius = animationEditorSettings.keySize * scale;

				if (hovered)
					animationEditorDrawDiamond({ x, centerY }, radius * 1.9f, { 1, 1, 1, 0.35f });

				if (selected)
				{
					animationEditorDrawDiamond({ x, centerY }, radius * 1.55f, animationEditorSettings.keySelectedColor);
					animationEditorDrawDiamond({ x, centerY }, radius, channelColor);
				}
				else
				{
					animationEditorDrawDiamond({ x, centerY }, radius, channelColor);
				}

				// draw the tag only on the parent row so it isn't duplicated
				if (key.tag[0] && rowInfo.component < 0)
					animationEditorDrawKeyTag(font, key.tag, x + radius + 4.0f * scale, centerY, animationEditorSettings, scale);
			}
		}
	}
	else
	{
		for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
		{
			AnimationEditorChannel& channel = state->channels[channelIndex];

			if (!channel.enabled)
				continue;

			i32 components = animationEditorComponentCount(channel.type);

			for (i32 component = 0; component < components; component++)
			{
				if (!animationEditorComponentVisible(channel, component))
					continue;

				Color componentColor = animationEditorChannelColor(channelIndex, component);

				for (i32 keyIndex = 0; keyIndex < (i32)channel.keys.size(); keyIndex++)
				{
					AnimationEditorKey& key = channel.keys[keyIndex];

					if (!animationEditorKeyHasComponent(key, component))
						continue;

					f32 x = animationEditorTimeToX(*state, plotRect, key.time);

					if (x < plotRect.x - animationEditorSettings.keySize * scale || x > plotRect.right() + animationEditorSettings.keySize * scale)
						continue;

					f32 y = animationEditorValueToY(plotRect, key.value[component], valueMin, valueMax);

					if (y < plotRect.y - animationEditorSettings.keySize * scale || y > plotRect.bottom() + animationEditorSettings.keySize * scale)
						continue;

					bool hovered = hoveredKey.channel == channelIndex && hoveredKey.key == keyIndex && hoveredKey.component == component;
					f32 radius = animationEditorSettings.keySize * scale * 0.8f;

					if (hovered)
						animationEditorDrawDiamond({ x, y }, radius * 2.2f, { 1, 1, 1, 0.35f });

					if (key.selected || key.selectedComponents != 0)
					{
						animationEditorDrawDiamond({ x, y }, radius * 1.7f, animationEditorSettings.keySelectedColor);
						animationEditorDrawDiamond({ x, y }, radius, componentColor);
					}
					else
					{
						animationEditorDrawDiamond({ x, y }, radius, componentColor);
					}

					if (key.tag[0])
						animationEditorDrawKeyTag(font, key.tag, x + radius + 4.0f * scale, y, animationEditorSettings, scale);
				}
			}
		}
	}

	ctx->renderer.popClipRect();

	// nothing is drawn past the end of the time range, except the repeated curves
	{
		f32 endTimeX = animationEditorTimeToX(*state, plotRect, state->endTime);

		if (endTimeX < canvasRect.right())
		{
			f32 maskX = std::max(endTimeX, canvasRect.x);

			ctx->renderer.cmdSetColor(backgroundColor);
			ctx->renderer.cmdDrawFilledRectangle({ maskX, canvasRect.y, canvasRect.right() - maskX, canvasRect.height });

			if (state->viewMode == AnimationEditorViewMode::Curves
				&& state->repeat
				&& state->endTime > state->startTime)
			{
				f32 cycle = state->endTime - state->startTime;
				f32 repeatStartX = std::max(endTimeX, plotRect.x);
				f32 repeatEndX = std::min(plotRect.right(), canvasRect.right());

				if (repeatEndX > repeatStartX)
				{
					ctx->renderer.pushClipRect({ repeatStartX, plotRect.y, repeatEndX - repeatStartX, plotRect.height });

					Color ghostColor = animationEditorSettings.dimTextColor;

					ghostColor.a = 0.25f;
					ctx->renderer.cmdSetLineStyle(LineStyle(ghostColor, animationEditorSettings.curveLineWidth * scale));

					i32 sampleCount = std::max(2, (i32)ceilf((repeatEndX - repeatStartX) / (2.0f * scale)));

					for (i32 channelIndex = 0; channelIndex < (i32)state->channels.size(); channelIndex++)
					{
						AnimationEditorChannel& channel = state->channels[channelIndex];

						if (!channel.enabled || channel.keys.empty())
							continue;

						i32 components = animationEditorComponentCount(channel.type);

						for (i32 component = 0; component < components; component++)
						{
							if (!animationEditorComponentVisible(channel, component))
								continue;

							std::vector<Point> points;

							points.reserve(sampleCount + 1);

							for (i32 sample = 0; sample <= sampleCount; sample++)
							{
								f32 x = repeatStartX + (repeatEndX - repeatStartX) * (f32)sample / (f32)sampleCount;
								f32 sampleTime = animationEditorXToTime(*state, plotRect, x);
								f32 localTime = state->startTime + fmodf(sampleTime - state->startTime, cycle);
								f32 value = animationEditorEvaluateComponent(channel, component, localTime);

								points.push_back({ x, animationEditorValueToY(plotRect, value, valueMin, valueMax) });
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
		&& runtime.dragType != AnimationEditorWidgetState::DragType::Keys)
	{
		AnimationEditorChannel& channel = state->channels[hoveredKey.channel];
		AnimationEditorKey& key = channel.keys[hoveredKey.key];
		char keyInfo[128];

		if (hoveredKey.component >= 0)
			snprintf(keyInfo, sizeof(keyInfo), "t %.3fs   v %.4f", key.time, key.value[hoveredKey.component]);
		else
			snprintf(keyInfo, sizeof(keyInfo), "t %.3fs", key.time);

		Point infoSize = animationEditorTextSize(font, keyInfo);
		Rect infoRect = { mouse.x + 14.0f * scale, mouse.y - 24.0f * scale, infoSize.x + 10.0f * scale, 18.0f * scale };

		if (infoRect.right() > outerRect.right())
			infoRect.x = mouse.x - infoRect.width - 8.0f * scale;

		if (infoRect.y < outerRect.y)
			infoRect.y = mouse.y + 18.0f * scale;

		ctx->renderer.cmdSetColor(headerColor);
		ctx->renderer.cmdDrawFilledRectangle(infoRect);
		ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.gridStrongColor, scale));
		ctx->renderer.cmdDrawRectangle(infoRect);
		animationEditorDrawText(font, keyInfo, infoRect, textColor, HAlignType::Center, false);
	}

	// playhead
	{
		f32 playheadX = animationEditorTimeToX(*state, plotRect, state->cursorTime);

		if (playheadX >= plotRect.x - 1.0f && playheadX <= plotRect.right() + 1.0f)
			drawTimeCursor(playheadX, rulerRect, canvasRect.bottom(), animationEditorSettings.playheadColor, animationEditorSettings.playheadWidth);
	}

	// end of the time range
	{
		f32 endTimeX = animationEditorTimeToX(*state, plotRect, state->endTime);

		if (endTimeX >= plotRect.x - 1.0f && endTimeX <= plotRect.right() + 1.0f)
		{
			ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.endTimeLineColor, animationEditorSettings.endTimeLineWidth * scale));
			ctx->renderer.cmdDrawLine({ endTimeX, rulerRect.y }, { endTimeX, canvasRect.bottom() });
		}
	}

	// box selection
	if (runtime.dragType == AnimationEditorWidgetState::DragType::BoxSelect)
	{
		Rect selectionRect = {
			std::min(runtime.boxStart.x, runtime.boxEnd.x),
			std::min(runtime.boxStart.y, runtime.boxEnd.y),
			fabsf(runtime.boxEnd.x - runtime.boxStart.x),
			fabsf(runtime.boxEnd.y - runtime.boxStart.y)
		};

		selectionRect = selectionRect.clipInside(canvasRect);

		ctx->renderer.cmdSetColor(animationEditorSettings.selectionFillColor);
		ctx->renderer.cmdDrawFilledRectangle(selectionRect);
		ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.selectionBorderColor, scale));
		ctx->renderer.cmdDrawRectangle(selectionRect);
	}

	// separators
	ctx->renderer.cmdSetLineStyle(LineStyle(animationEditorSettings.gridStrongColor, scale));
	ctx->renderer.cmdDrawLine({ outerRect.x, toolbarRect.bottom() }, { outerRect.right(), toolbarRect.bottom() });
	ctx->renderer.cmdDrawLine({ outerRect.x, listHeaderRect.bottom() }, { outerRect.right(), listHeaderRect.bottom() });
	ctx->renderer.cmdDrawLine({ listHeaderRect.right(), toolbarRect.bottom() }, { listHeaderRect.right(), outerRect.bottom() });
	ctx->renderer.cmdDrawLine({ rulerRect.x, rulerRect.bottom() }, { rulerRect.right(), rulerRect.bottom() });

	if (disabled)
	{
		ctx->renderer.cmdSetColor(animationEditorSettings.disabledOverlayColor);
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
			const AnimationEditorToolbarField& field = toolbarFields[i];

			if (field.channel < 0 || field.channel >= (i32)state->channels.size())
				continue;

			AnimationEditorChannel& channel = state->channels[field.channel];

			if (field.key < 0 || field.key >= (i32)channel.keys.size())
				continue;

			AnimationEditorKey& key = channel.keys[field.key];
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
				// the full timeline range is used so the slider fill reflects the
				// key position in the timeline, the key is clamped between its
				// neighbours afterwards to keep the keys ordered
				fieldChanged = comboSliderFloatRanged(&key.time, state->startTime, state->endTime, timeStepsPerPixel, timeArrowStep, "t %.3f");

				if (fieldChanged)
				{
					if (field.key > 0)
						key.time = std::max(key.time, channel.keys[field.key - 1].time + 0.001f);

					if (field.key + 1 < (i32)channel.keys.size())
						key.time = std::min(key.time, channel.keys[field.key + 1].time - 0.001f);
				}
			}
			else if (field.component == -2)
			{
				idPush((void*)&runtime);
				fieldChanged = textInput("animationEditorKeyTag", key.tag, AnimationEditorKey::maxTagSize, TextInputFlags::None, "Tag");
				idPop();
			}
			else if (field.component >= 0 && field.component < animationEditorComponentCount(channel.type))
			{
				const char* label = animationEditorComponentLabel(channel.type, field.component);
				char format[32];
				f32 stepsPerPixel = valueStepsPerPixel;
				f32 arrowStep = valueArrowStep;

				if (channel.type == AnimationEditorKeyType::Int)
				{
					stepsPerPixel = 1.0f;
					arrowStep = 1.0f;
				}

				snprintf(format, sizeof(format), "%s %s", label, channel.type == AnimationEditorKeyType::Int ? "%.0f" : "%.3f");
				fieldChanged = comboSliderFloat(&key.value[field.component], stepsPerPixel, arrowStep, format);

				if (fieldChanged)
					key.value[field.component] = animationEditorQuantizeComponent(channel.type, key.value[field.component], field.component);
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
			const AnimationEditorChannelRow& rowInfo = channelRows[rowIndex];

			if (rowInfo.component >= 0)
				continue;

			i32 channelIndex = rowInfo.channel;
			Rect rowRect = { listRect.x, listRect.y - scrollOffset + (f32)rowIndex * rowHeight, listRect.width, rowHeight };
			Rect checkboxRect = animationEditorChannelCheckboxRect(rowRect, scale);

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
		&& (mouseInAnimationEditor
			|| runtime.dragType == AnimationEditorWidgetState::DragType::TangentIn
			|| runtime.dragType == AnimationEditorWidgetState::DragType::TangentOut))
	{
		if (runtime.dragType == AnimationEditorWidgetState::DragType::Pan)
			mouseCursorSetType(MouseCursorType::SizeAll);
		else if (runtime.dragType == AnimationEditorWidgetState::DragType::Playhead)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == AnimationEditorWidgetState::DragType::TangentIn
			|| runtime.dragType == AnimationEditorWidgetState::DragType::TangentOut)
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (runtime.dragType == AnimationEditorWidgetState::DragType::Keys)
			mouseCursorSetType(state->viewMode == AnimationEditorViewMode::Curves ? MouseCursorType::SizeAll : MouseCursorType::SizeWE);
		else if (runtime.dragType == AnimationEditorWidgetState::DragType::ChannelResize || channelSplitterRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (runtime.dragType == AnimationEditorWidgetState::DragType::ChannelScroll || channelScrollbar.track.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (hoveredToolbarButton >= 0 || listRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (rulerRect.contains(mouse) || fabsf(mouse.x - animationEditorTimeToX(*state, plotRect, state->cursorTime)) <= 5.0f * scale)
			mouseCursorSetType(MouseCursorType::SizeWE);
		else if (hoveredTangent.channel >= 0 || hoveredKey.channel >= 0)
			mouseCursorSetType(MouseCursorType::HandPointing);
		else if (canvasRect.contains(mouse))
			mouseCursorSetType(MouseCursorType::CrossHair);
	}

	return changed;
}

bool animationEditorAction(AnimationEditorState* state, AnimationEditorAction action, f32 viewWidth)
{
	if (!state)
		return false;

	AnimationEditorSettings settings = ctx->settings.animationEditor;

	if (settings.useThemeColors)
		animationEditorApplyThemeColors(settings);

	bool changed = false;
	Rect viewRect = { 0.0f, 0.0f, std::max(viewWidth, 1.0f), 1.0f };

	switch (action)
	{
	case AnimationEditorAction::Play:
		state->playing = !state->playing;

		if (state->playing && state->cursorTime >= state->endTime)
			state->cursorTime = state->startTime;

		changed = true;
		break;

	case AnimationEditorAction::KeyframesMode:
		if (state->viewMode != AnimationEditorViewMode::Keyframes)
		{
			state->viewMode = AnimationEditorViewMode::Keyframes;
			changed = true;
		}
		break;

	case AnimationEditorAction::CurvesMode:
		if (state->viewMode != AnimationEditorViewMode::Curves)
		{
			state->viewMode = AnimationEditorViewMode::Curves;
			changed = true;
		}
		break;

	case AnimationEditorAction::AddKey:
	{
		i32 channelIndex = animationEditorTargetChannel(*state);

		if (channelIndex >= 0)
		{
			f32 values[4];
			f32 addTime = animationEditorSnapTime(*state, state->cursorTime);
			const AnimationEditorChannel& channel = state->channels[channelIndex];

			// when specific sub-channels are selected, the new key belongs only
			// to them, otherwise it covers all the channel components
			u32 componentMask = channel.selected ? 0 : channel.selectedComponents;

			animationEditorEvaluateChannel(channel, addTime, values);
			animationEditorAddKey(*state, channelIndex, addTime, values, componentMask, true);
			changed = true;
		}
		break;
	}

	case AnimationEditorAction::DeleteSelected:
		changed = animationEditorDeleteSelectedKeys(*state) > 0;
		break;

	case AnimationEditorAction::ToggleSnap:
		state->snapToFrames = !state->snapToFrames;
		changed = true;
		break;

	case AnimationEditorAction::ZoomOut:
	case AnimationEditorAction::ZoomIn:
	case AnimationEditorAction::FitView:
		if (viewWidth > 0.0f)
		{
			if (action == AnimationEditorAction::FitView)
			{
				animationEditorFitView(*state, settings, viewRect);
			}
			else
			{
				f32 factor = action == AnimationEditorAction::ZoomOut ? 1.0f / settings.zoomStep : settings.zoomStep;

				animationEditorZoom(*state, settings, viewRect, factor, viewRect.x + viewRect.width * 0.5f);
			}

			animationEditorClampView(*state, viewRect, settings);
			changed = true;
		}
		break;
	}

	return changed;
}

}
