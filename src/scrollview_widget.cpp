#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "context.h"
#include "theme.h"
#include "util.h"
#include "font.h" // for metric fallback when computing default

namespace hui
{
static constexpr f32 SCROLL_SNAP_EPS = 0.5f; // pixels

static void updateScrollMax(
    ScrollbarState& state,
    f32 contentSize,
    f32 viewSize)
{
    state.scrollMax = (contentSize > viewSize)
        ? (contentSize - viewSize)
        : 0.0f;

    state.scrollOffset = std::clamp(
        state.scrollOffset, 0.0f, state.scrollMax);
}

static f32 computeHandleSize(
    f32 scrollBarSize,
    f32 contentSize,
    f32 viewSize,
    f32 minHandleSize)
{
    if (contentSize <= viewSize)
        return scrollBarSize;

    f32 size = scrollBarSize * (viewSize / contentSize);
    if (size < minHandleSize)
        size = minHandleSize;

    return size;
}

static f32 computeHandleOffset(
    const ScrollbarState& state,
    f32 scrollBarSize,
    f32 handleSize)
{
	if (state.scrollMax <= 0.0f)
		return 0.0f;

	if (state.scrollOffset <= 0.0f)
		return 0.0f;

	if (state.scrollOffset >= state.scrollMax)
		return scrollBarSize - handleSize;

	f32 slack = scrollBarSize - handleSize;
	if (slack <= 0.0f)
		return 0.0f;

	return (state.scrollOffset / state.scrollMax) * slack;
}

static void applyPageScroll(
    ScrollbarState& state,
    f32 viewSize,
    f32 pageSizeNormalized, // 0..1
    f32 direction) // -1 or +1
{
    if (state.scrollMax <= 0.0f)
        return;

    pageSizeNormalized = std::clamp(pageSizeNormalized, 0.0f, 1.0f);
    direction = (direction < 0.0f) ? -1.0f : 1.0f;

    f32 pageStep = viewSize * pageSizeNormalized;
    state.scrollOffset += pageStep * direction;
    state.scrollOffset = std::clamp(
        state.scrollOffset, 0.0f, state.scrollMax);
}

static void snapToItem(
    ScrollbarState& state,
    const ScrollToItemBounds& item,
    f32 viewSize,
	ScrollToItemSnapMode mode)
{
    if (state.scrollMax <= 0.0f)
        return;

    f32 targetScroll = state.scrollOffset;

    switch (mode)
    {
        case ScrollToItemSnapMode::Minimal:
        {
            if (item.min < state.scrollOffset)
                targetScroll = item.min;
            else if (item.max > state.scrollOffset + viewSize)
                targetScroll = item.max - viewSize;
            break;
        }

        case ScrollToItemSnapMode::AlignStart:
            targetScroll = item.min;
            break;

        case ScrollToItemSnapMode::AlignCenter:
        {
            f32 itemCenter = (item.min + item.max) * 0.5f;
            targetScroll = itemCenter - viewSize * 0.5f;
            break;
        }

        case ScrollToItemSnapMode::AlignEnd:
            targetScroll = item.max - viewSize;
            break;
    }

    state.scrollOffset = std::clamp(
        targetScroll, 0.0f, state.scrollMax);
}

static void applyHandleDrag(
	ScrollbarState& state,
	f32 scrollBarStart,   // screen space start of scrollbar
	f32 scrollBarSize,
	f32 handleSize,
	f32 mousePos,         // current mouse position (same axis)
	f32 grabOffset)       // mousePos handleStart at mouse-down
{
	if (state.scrollMax <= 0.0f)
		return;

	f32 slack = scrollBarSize - handleSize;
	if (slack <= 0.0f)
		return;

	f32 handleOffset =
		(mousePos - scrollBarStart) - grabOffset;

	handleOffset = std::clamp(handleOffset, 0.0f, slack);

	f32 handleRatio = handleOffset / slack;
	state.scrollOffset = handleRatio * state.scrollMax;

	if (handleOffset <= 0.0f)
		state.scrollOffset = 0.0f;
	else if (handleOffset >= slack)
		state.scrollOffset = state.scrollMax;
}

void scrollViewBegin(const char* id, f32 size, f32 scrollPos)
{
	scrollViewBegin(id, size, { 0, scrollPos }, 0, ScrollViewFlags::None);
}

void scrollViewBegin(const char* id, f32 size, f32 scrollPos, f32 virtualHeight)
{
	scrollViewBegin(id, size, { 0, scrollPos }, { 0, virtualHeight }, ScrollViewFlags::None);
}

void scrollViewBegin(const char* id, f32 size, f32 scrollPos, f32 virtualHeight, ScrollViewFlags flags)
{
	scrollViewBegin(id, size, { 0, scrollPos }, { 0, virtualHeight }, flags);
}

// Main implementation with all parameters
void scrollViewBegin(const char* id, f32 height, Point scrollOffset, Point virtualSize, ScrollViewFlags flags)
{
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
	auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
	
	widgetPushDisabled(widgetGetDisabled());

	ctx->id = genId(id);

	if (height <= 0.0f)
	{
		// Use remaining height in layout
		height = layoutGetRemainingHeight();

		if (height <= 0)
		{
			// Fallback to a reasonable default if no space available
			height = 10.0f;
		}
	}

	if (ctx->settings.scaleScrollViewHeight)
		height *= ctx->scale;

	auto& scrollViewState = ctx->scrollViewState[ctx->id];
	scrollViewState.height = height;

	// Do NOT override the vertical virtual size here.
	// Let beginVirtualListContent(...) set the total virtual height when used.
	// Preserve any provided horizontal virtual size, but leave virtualSize.y as-is (0 = auto).
	scrollViewState.virtualSize.x = virtualSize.x;

	scrollViewState.id = ctx->id;
	scrollViewState.flags = flags;

	const auto& padding = paddingGet(PaddingType::ScrollView);
	const auto border = (has(flags, ScrollViewFlags::NoBorder) ? 0 : (f32)scrollViewElemState.border * ctx->scale);
	auto internalPadding = border + (has(flags, ScrollViewFlags::NoPadding) ? 0 : padding.x);

	Rect rect =
	{
		ctx->position.x,
		ctx->position.y,
		ctx->layout.width,
		height
	};

	scrollViewState.rect = rect;

	Rect clipRect = rect;

	clipRect.x += internalPadding;
	clipRect.y += border * ctx->scale;
	clipRect.height -= border * ctx->scale * 2.0f;

	auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
	f32 hBarHeight = scrollViewScrollBarElemStateH.height * ctx->scale;

	bool willHaveHBar = !has(flags, ScrollViewFlags::NoHorizontalScroll)
		&& (virtualSize.x > rect.width || (virtualSize.x == 0 && scrollViewState.horizontal.wasVisible));

	f32 approximateScrollAreaV = clipRect.height;
	if (willHaveHBar)
	{
		approximateScrollAreaV -= hBarHeight + padding.y;
	}

	bool reserveVBar = true;
	// if we passed a known virtual height and it doesn't exceed our area, we don't need a VBar!
	if (virtualSize.y > 0 && virtualSize.y <= approximateScrollAreaV)
	{
		reserveVBar = false;
	}

	if (reserveVBar)
	{
		clipRect.width -= scrollViewScrollBarElemStateV.width * ctx->scale + internalPadding * 2.0f;
	}

	if (willHaveHBar)
	{
		clipRect.height -= scrollViewScrollBarElemStateH.height * ctx->scale + padding.y;
	}

	if (!has(flags, ScrollViewFlags::NoBorder))
	{
		ctx->renderer.cmdSetColor(scrollViewElemState.color);
		ctx->renderer.cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->scale);
	}

