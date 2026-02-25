#include <iostream>
#include <algorithm>
#include <cmath>
#include "context.h"
#include "theme.h"
#include "util.h"
#include "font.h" // for metric fallback when computing default

namespace hui
{
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

void beginScrollView(const char* id, f32 size, f32 scrollPos)
{
	beginScrollView(id, size, { 0, scrollPos }, 0, ScrollViewFlags::None);
}

void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight)
{
	beginScrollView(id, size, { 0, scrollPos }, { 0, virtualHeight }, ScrollViewFlags::None);
}

void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight, ScrollViewFlags flags)
{
	beginScrollView(id, size, { 0, scrollPos }, { 0, virtualHeight }, flags);
}

// Main implementation with all parameters
void beginScrollView(const char* id, f32 height, Point scrollOffset, Point virtualSize, ScrollViewFlags flags)
{
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
	auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();

	ctx->id = genId(id);

	if (height <= 0.0f)
	{
		// Use remaining height in layout
		height = getRemainingHeight();

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

	const auto& padding = getPadding(PaddingType::ScrollView);
	const auto border = (has(flags, ScrollViewFlags::NoBorder) ? 0 : (f32)scrollViewElemState.border * ctx->scale);
	auto internalPadding = border + (has(flags, ScrollViewFlags::NoPadding) ? 0 : padding.x);

	Rect rect =
	{
		round(ctx->position.x),
		round(ctx->position.y),
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

	scrollViewState.scrollOffset = scrollOffset;
	ctx->maxContentWidthStack.push_back(ctx->maxContentWidth);
	ctx->maxContentWidth = 0.0f;

	ctx->renderer.pushClipRect(clipRect);
	pushPosition();
	ctx->position = { clipRect.x, clipRect.y };
	ctx->position -= scrollOffset;
	pushLayout();
	ctx->layout = LayoutState(LayoutType::ScrollView);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.id = ctx->id;
	ctx->layout.width = clipRect.width;
	ctx->layout.height = clipRect.height;
}

Point endScrollView()
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
	const auto& padding = getPadding(PaddingType::ScrollView);
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
        if (ctx->isActiveLayer() && ctx->event.type == InputEvent::Type::MouseWheel)
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

                    // Apply to authoritative scrollbar state and clamp immediately using scrollMax
                    auto& v = scrollViewState.vertical;
                    v.scrollOffset = std::clamp(v.scrollOffset - delta, 0.0f, v.scrollMax);

                    // Mirror authoritative value back into the Point and the overall scrollViewState
                    scrollOffset.y = v.scrollOffset;
                    scrollViewState.scrollOffset = scrollOffset;

                    // consume event and request repaint
                    cancelEvent();
                    forceRepaint();
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

			if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
			{
				if (rectScrollBarHandleV.contains(ctx->mousePosition))
				{
					setWindowCapture();  // Capture mouse to get events outside window
					scrollViewState.vertical.draggingThumb = true;
					scrollViewState.vertical.dragDelta = ctx->mousePosition - rectScrollBarHandleV.topLeft();
					ctx->dragScrollViewHandleWidgetId = scrollViewState.id;
					ctx->widget.focusedId = ctx->id;
					scrollViewState.lastMousePos = ctx->mousePosition;
				}
				else if (rectScrollBarV.contains(ctx->mousePosition))
				{
					applyPageScroll(scrollViewState.vertical, scrollAreaV, ctx->scrollViewScrollPageSize, (ctx->mousePosition.y < rectScrollBarHandleV.y) ? -1.0f : 1.0f);
				}
			}
			else if (ctx->mouseMoved
				&& ctx->event.type != InputEvent::Type::MouseUp
				&& scrollViewState.vertical.draggingThumb
				&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
			{
				// kill event, only we're dragging now
				hui::cancelEvent();
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
				releaseWindowCapture();
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

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
		{
			if (rectScrollBarHandleH.contains(ctx->mousePosition))
			{
				setWindowCapture();  // Capture mouse to get events outside window
				scrollViewState.horizontal.draggingThumb = true;
				scrollViewState.horizontal.dragDelta = ctx->mousePosition - rectScrollBarHandleH.topLeft();
				ctx->dragScrollViewHandleWidgetId = scrollViewState.id;
				ctx->widget.focusedId = ctx->id;
				scrollViewState.lastMousePos = ctx->mousePosition;
			}
			else if (rectScrollBarH.contains(ctx->mousePosition))
			{
				applyPageScroll(scrollViewState.horizontal, scrollAreaWidth, ctx->scrollViewScrollPageSize, (ctx->mousePosition.x < rectScrollBarHandleH.x) ? -1.0f : 1.0f);
			}
		}
		else if (ctx->mouseMoved
			&& ctx->event.type != InputEvent::Type::MouseUp
			&& scrollViewState.horizontal.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewState.id)
		{
			// kill event, only we're dragging now
			hui::cancelEvent();
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
			releaseWindowCapture();  // Release mouse capture
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

	updateScrollMax(scrollViewState.horizontal, scrollContentH, scrollAreaWidth);
	scrollOffset.x = scrollViewState.horizontal.scrollOffset;
	scrollOffset.y = scrollViewState.vertical.scrollOffset;
	scrollViewState.scrollOffset = scrollOffset; // Persist the updated offset to state
	popPosition();
	addWidget(height/ctx->scale);
	popLayout();

	return scrollOffset;
}

void beginVirtualListContent(u32 totalRowCount, f32 itemHeight, f32 scrollPos)
{
	f32 skipRows = scrollPos / itemHeight;
	auto pos = ctx->position;
	ctx->position = { pos.x, pos.y + (i32)skipRows * itemHeight };
	ctx->virtualListStack.push_back(VirtualListContentState());
	ctx->virtualListStack.back().totalRowCount = totalRowCount;
	ctx->virtualListStack.back().itemHeight = itemHeight;
	ctx->virtualListStack.back().lastPosition = pos;
}

void endVirtualListContent()
{
	hui::setPosition(
		{
			ctx->virtualListStack.back().lastPosition.x,
			// ensure we reserve at least the rounded-up total height to avoid underestimating content size
			// Add one extra pixel to avoid off-by-one/float precision cases that made the last item unreachable.
			ctx->virtualListStack.back().lastPosition.y + std::ceil(ctx->virtualListStack.back().totalRowCount * ctx->virtualListStack.back().itemHeight) + 1.0f
		});

	ctx->virtualListStack.pop_back();
}

void beginVirtualListContent(VirtualScrollInfo& info)
{
	// Determine current scroll view context
	WidgetId svId = ctx->layout.id;
	auto& svState = ctx->scrollViewState[svId];

	// Ensure itemHeight is valid (will be measured if left zero)
	f32 itemH = computeDefaultItemHeight(info.itemHeight);

	// Set a provisional authoritative vertical virtual height for the scroll view
	// (virtual-list code owns vertical size now). This is overwritten once measurement completes.
	// Round up the virtual size to avoid floating-point underestimation that can make the last item unreachable.
	// Add a 1-pixel epsilon to cope with rounding/precision and ensure the final item is reachable.
	svState.virtualSize.y = std::ceil(info.totalItemCount * itemH) + 1.0f;

	// Push internal virtual-list state (used by endVirtualListContent to reserve height)
	ctx->virtualListStack.push_back(VirtualListContentState());
	ctx->virtualListStack.back().totalRowCount = info.totalItemCount;
	ctx->virtualListStack.back().itemHeight = itemH;
	ctx->virtualListStack.back().lastPosition = ctx->position;

	// Reset advance internal state so the caller can start stepping.
	info._started = false;
	info._step = 0;
	info._measureStartY = 0.0f;
	info._measuredItemHeight = 0.0f;
	info.firstVisibleItem = 0;
	info.visibleItemCount = 0;
	info.scrollOffsetY = svState.scrollOffset.y;
}

// Step-like API with automatic first-item measurement.
// First advance() call issues a range of 1 item (the first visible item) so caller can draw it and allow us to measure its height.
// Second advance() call computes a measured item height from ctx->position delta and issues the remaining visible range.
// After that advance() returns false.
bool VirtualScrollInfo::advance()
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

	// Approximate item height (used for first-step only if user didn't supply itemHeight).
	f32 approxH = computeDefaultItemHeight(itemHeight);

	const i32 buffer = 3;

	// Step 0: issue first-visible single item so caller can render it and we can measure.
	if (_step == 0)
	{
		i32 firstVisible = (i32)std::floor(scrollY / approxH);
		if (firstVisible < 0) firstVisible = 0;
		if (firstVisible > (i32)totalItemCount - 1) firstVisible = (i32)totalItemCount - 1;

		// issue only the first item for measurement
		firstVisibleItem = (u32)firstVisible;
		visibleItemCount = 1;
		scrollOffsetY = scrollY;

		// position the pen to the start of the first visible item
		f32 skipY = (f32)firstVisible * approxH;
		ctx->position = { basePos.x, basePos.y + skipY };

		// record start y for measurement after the caller draws the first item
		_measureStartY = ctx->position.y;

		_step = 1;
		return true;
	}

	// Step 1: measure height based on how much ctx->position advanced while the caller drew the first item,
	// then issue the remaining visible range (excluding the first measured item).
	if (_step == 1)
	{
		// measure
		f32 afterY = ctx->position.y;
		f32 measuredH = afterY - _measureStartY;

		// fallback if measurement failed
		if (measuredH <= 0.0f)
			measuredH = approxH;

		_measuredItemHeight = measuredH;

		// update the scroll view authoritative virtual size using the measured height
		// round up the total height to avoid floating point underestimation that can leave the last item unreachable.
		// Add a 1-pixel epsilon to cope with rounding/precision and ensure the final item is reachable.
		svState.virtualSize.y = std::ceil(totalItemCount * measuredH) + 1.0f;
		vstate.itemHeight = measuredH;
		vstate.totalHeight = svState.virtualSize.y;

		// compute the final visible range using measured height
		i32 firstVisible = (i32)std::floor(scrollY / measuredH);
		if (firstVisible < 0) firstVisible = 0;

		i32 approxVisible = (i32)std::ceil(viewHeight / measuredH);
		i32 lastVisible = firstVisible + approxVisible + buffer;
		if (lastVisible > (i32)totalItemCount - 1) lastVisible = (i32)totalItemCount - 1;
		if (lastVisible < firstVisible) lastVisible = firstVisible;

		// We already rendered the first visible item; issue the remaining range starting at firstVisible+1
		i32 remainingStart = firstVisible + 1;
		if (remainingStart > lastVisible)
		{
			// nothing more to render
			firstVisibleItem = (u32)remainingStart;
			visibleItemCount = 0;
		}
		else
		{
			firstVisibleItem = (u32)remainingStart;
			visibleItemCount = (u32)(lastVisible - remainingStart + 1);
		}

		scrollOffsetY = scrollY;

		// position the pen to the start of the remainingStart item
		f32 skipY = (f32)remainingStart * measuredH;
		ctx->position = { basePos.x, basePos.y + skipY };

		// mark finished after this step
		_step = 2;
		_started = true; // next call will return false
		return true;
	}

	// any other case -> done
	_started = true;
	return false;
}

}