	// Do not blindly overwrite the authoritative scrollbar state with the caller-provided
	// scrollOffset while this scrollview may be actively dragged. Use the authoritative
	// per-axis state when dragging; otherwise accept the incoming offset.
	bool draggingThisBegin = (ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
		|| scrollViewState.vertical.draggingThumb
		|| scrollViewState.horizontal.draggingThumb;

	if (!draggingThisBegin)
	{
		// Accept caller offset when not dragging (will be clamped/finalized in scrollViewEnd).
		scrollViewState.scrollOffset = scrollOffset;
	}
	// Use authoritative per-axis offset for layout positioning in this frame.
	Point useOffset = scrollViewState.scrollOffset;

	ctx->maxContentWidthStack.push_back(ctx->maxContentWidth);
	ctx->maxContentWidth = 0.0f;

	ctx->renderer.pushClipRect(clipRect);
	widgetPushPosition();
	ctx->position = { clipRect.x, clipRect.y };
	ctx->position -= useOffset;

	layoutPush();
	ctx->layout = LayoutState(LayoutType::ScrollView);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.id = ctx->id;
	ctx->layout.width = clipRect.width;
	ctx->layout.height = clipRect.height;
}

Point scrollViewEnd()
{
	ctx->id = ctx->layout.id;

	auto prevPenPos = ctx->layout.savedPosition;
	auto clipRect = ctx->renderer.getClipRect();

	ctx->renderer.popClipRect();

	auto& scrollViewState = ctx->scrollViewState[ctx->id];
	const auto& fullRect = scrollViewState.rect;
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto scrollOffset = scrollViewState.scrollOffset;
	f32 height = scrollViewState.height;
	const auto& padding = paddingGet(PaddingType::ScrollView);
	const auto border = (has(scrollViewState.flags, ScrollViewFlags::NoBorder) ? 0 : (f32)scrollViewElemState.border * ctx->scale);
	auto internalPadding = padding + border;

	// make the rect for the scrollbars, without the UI element border
	auto rectNoBorders = fullRect.contract(scrollViewElemState.border);
	auto scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
	auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();

	f32 scrollContentH = ctx->maxContentWidth - rectNoBorders.x - internalPadding.x + scrollViewState.horizontal.scrollOffset;

	if (scrollViewState.virtualSize.x > scrollContentH)
		scrollContentH = scrollViewState.virtualSize.x;

	// Restore previous max content X
	ctx->maxContentWidth = ctx->maxContentWidthStack.back();
	ctx->maxContentWidthStack.pop_back();

	// compute measured content height early so it can be used for layout/availability checks
	f32 scrollContentSizeV = ctx->position.y - prevPenPos.y + internalPadding.y;
	// authoritative clamp size: use the larger of measured and virtualSize (virtual list may set this)
	f32 contentSizeForClamp = std::max(scrollContentSizeV, scrollViewState.virtualSize.y);

	// DEBUG: log measured vs authoritative content size
	/*std::printf("[DBG] scrollContentSizeV=%.3f virtualSize.y=%.3f contentSizeForClamp=%.3f\n",
		scrollContentSizeV, scrollViewState.virtualSize.y, contentSizeForClamp);*/

	f32 availableWidth = rectNoBorders.width - padding.x * 2.0f;

	if (scrollContentSizeV > rectNoBorders.height)
	{
		availableWidth -= scrollViewScrollBarElemStateV.width * ctx->scale;
	}

	bool hasHorizontalScrollbar =
		!has(scrollViewState.flags, ScrollViewFlags::NoHorizontalScroll)
		&& ((scrollViewState.virtualSize.x > 0 && scrollViewState.virtualSize.x > availableWidth) || scrollContentH > availableWidth);

	f32 scrollAreaV = rectNoBorders.height;

	if (hasHorizontalScrollbar)
	{
		scrollAreaV -= scrollViewScrollBarElemStateH.height * ctx->scale;
	}

	bool hasVerticalScrollbar = scrollContentSizeV > scrollAreaV;

	if (hasVerticalScrollbar)
	{
		// Make sure vertical scrollMax is up-to-date before applying any wheel deltas.
        // Use authoritative content size (measured OR virtualSize set by virtual list).
        updateScrollMax(scrollViewState.vertical, contentSizeForClamp, scrollAreaV);

        // scroll view with mouse wheel
        if (ctx->isActiveLayer() && ctx->event.type == InputEvent::Type::MouseWheel && !ctx->widget.disabled)
        {
            // Only scroll if mouse is over this scroll view AND this window is the hovered window
            if (rectNoBorders.contains(ctx->mousePosition))
            {
                bool isWindowHovered = true;
                if (ctx->settings.services.getCurrentWindow && ctx->settings.services.getHoveredWindow)
                    isWindowHovered = (ctx->settings.services.getCurrentWindow() == ctx->settings.services.getHoveredWindow());

                if (isWindowHovered)
                {
                    // Compute wheel delta (same convention as before)
                    f32 wheelFactor = (scrollAreaV * ctx->scrollViewSpeed) * ctx->scale;
                    f32 delta = ctx->event.mouse.wheel.y * wheelFactor; // positive/negative per input source

                    // Check if we're at a limit before applying scroll
                    auto& v = scrollViewState.vertical;
                    bool atTop = (v.scrollOffset <= 0.0f) && (delta > 0.0f);
                    bool atBottom = (v.scrollOffset >= v.scrollMax) && (v.scrollMax > 0.0f) && (delta < 0.0f);
                    
                    // Only scroll if not at a limit - this allows wheel to bubble to parent scroll view
                    if (!atTop && !atBottom)
                    {
                        // Apply to authoritative scrollbar state and clamp immediately using scrollMax
                        v.scrollOffset = std::clamp(v.scrollOffset - delta, 0.0f, v.scrollMax);

                        // Snap small rounding differences to the exact max
                        if (v.scrollMax > 0.0f && (v.scrollMax - v.scrollOffset) <= SCROLL_SNAP_EPS)
                            v.scrollOffset = v.scrollMax;

                        // Mirror authoritative value back into the Point and the overall scrollViewState
                        scrollOffset.y = v.scrollOffset;
                        scrollViewState.scrollOffset = scrollOffset;

                        inputEventCancel();
                        forceRepaint();
                    }
                }
            }
        }

		scrollViewState.horizontal.wasVisible = hasHorizontalScrollbar; // Persist visibility for next frame reservation

		// auto scroll to the focused widget if curent widget changed
		if (ctx->focusChanged && ctx->widget.focusedId == ctx->id)
		{
			if (ctx->widget.focusedWidgetRect.y > rectNoBorders.bottom())
			{
				scrollOffset.y = (ctx->widget.focusedWidgetRect.y + scrollOffset.y) - rectNoBorders.y;
			}
		}

		// draw scrollbar if content is bigger than scroll view
		if (scrollContentSizeV > scrollAreaV)
		{
			auto scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
			auto scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
			const auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

			// the actual scroll bar height, without the borders, for handle to be drawn correctly
			f32 scrollBarHeightFull = rectNoBorders.height;

			if (hasHorizontalScrollbar)
			{
				scrollBarHeightFull -= scrollViewScrollBarElemStateH.height * ctx->scale;
			}
			f32 scrollBarHeight = scrollBarHeightFull - scrollViewScrollBarElemStateV.border* ctx->scale * 2.0f;

			Rect rectScrollBarV =
			{
				rectNoBorders.right() - scrollViewScrollBarElemStateV.width * ctx->scale,
				rectNoBorders.y,
				scrollViewScrollBarElemStateV.width * ctx->scale,
				scrollBarHeightFull
			};

			 // Use authoritative clamp (contentSizeForClamp) for handle math
            updateScrollMax(scrollViewState.vertical, contentSizeForClamp, scrollAreaV);
            f32 handleSize = computeHandleSize(scrollBarHeight, contentSizeForClamp, scrollAreaV, ctx->settings.minScrollViewHandleSize);
            f32 handleOffset = computeHandleOffset(scrollViewState.vertical, scrollBarHeight, handleSize);

			Rect rectScrollBarHandleV =
			{
				rectScrollBarV.x + (scrollViewScrollBarElemStateV.width - scrollViewScrollThumbElemStateV.width) * ctx->scale * 0.5f,
				rectScrollBarV.y + handleOffset + scrollViewScrollBarElemStateV.border * ctx->scale,
				scrollViewScrollThumbElemStateV.width * ctx->scale,
				handleSize
			};

			if (ctx->isActiveLayer())
			{
				if (rectScrollBarHandleV.contains(ctx->mousePosition)
					|| (scrollViewState.vertical.draggingThumb && ctx->dragScrollViewHandleWidgetId == scrollViewState.id))
				{
					scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).getState(WidgetStateType::Hovered);
				}
			}

			if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer() && !ctx->widget.disabled)
			{
				if (rectScrollBarHandleV.contains(ctx->mousePosition))
				{
					windowSetCapture();  // Capture mouse to get events outside window
					scrollViewState.vertical.draggingThumb = true;
					scrollViewState.vertical.dragDelta = ctx->mousePosition - rectScrollBarHandleV.topLeft();
					ctx->dragScrollViewHandleWidgetId = scrollViewState.id;
					ctx->widget.focusedId = ctx->id;
					scrollViewState.lastMousePos = ctx->mousePosition;
				}
				else if (rectScrollBarV.contains(ctx->mousePosition))
				{
					applyPageScroll(scrollViewState.vertical, scrollAreaV, ctx->scrollViewScrollPageSize, (ctx->mousePosition.y < rectScrollBarHandleV.y) ? -1.0f : 1.0f);
					scrollOffset.y = scrollViewState.vertical.scrollOffset;
					scrollViewState.scrollOffset = scrollOffset;
				}
			}
			else if (ctx->mouseMoved
				&& ctx->event.type != InputEvent::Type::MouseUp
				&& scrollViewState.vertical.draggingThumb
				&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
			{
				// kill event, only we're dragging now
				hui::inputEventCancel();
				applyHandleDrag(
					scrollViewState.vertical,
					rectScrollBarV.y,
					scrollBarHeight,
					handleSize,
					ctx->mousePosition.y,
					scrollViewState.vertical.dragDelta.y);
			}

			if (ctx->event.type == InputEvent::Type::MouseUp
				&& scrollViewState.vertical.draggingThumb
				&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
			{
				scrollViewState.vertical.draggingThumb = false;
				ctx->dragScrollViewHandleWidgetId = 0;
				ctx->widget.captureId = 0;
				windowReleaseCapture();

				// snap any tiny rounding residual to the exact max so last item becomes reachable
				auto& v = scrollViewState.vertical;
				if (v.scrollMax > 0.0f && (v.scrollMax - v.scrollOffset) <= SCROLL_SNAP_EPS)
					v.scrollOffset = v.scrollMax;

				// keep Point in sync
				scrollOffset.y = v.scrollOffset;
				scrollViewState.scrollOffset = scrollOffset;
			}

			// ensure we clamp using the authoritative size after dragging/updates
            updateScrollMax(scrollViewState.vertical, contentSizeForClamp, scrollAreaV);
            handleSize = computeHandleSize(scrollBarHeight, contentSizeForClamp, scrollAreaV, ctx->settings.minScrollViewHandleSize);
            handleOffset = computeHandleOffset(scrollViewState.vertical, scrollBarHeight, handleSize);

			rectScrollBarHandleV =
			{
				rectScrollBarV.x + (scrollViewScrollBarElemStateV.width - scrollViewScrollThumbElemStateV.width) * ctx->scale * 0.5f,
				rectScrollBarV.y + handleOffset + scrollViewScrollBarElemStateV.border * ctx->scale,
				scrollViewScrollThumbElemStateV.width * ctx->scale,
				handleSize
			};

			// draw scroll bar line
			ctx->renderer.cmdSetColor(scrollViewElemState.color);
			ctx->renderer.cmdDrawImageBordered(scrollViewScrollBarElemStateV.image, scrollViewScrollBarElemStateV.border, rectScrollBarV, ctx->scale);

			// draw scroll bar thumb
			ctx->renderer.cmdSetColor(scrollViewScrollThumbElemStateV.color);
			ctx->renderer.cmdDrawImageBordered(scrollViewScrollThumbElemStateV.image, scrollViewScrollThumbElemStateV.border, rectScrollBarHandleV, ctx->scale);
		}
	}

	f32 scrollAreaWidth = rectNoBorders.width - padding.x * 2.0f;

	if (hasHorizontalScrollbar)
	{
		auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
		auto scrollViewScrollThumbElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).normalState();

		// the actual scroll bar width, without the borders, for handle to be drawn correctly
		f32 scrollBarWidth = rectNoBorders.width - scrollViewScrollBarElemStateH.border * ctx->scale * 2.0f;
		f32 scrollBarWidthFull = rectNoBorders.width;

		if (scrollContentSizeV > rectNoBorders.height)
		{
			auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
			scrollBarWidth -= scrollViewScrollBarElemStateV.width * ctx->scale;
			scrollAreaWidth -= scrollViewScrollBarElemStateV.width * ctx->scale;
			scrollBarWidthFull -= scrollViewScrollBarElemStateV.width * ctx->scale;
		}

		Rect rectScrollBarH =
		{
			rectNoBorders.x,
			rectNoBorders.bottom() - scrollViewScrollBarElemStateH.height * ctx->scale,
			scrollBarWidthFull,
			scrollViewScrollBarElemStateH.height * ctx->scale
		};

		updateScrollMax(scrollViewState.horizontal, scrollContentH, scrollAreaWidth);
		f32 handleSize = computeHandleSize(scrollBarWidth, scrollContentH, scrollAreaWidth, ctx->settings.minScrollViewHandleSize);
		f32 handleOffset = computeHandleOffset(scrollViewState.horizontal, scrollBarWidth, handleSize);

		Rect rectScrollBarHandleH =
		{
			rectScrollBarH.x + handleOffset + scrollViewScrollBarElemStateH.border * ctx->scale,
			rectNoBorders.bottom() - scrollViewScrollBarElemStateH.height * ctx->scale + (scrollViewScrollBarElemStateH.height - scrollViewScrollThumbElemStateH.height) * ctx->scale * 0.5f,
			handleSize,
			scrollViewScrollThumbElemStateH.height * ctx->scale
		};

		if (ctx->isActiveLayer())
		{
			if (rectScrollBarHandleH.contains(ctx->mousePosition)
				|| (scrollViewState.horizontal.draggingThumb && ctx->dragScrollViewHandleWidgetId == scrollViewState.id))
			{
				scrollViewScrollThumbElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).getState(WidgetStateType::Hovered);
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer() && !ctx->widget.disabled)
		{
			if (rectScrollBarHandleH.contains(ctx->mousePosition))
			{
				windowSetCapture();  // Capture mouse to get events outside window
				scrollViewState.horizontal.draggingThumb = true;
				scrollViewState.horizontal.dragDelta = ctx->mousePosition - rectScrollBarHandleH.topLeft();
				ctx->dragScrollViewHandleWidgetId = scrollViewState.id;
				ctx->widget.focusedId = ctx->id;
				scrollViewState.lastMousePos = ctx->mousePosition;
			}
			else if (rectScrollBarH.contains(ctx->mousePosition))
			{
				applyPageScroll(scrollViewState.horizontal, scrollAreaWidth, ctx->scrollViewScrollPageSize, (ctx->mousePosition.x < rectScrollBarHandleH.x) ? -1.0f : 1.0f);
				scrollOffset.x = scrollViewState.horizontal.scrollOffset;
				scrollViewState.scrollOffset = scrollOffset;
			}
		}
		else if (ctx->mouseMoved
			&& ctx->event.type != InputEvent::Type::MouseUp
			&& scrollViewState.horizontal.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
		{
			// kill event, only we're dragging now
			hui::inputEventCancel();
			applyHandleDrag(
				scrollViewState.horizontal,
				rectScrollBarH.x,
				scrollBarWidth,
				handleSize,
				ctx->mousePosition.x,
				scrollViewState.horizontal.dragDelta.x);
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& scrollViewState.horizontal.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
		{
			scrollViewState.horizontal.draggingThumb = false;
			ctx->dragScrollViewHandleWidgetId = 0;
			ctx->widget.captureId = 0;
			windowReleaseCapture();  // Release mouse capture
		}

		updateScrollMax(scrollViewState.horizontal, scrollContentH, scrollAreaWidth);
		handleSize = computeHandleSize(scrollBarWidth, scrollContentH, scrollAreaWidth, ctx->settings.minScrollViewHandleSize);
		handleOffset = computeHandleOffset(scrollViewState.horizontal, scrollBarWidth, handleSize);

		rectScrollBarHandleH =
		{
			rectScrollBarH.x + handleOffset + scrollViewScrollBarElemStateH.border * ctx->scale,
			rectNoBorders.bottom() - scrollViewScrollBarElemStateH.height * ctx->scale + (scrollViewScrollBarElemStateH.height - scrollViewScrollThumbElemStateH.height) * ctx->scale * 0.5f,
			handleSize,
			scrollViewScrollThumbElemStateH.height * ctx->scale
		};

		// draw horizontal scroll bar
		ctx->renderer.cmdSetColor(scrollViewElemState.color);
		ctx->renderer.cmdDrawImageBordered(scrollViewScrollBarElemStateH.image, scrollViewScrollBarElemStateH.border, rectScrollBarH, ctx->scale);

		// draw horizontal scroll bar thumb
		ctx->renderer.cmdSetColor(scrollViewScrollThumbElemStateH.color);
		ctx->renderer.cmdDrawImageBordered(scrollViewScrollThumbElemStateH.image, scrollViewScrollThumbElemStateH.border, rectScrollBarHandleH, ctx->scale);
	}

	// Final synchronization of authoritative scrollbar offsets.
	// Recompute horizontal/vertical scrollMax before clamping.
	updateScrollMax(scrollViewState.horizontal, scrollContentH, scrollAreaWidth);
	updateScrollMax(scrollViewState.vertical, contentSizeForClamp, scrollAreaV);

	bool draggingThis = (ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
		|| scrollViewState.vertical.draggingThumb
		|| scrollViewState.horizontal.draggingThumb;

	if (!draggingThis)
	{
		// Accept external caller-provided scrollOffset only when not dragging.
		// Mirror the caller values into the authoritative per-axis scrollbar states,
		// clamping to the recomputed scrollMax above.
		scrollViewState.horizontal.scrollOffset = std::clamp(scrollOffset.x, 0.0f, scrollViewState.horizontal.scrollMax);
		scrollViewState.vertical.scrollOffset = std::clamp(scrollOffset.y, 0.0f, scrollViewState.vertical.scrollMax);
	}
	else
	{
		// While dragging, the per-axis scrollbar states are authoritative.
		// Use them as the scrollOffset to return to the caller.
		scrollOffset.x = scrollViewState.horizontal.scrollOffset;
		scrollOffset.y = scrollViewState.vertical.scrollOffset;
	}

	// Persist combined authoritative offset and return it.
	scrollViewState.scrollOffset = scrollOffset;
	widgetPopDisabled();
	widgetPopPosition();
	addWidget(height/ctx->scale);
	layoutPop();

	return scrollOffset;
}

void virtualListContentBegin(u32 totalRowCount, f32 itemHeight, f32 scrollPos)
{
	f32 skipRows = scrollPos / itemHeight;
	auto pos = ctx->position;
	ctx->position = { pos.x, pos.y + (i32)skipRows * itemHeight };
	ctx->virtualListStack.push_back(VirtualListContentState());
	ctx->virtualListStack.back().totalRowCount = totalRowCount;
	ctx->virtualListStack.back().itemHeight = itemHeight;
	ctx->virtualListStack.back().lastPosition = pos;
}

void virtualListContentEnd()
{
	hui::widgetSetPosition(
		{
			ctx->virtualListStack.back().lastPosition.x,
			ctx->virtualListStack.back().lastPosition.y + ctx->virtualListStack.back().totalHeight
		});
	ctx->virtualListStack.pop_back();
}

void virtualListContentBegin(VirtualScrollInfo& info)
{
	// Determine current scroll view context
	WidgetId svId = ctx->layout.id;
	auto& svState = ctx->scrollViewState[svId];

	// We will always measure the item height on the first nextStep() call.
	// Do not set a provisional virtualSize.y based on an estimated height.
	// Let the measured content size (or measured total after measurement) be authoritative.
	svState.virtualSize.y = 0.0f;

	// Keep info.itemHeight as-is (0 = auto). Initialize internal virtual-list state.
	ctx->virtualListStack.push_back(VirtualListContentState());
	auto& vstate = ctx->virtualListStack.back();
	vstate.totalRowCount = info.totalItemCount;
	vstate.itemHeight = 0.0f;
	vstate.totalHeight = 0.0f;
	vstate.lastPosition = ctx->position;
	vstate.pendingScrollToIndex = -1;
	vstate.pendingScrollToMode = ScrollToItemSnapMode::Minimal;

	// Reset internal advance state so the caller can start stepping.
	info._started = false;
	info._step = 0;
	info._measureStartY = 0.0f;
	info._measuredItemHeight = 0.0f;
	info.startIndex = 0;
	info.endIndex = 0;
	info.scrollOffsetY = svState.scrollOffset.y;
}

bool VirtualScrollInfo::nextStep()
{
	// If we've finished all steps, no further ranges.
	if (_started)
		return false;

	// Acquire current scroll view info
	WidgetId svId = ctx->layout.id;
	auto& svState = ctx->scrollViewState[svId];
	f32 scrollY = svState.scrollOffset.y;
	f32 viewHeight = ctx->layout.height;

	// baseline position where the virtual list starts (saved in beginVirtualListContent)
	if (ctx->virtualListStack.empty())
	{
		// Defensive: nothing to do if stack missing
		_started = true;
		return false;
	}

	auto& vstate = ctx->virtualListStack.back();
	Point basePos = vstate.lastPosition;

	const i32 buffer = 0;

	// Step 0: issue first-visible single item so caller can render it and we can measure.
	if (_step == 0)
	{
		// Compute a robust guess of the first visible index:
		// Prefer a known itemHeight (info.itemHeight), then previously measured height (vstate.itemHeight).
		// Fall back to an estimate based on the view height (avoid dividing by zero).
		f32 estimateH = 0.0f;
		if (itemHeight > 0.0f)
			estimateH = itemHeight;
		else if (vstate.itemHeight > 0.0f)
			estimateH = vstate.itemHeight;
		else
		{
			// fallback: assume ~10 items visible to get a reasonable estimate
			u32 denom = std::min<u32>(10, std::max<u32>(1, totalItemCount));
			estimateH = std::max(1.0f, viewHeight / (f32)denom);
		}

		i32 firstVisible = 0;
		if (estimateH > 0.0f)
		{
			firstVisible = (i32)std::floor(scrollY / estimateH);
		}
		// clamp
		if (firstVisible < 0) firstVisible = 0;
		if ((u32)firstVisible >= totalItemCount)
			firstVisible = (i32)std::max(0u, totalItemCount - 1);

		startIndex = (u32)firstVisible;
		endIndex = (u32)firstVisible;
		scrollOffsetY = scrollY;
		_measureStartY = ctx->position.y;
		_step = 1;

		//std::printf("[VIRT] step0 startIndex=%u scrollY=%.3f estimateH=%.3f _measureStartY=%.3f basePos.y=%.3f\n",
		//	startIndex, scrollY, estimateH, _measureStartY, basePos.y);

		return true;
	}

	// Step 1: measure height based on how much ctx->position advanced while the caller drew the first item,
	// then issue the remaining visible range (excluding the first measured item).
	if (_step == 1)
	{
		// measure
		f32 afterY = ctx->position.y;
		f32 measuredH = afterY - _measureStartY;

		/*std::printf("[VIRT] pre-measure _measureStartY=%.3f afterY=%.3f rawMeasuredH=%.3f viewH=%.3f\n",
			_measureStartY, afterY, measuredH, viewHeight);*/

		// Guard against zero or NaN measured heights to avoid division by zero or crazy indices.
		// Also guard against implausibly large measurements (likely caused by header finishing or other layout moves).
		bool measuredInvalid = false;
		if (!(measuredH > 0.0f) || !std::isfinite(measuredH))
			measuredInvalid = true;
		// treat measurements larger than the viewport as suspicious (e.g. header + row)
		// allow a small margin (90% of view) but clamp very large values
		if (!measuredInvalid && viewHeight > 0.0f)
		{
			const f32 LARGE_MEASURE_RATIO = 0.9f; // if measuredH > 90% of viewHeight, treat as invalid
			const f32 HARD_MAX = 10000.0f;        // absolute sanity cap
			if (measuredH > viewHeight * LARGE_MEASURE_RATIO || measuredH > HARD_MAX)
				measuredInvalid = true;
		}

		if (measuredInvalid)
		{
			// Prefer declared itemHeight if available, otherwise use a conservative default.
			if (itemHeight > 0.0f)
				measuredH = itemHeight;
			else
				measuredH = std::max(1.0f, viewHeight / 10.0f);
		}

		// store measured values
		svState.virtualSize.y = (f32)totalItemCount * measuredH;
		vstate.itemHeight = measuredH;
		vstate.totalHeight = svState.virtualSize.y;
		itemHeight = measuredH;
		_measuredItemHeight = measuredH;

		if (vstate.pendingScrollToIndex >= 0)
		{
			f32 startY = vstate.lastPosition.y - ctx->layout.savedPosition.y;
			ScrollToItemBounds boundsY;
			boundsY.min = startY + (f32)vstate.pendingScrollToIndex * measuredH;
			boundsY.max = boundsY.min + measuredH;
			f32 viewSizeY = ctx->layout.height;
			snapToItem(svState.vertical, boundsY, viewSizeY, vstate.pendingScrollToMode);
			svState.scrollOffset.y = svState.vertical.scrollOffset;
			scrollY = svState.scrollOffset.y; // update local scrollY so visible range calculation is correct
			vstate.pendingScrollToIndex = -1;
		}

		// compute the final visible range using measured height
		i32 firstVisible = (i32)std::floor(scrollY / measuredH) - 1; // give a small above-buffer
		if (firstVisible < 0) firstVisible = 0;
		if ((u32)firstVisible >= totalItemCount)
			firstVisible = (i32)std::max(0u, totalItemCount - 1);

		// how many items approximately fit in view
		i32 approxVisible = (i32)std::ceil(viewHeight / measuredH) + 1; // +1 for safety

		// clamp so we don't overflow total count
		if ((u64)firstVisible + (u64)approxVisible >= (u64)totalItemCount)
		{
			approxVisible = (i32)std::max<i64>(0, (i64)totalItemCount - (i64)firstVisible - 1);
		}

		i32 lastVisible = firstVisible + approxVisible + buffer;
		if (lastVisible < firstVisible) lastVisible = firstVisible;
		if ((u32)lastVisible >= totalItemCount) lastVisible = (i32)std::max(0u, totalItemCount - 1);

		// We already drew one item (the one returned in step 0). Start remaining from firstVisible+1 if that same index was drawn,
		// otherwise start at firstVisible (covers the case where our initial guess was off but measurement corrected it).
		i32 remainingStart = firstVisible;
		// if the caller drew exactly firstVisible earlier, skip it
		if (startIndex == (u32)firstVisible)
			remainingStart = firstVisible + 1;

		if (remainingStart < 0) remainingStart = 0;
		if ((u32)remainingStart >= totalItemCount) remainingStart = (i32)std::max(0u, totalItemCount - 1);

		startIndex = (u32)remainingStart;
		endIndex = (u32)lastVisible;
		scrollOffsetY = scrollY;

		// position the pen to the start of the remainingStart item
		f32 skipY = (f32)remainingStart * measuredH;
		ctx->position = { basePos.x, basePos.y + skipY };

		/*std::printf("[VIRT] step1 measuredH=%.3f virtualSize.y=%.3f firstVisible=%d approxVisible=%d remainingStart=%d skipY=%.3f\n",
			measuredH, svState.virtualSize.y, firstVisible, approxVisible, remainingStart, (f32)remainingStart * measuredH);*/

		// mark finished after this step
		_step = 2;
		_started = true; // next call will return false
		return true;
	}

	// any other case -> done
	_started = true;
	return false;
}

void scrollViewScrollToWidget(WidgetId id, ScrollToItemSnapMode mode)
{
	WidgetId svId = 0;
	LayoutState* svLayout = nullptr;
	if (ctx->layout.type == LayoutType::ScrollView)
	{
		svId = ctx->layout.id;
		svLayout = &ctx->layout;
	}
	else
	{
		for (auto it = ctx->layoutStack.rbegin(); it != ctx->layoutStack.rend(); ++it)
		{
			if (it->type == LayoutType::ScrollView)
			{
				svId = it->id;
				svLayout = &(*it);
				break;
			}
		}
	}

	if (!svId)
		return;

	auto& svState = ctx->scrollViewState[svId];

	if (!ctx->virtualListStack.empty())
	{
		auto& virtState = ctx->virtualListStack.back();
		f32 itemHeight = virtState.itemHeight;

		// If measurement hasn't completed yet, deduce it from historical sizes or queue it
		if (itemHeight <= 0.0f)
		{
			if (virtState.totalRowCount > 0 && svState.virtualSize.y > 0.0f)
            {
				itemHeight = svState.virtualSize.y / (f32)virtState.totalRowCount;
            }
			else
			{
				virtState.pendingScrollToIndex = (i64)id;
				virtState.pendingScrollToMode = mode;
				return; 
			}
		}

		f32 startY = virtState.lastPosition.y - svLayout->savedPosition.y;

		ScrollToItemBounds boundsY;
		boundsY.min = startY + (f32)id * itemHeight; // using id as index
		boundsY.max = boundsY.min + itemHeight;
		f32 viewSizeY = svLayout->height;
		snapToItem(svState.vertical, boundsY, viewSizeY, mode);
		svState.scrollOffset.y = svState.vertical.scrollOffset;
	}
	else
	{
		Rect targetRect = ctx->widget.rect;

		ScrollToItemBounds boundsY;
		boundsY.min = targetRect.y - svLayout->savedPosition.y;
		boundsY.max = boundsY.min + targetRect.height;
		f32 viewSizeY = svLayout->height;
		snapToItem(svState.vertical, boundsY, viewSizeY, mode);
		svState.scrollOffset.y = svState.vertical.scrollOffset;

		ScrollToItemBounds boundsX;
		boundsX.min = targetRect.x - svLayout->savedPosition.x;
		boundsX.max = boundsX.min + targetRect.width;
		f32 viewSizeX = svLayout->width;
		snapToItem(svState.horizontal, boundsX, viewSizeX, mode);
		svState.scrollOffset.x = svState.horizontal.scrollOffset;
	}
}

}