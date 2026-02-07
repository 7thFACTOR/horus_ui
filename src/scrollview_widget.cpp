#include <iostream>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
struct ItemRect
{
    f32 min; // start (top / left) in content space
    f32 max; // end   (bottom / right) in content space
};

enum class SnapMode
{
    Minimal,
    AlignStart,
    AlignCenter,
    AlignEnd
};

inline void updateScrollMax(
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

inline f32 computeHandleSize(
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

inline f32 computeHandleOffset(
    const ScrollbarState& state,
    f32 scrollBarSize,
    f32 handleSize)
{
    if (state.scrollMax <= 0.0f)
        return 0.0f;

    f32 slack = scrollBarSize - handleSize;
    if (slack <= 0.0f)
        return 0.0f;

    return (state.scrollOffset / state.scrollMax) * slack;
}

inline void applyHandleDrag(
    ScrollbarState& state,
    f32 mouseDelta,
    f32 scrollBarSize,
    f32 handleSize)
{
    f32 slack = scrollBarSize - handleSize;
    if (slack <= 0.0f || state.scrollMax <= 0.0f)
        return;

    f32 ratio = mouseDelta / slack;
    state.scrollOffset += ratio * state.scrollMax;
    state.scrollOffset = std::clamp(
        state.scrollOffset, 0.0f, state.scrollMax);
}

inline void applyPageScroll(
    ScrollbarState& state,
    f32 viewSize,
    f32 pageSizeNormalized, // 0..1
    f32 direction)          // -1 or +1
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

inline void snapToItem(
    ScrollbarState& state,
    const ItemRect& item,
    f32 viewSize,
    SnapMode mode)
{
    if (state.scrollMax <= 0.0f)
        return;

    f32 targetScroll = state.scrollOffset;

    switch (mode)
    {
        case SnapMode::Minimal:
        {
            if (item.min < state.scrollOffset)
                targetScroll = item.min;
            else if (item.max > state.scrollOffset + viewSize)
                targetScroll = item.max - viewSize;
            break;
        }

        case SnapMode::AlignStart:
            targetScroll = item.min;
            break;

        case SnapMode::AlignCenter:
        {
            f32 itemCenter = (item.min + item.max) * 0.5f;
            targetScroll = itemCenter - viewSize * 0.5f;
            break;
        }

        case SnapMode::AlignEnd:
            targetScroll = item.max - viewSize;
            break;
    }

    state.scrollOffset = std::clamp(
        targetScroll, 0.0f, state.scrollMax);
}

void beginScrollView(const char* id, f32 size, f32 scrollPos)
{
	beginScrollView(id, size, scrollPos, 0, ScrollViewFlags::None, 0.0f, 0.0f);
}

void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight)
{
	beginScrollView(id, size, scrollPos, virtualHeight, ScrollViewFlags::None, 0.0f, 0.0f);
}

void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight, ScrollViewFlags flags)
{
	beginScrollView(id, size, scrollPos, virtualHeight, flags, 0.0f, 0.0f);
}

// Main implementation with all parameters
void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight, ScrollViewFlags flags, f32 scrollPosX, f32 virtualWidth)
{
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
	auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();

	ctx->id = genId(id);

	if (size <= 0.0f)
	{
		// Use remaining height in layout
		size = getRemainingHeight();
		if (size <= 0)
		{
			// Fallback to a reasonable default if no space available
			size = 10.0f;
		}
	}

	if (ctx->settings.scaleScrollViewHeight)
		size *= ctx->scale;

	auto& scrollViewInfo = ctx->scrollViewStack[ctx->scrollViewDepth];
	scrollViewInfo.size = size;
	scrollViewInfo.virtualHeight = virtualHeight;
	scrollViewInfo.virtualWidth = virtualWidth;
	scrollViewInfo.id = ctx->id;
	scrollViewInfo.flags = flags;

	const auto& padding = getPadding(PaddingType::ScrollView);
	const auto border = (has(flags, ScrollViewFlags::NoBorder) ? 0 : (f32)scrollViewElemState.border * ctx->scale);
	auto internalPadding = border + padding.x;

	Rect rect =
	{
		round(ctx->position.x),
		round(ctx->position.y),
		ctx->layout.width,
		size
	};

	scrollViewInfo.rect = rect;

	// Load persistent state
	auto& persistentState = ctx->widgetScrollStates[ctx->id];
	scrollViewInfo.draggingThumb = persistentState.draggingThumb;
	scrollViewInfo.dragDelta = persistentState.dragDelta;
	scrollViewInfo.draggingThumbX = persistentState.draggingThumbX;
	scrollViewInfo.dragDeltaX = persistentState.dragDeltaX;
	scrollViewInfo.wasHorizontalScrollbarVisible = persistentState.wasHorizontalScrollbarVisible;

	Rect clipRect = rect;

	clipRect.x += internalPadding;
	clipRect.y += border;
	clipRect.width -= scrollViewScrollBarElemStateV.width + internalPadding * 2.0f;
	clipRect.height -= border * ctx->scale * 2.0f;

	// Reserve space if we know we need horizontal scrollbar OR if content overflowed (previous frame)
	// Reserve space if we know we need horizontal scrollbar
	// If virtualWidth is 0 (dynamic), we rely on the previous frame's visibility status.
	// This avoids "phantom gaps" when shrinking because if the bar wasn't there (content wrapped), we don't reserve.
	auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

	if (virtualWidth > rect.width || (virtualWidth == 0 && scrollViewInfo.wasHorizontalScrollbarVisible))
	{
		clipRect.height -= scrollViewScrollBarElemStateH.height* ctx->scale + padding.y;
	}

	if (!has(flags, ScrollViewFlags::NoBorder))
	{
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->scale);
	}

	scrollViewInfo.scrollPosition = scrollPos;
	scrollViewInfo.scrollPositionX = scrollPosX;
	scrollViewInfo.maxContentX = 0.0f; // Reset max content X

	ctx->renderer->pushClipRect(clipRect);
	pushPosition();
	ctx->position = { clipRect.x, clipRect.y };
	ctx->position.y -= scrollPos;
	ctx->position.x -= scrollPosX;  // Apply horizontal scroll offset
	pushLayout();
	ctx->layout = LayoutState(LayoutType::ScrollView);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.id = ctx->id;
	ctx->layout.width = clipRect.width;
	ctx->layout.height = clipRect.height;
	ctx->scrollViewDepth++;
}

Point endScrollView()
{
	ctx->id = ctx->layout.id;
	ctx->scrollViewDepth--;
	auto prevPenPos = ctx->layout.savedPosition;
	auto clipRect = ctx->renderer->getClipRect();
	ctx->renderer->popClipRect();
	auto& scrollViewInfo = ctx->scrollViewStack[ctx->scrollViewDepth];
	const auto& fullRect = scrollViewInfo.rect;
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	f32 scrollPos = scrollViewInfo.scrollPosition;
	f32 scrollPosX = scrollViewInfo.scrollPositionX;
	f32 size = scrollViewInfo.size;
	const auto& padding = getPadding(PaddingType::ScrollView);
	const auto border = (has(scrollViewInfo.flags, ScrollViewFlags::NoBorder) ? 0 : (f32)scrollViewElemState.border * ctx->scale);
	auto internalPaddingX = border + padding.x;
	auto internalPaddingY = border + padding.y;
	f32 scrollContentSize = ctx->position.y - prevPenPos.y + internalPaddingY; // Add bottom padding to prevent clipping

	// make the rect for the scrollbars, without the UI element border
	auto rectNoBorders = fullRect.contract(scrollViewElemState.border);
	auto scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
	auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();

	// Use maxContentX (highest X reached) for horizontal content width, not final position
	f32 scrollContentWidth = scrollViewInfo.maxContentX - rectNoBorders.x - internalPaddingX;
	f32 scrollAmount = 0;
	f32 scrollAmountX = 0;
	bool hasHorizontalScrollbar = !has(scrollViewInfo.flags, ScrollViewFlags::NoHorizontalScroll) &&
		(scrollViewInfo.virtualWidth > 0 || scrollContentWidth > rectNoBorders.width);
	f32 effectiveViewHeight = rectNoBorders.height;
	if (hasHorizontalScrollbar)
	{
		effectiveViewHeight -= scrollViewScrollBarElemStateH.height * ctx->scale;
	}

	ctx->renderer->cmdDrawRectangle(
		{
			rectNoBorders.x, rectNoBorders.y + scrollContentSize,

		 scrollContentWidth, 11}
	);

	// scroll view with mouse wheel
	if (ctx->event.type == InputEvent::Type::MouseWheel
		&& ctx->isActiveLayer())
	{
		if (rectNoBorders.contains(ctx->mousePosition))
		{
			scrollAmount = ctx->event.mouse.wheel.y * (effectiveViewHeight * ctx->scrollViewSpeed) * ctx->scale;
			scrollPos -= scrollAmount;
			ctx->event.type = InputEvent::Type::None;
			forceRepaint();
		}
	}

	scrollViewInfo.wasHorizontalScrollbarVisible = hasHorizontalScrollbar; // Persist visibility for next frame reservation

	// auto scroll to the focused widget if curent widget changed
	if (ctx->focusChanged && ctx->widget.focusedId == ctx->id)
	{
		if (ctx->widget.focusedWidgetRect.y > rectNoBorders.bottom())
		{
			scrollPos = (ctx->widget.focusedWidgetRect.y + scrollPos) - rectNoBorders.y;
		}
	}

	// if we reached the top and trying to scroll more, just set to 0
	if (scrollPos < 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	// if content is smaller than scroll view, just set pos to 0
	if (scrollContentSize < effectiveViewHeight && fabs(scrollPos) > 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	// if we reached bottom of the content, stop
	if (ctx->position.y + scrollAmount + internalPaddingY < rectNoBorders.y + effectiveViewHeight)
	{
		if (scrollContentSize > effectiveViewHeight)
		{
			scrollPos = scrollContentSize - effectiveViewHeight;
		}
	}

	// draw scrollbar if content is bigger than scroll view
	if (scrollContentSize > effectiveViewHeight)
	{
		auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();  // Use auto, not auto& to avoid mutating cache
		auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

		// Adjust height to not overlap with horizontal scrollbar if present
		f32 scrollBarHeight = rectNoBorders.height;
		if (scrollViewInfo.virtualWidth > 0 || scrollContentWidth > rectNoBorders.width)
		{
			scrollBarHeight -= scrollViewScrollBarElemStateH.height * ctx->scale;
		}

		Rect rectScrollBar =
		{
			rectNoBorders.right() - scrollViewScrollBarElemState.width * ctx->scale,
			rectNoBorders.y,
			scrollViewScrollBarElemState.width * ctx->scale,
			scrollBarHeight
		};

		f32 handleSize = rectScrollBar.height * effectiveViewHeight / scrollContentSize;

		if (handleSize < ctx->settings.minScrollViewHandleSize)
			handleSize = ctx->settings.minScrollViewHandleSize;

		f32 maxScroll = scrollContentSize - effectiveViewHeight;
		f32 handleOffset = (scrollPos / maxScroll) * (rectScrollBar.height - handleSize);

		Rect rectScrollBarHandle =
		{
			rectNoBorders.right() - scrollViewScrollThumbElemState.width * ctx->scale,
			rectScrollBar.y + handleOffset,
			scrollViewScrollThumbElemState.width * ctx->scale,
			handleSize
		};

		if (ctx->isActiveLayer())
		{
			if (rectScrollBarHandle.contains(ctx->mousePosition)
				|| (scrollViewInfo.draggingThumb && ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id))
			{
				scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).getState(WidgetStateType::Hovered);
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
		{
			if (rectScrollBarHandle.contains(ctx->mousePosition))
			{
				setWindowCapture();  // Capture mouse to get events outside window
				scrollViewInfo.draggingThumb = true;
				scrollViewInfo.dragDelta = ctx->mousePosition - rectScrollBarHandle.topLeft();
				ctx->dragScrollViewHandleWidgetId = scrollViewInfo.id;
				ctx->widget.focusedId = ctx->id;
			}
			else if (rectScrollBar.contains(ctx->mousePosition))
			{
				f32 pageSize = (rectNoBorders.height * ctx->scrollViewScrollPageSize);

				// page up
				if (ctx->mousePosition.y < rectScrollBarHandle.y)
				{
					scrollPos -= pageSize;
				}
				// page down
				else if (ctx->mousePosition.y > rectScrollBarHandle.bottom())
				{
					scrollPos += pageSize;
				}
			}
		}
		else if (ctx->mouseMoved
			&& ctx->event.type != InputEvent::Type::MouseUp
			&& scrollViewInfo.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id)
		{
			f32 crtLocalY = ctx->mousePosition.y - scrollViewInfo.dragDelta.y - rectScrollBar.y;
			f32 trackSize = rectScrollBar.height - handleSize;
			f32 percent = crtLocalY / trackSize;
			f32 oldScrollPos = scrollPos;

			// kill event, only we're dragging now
			hui::cancelEvent();
			scrollPos = percent * (scrollContentSize - effectiveViewHeight);
			scrollAmount = oldScrollPos - scrollPos;

			//TODO: duplicated code see above scrollPos correction
			if (scrollPos < 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (scrollContentSize < effectiveViewHeight && fabs(scrollPos) > 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (ctx->position.y + scrollAmount + internalPaddingY < rectNoBorders.y + effectiveViewHeight)
			{
				if (scrollContentSize > effectiveViewHeight)
				{
					scrollPos = scrollContentSize - effectiveViewHeight;
				}
			}
			// end duplicated code

			maxScroll = scrollContentSize - effectiveViewHeight;
			handleOffset = (scrollPos / maxScroll) * (rectScrollBar.height - handleSize);

			rectScrollBarHandle =
			{
				rectNoBorders.right() - scrollViewScrollThumbElemState.width * ctx->scale,
				rectScrollBar.y + handleOffset,
				scrollViewScrollThumbElemState.width * ctx->scale,
				handleSize
			};
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& scrollViewInfo.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id)
		{
			scrollViewInfo.draggingThumb = false;
			ctx->dragScrollViewHandleWidgetId = 0;
			ctx->widget.captureId = 0;
			releaseWindowCapture();  // Release mouse capture
		}

		// draw scroll bar line
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollBarElemState.image, scrollViewScrollBarElemState.border, rectScrollBar, ctx->scale);

		// draw scroll bar thumb
		ctx->renderer->cmdSetColor(scrollViewScrollThumbElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollThumbElemState.image, scrollViewScrollThumbElemState.border, rectScrollBarHandle, ctx->scale);
	}

	f32 scrollAreaWidth = rectNoBorders.width - padding.x * 2.0f;

	// ========== HORIZONTAL SCROLLBAR ==========
	// Draw horizontal scrollbar if content is wider than view and virtualWidth is set
	if (hasHorizontalScrollbar)
	{
		auto& scrollViewScrollBarElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
		auto& scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).normalState();
		// Adjust width to not overlap with vertical scrollbar if present
		f32 scrollBarWidth = rectNoBorders.width;

		if (scrollContentSize > rectNoBorders.height)
		{
			auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
			scrollBarWidth -= scrollViewScrollBarElemStateV.width * ctx->scale;
			scrollAreaWidth -= scrollViewScrollBarElemStateV.width * ctx->scale;
		}

		Rect rectScrollBarX =
		{
			rectNoBorders.x,
			rectNoBorders.bottom() - scrollViewScrollBarElemState.height * ctx->scale,
			scrollBarWidth,
			scrollViewScrollBarElemState.height * ctx->scale
		};
		
		f32 handleSizeX = rectScrollBarX.width * scrollBarWidth / scrollContentWidth;

		if (handleSizeX < ctx->settings.minScrollViewHandleSize)
			handleSizeX = ctx->settings.minScrollViewHandleSize;

		f32 maxScrollX = scrollContentWidth - rectNoBorders.width;
		f32 handleOffsetX = (scrollPosX / maxScrollX) * (scrollBarWidth - handleSizeX);  // Use scrollBarWidth for visual positioning

		Rect rectScrollBarHandleX =
		{
			rectScrollBarX.x + handleOffsetX,
			rectNoBorders.bottom() - scrollViewScrollThumbElemState.height * ctx->scale,
			handleSizeX,
			scrollViewScrollThumbElemState.height * ctx->scale
		};

		if (ctx->isActiveLayer())
		{
			if (rectScrollBarHandleX.contains(ctx->mousePosition)
				|| (scrollViewInfo.draggingThumbX && ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id))
			{
				scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).getState(WidgetStateType::Hovered);
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
		{
			if (rectScrollBarHandleX.contains(ctx->mousePosition))
			{
				setWindowCapture();  // Capture mouse to get events outside window
				scrollViewInfo.draggingThumbX = true;
				scrollViewInfo.dragDeltaX = ctx->mousePosition - rectScrollBarHandleX.topLeft();
				ctx->dragScrollViewHandleWidgetId = scrollViewInfo.id;
				ctx->widget.focusedId = ctx->id;
			}
			else if (rectScrollBarX.contains(ctx->mousePosition))
			{
				f32 pageSize = (rectNoBorders.width * ctx->scrollViewScrollPageSize);

				// page left
				if (ctx->mousePosition.x < rectScrollBarHandleX.x)
				{
					scrollPosX -= pageSize;
				}
				// page right
				else if (ctx->mousePosition.x > rectScrollBarHandleX.right())
				{
					scrollPosX += pageSize;
				}
			}
		}
		else if (ctx->mouseMoved
			&& ctx->event.type != InputEvent::Type::MouseUp
			&& scrollViewInfo.draggingThumbX
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id)
		{
			f32 crtLocalX = ctx->mousePosition.x - scrollViewInfo.dragDeltaX.x - rectNoBorders.x;
			f32 trackSizeX = scrollBarWidth - handleSizeX;
			f32 percentX = crtLocalX / trackSizeX;
			f32 oldScrollPosX = scrollPosX;

			// kill event, only we're dragging now
			hui::cancelEvent();
			scrollPosX = percentX * (scrollContentWidth - scrollAreaWidth);
			scrollAmountX = oldScrollPosX - scrollPosX;

			// Bounds checking (similar to vertical)
			if (scrollPosX < 0)
			{
				scrollPosX = 0;
				forceRepaint();
			}

			if (scrollContentWidth < scrollAreaWidth && fabs(scrollPosX) > 0)
			{
				scrollPosX = 0;
				forceRepaint();
			}

			// Clamp to maximum scroll position
			if (scrollContentWidth > scrollAreaWidth && scrollPosX > (scrollContentWidth - scrollAreaWidth))
			{
				scrollPosX = scrollContentWidth - scrollAreaWidth;
				forceRepaint();
			}

			maxScrollX = scrollContentWidth - scrollAreaWidth;  // Use effective width
			handleOffsetX = (scrollPosX / maxScrollX) * (scrollBarWidth - handleSizeX);

			rectScrollBarHandleX =
			{
				rectScrollBarX.x + handleOffsetX,
				rectNoBorders.bottom() - scrollViewScrollThumbElemState.height * ctx->scale,
				handleSizeX,
				scrollViewScrollThumbElemState.height * ctx->scale
			};
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& scrollViewInfo.draggingThumbX
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id)
		{
			scrollViewInfo.draggingThumbX = false;
			ctx->dragScrollViewHandleWidgetId = 0;
			ctx->widget.captureId = 0;
			releaseWindowCapture();  // Release mouse capture
		}

		// draw horizontal scroll bar line
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollBarElemState.image, scrollViewScrollBarElemState.border, rectScrollBarX, ctx->scale);

		// draw horizontal scroll bar thumb
		ctx->renderer->cmdSetColor(scrollViewScrollThumbElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollThumbElemState.image, scrollViewScrollThumbElemState.border, rectScrollBarHandleX, ctx->scale);
	}

	// Apply horizontal scroll bounds checking, even if we didn't just drag, to handle content changes that might have made the current scroll position invalid
	if (scrollPosX < 0)
	{
		scrollPosX = 0;
		forceRepaint();
	}

	if (scrollContentWidth < scrollAreaWidth && fabs(scrollPosX) > 0)
	{
		scrollPosX = 0;
		forceRepaint();
	}

	if (scrollViewInfo.maxContentX + scrollAmountX + internalPaddingX < rectNoBorders.right())
	{
		if (scrollContentWidth > scrollAreaWidth)
		{
			scrollPosX = scrollContentWidth - scrollAreaWidth;
			forceRepaint();
		}
	}

	popPosition();
	addWidget(size);
	popLayout();

	// Save persistent state
	auto& persistentState = ctx->widgetScrollStates[scrollViewInfo.id];
	persistentState.draggingThumb = scrollViewInfo.draggingThumb;
	persistentState.dragDelta = scrollViewInfo.dragDelta;
	persistentState.draggingThumbX = scrollViewInfo.draggingThumbX;
	persistentState.dragDeltaX = scrollViewInfo.dragDeltaX;
	persistentState.wasHorizontalScrollbarVisible = scrollViewInfo.wasHorizontalScrollbarVisible;
	persistentState.maxContentX = scrollViewInfo.maxContentX;

	return Point(scrollPosX, scrollPos);
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
			ctx->virtualListStack.back().lastPosition.y + ctx->virtualListStack.back().totalRowCount * ctx->virtualListStack.back().itemHeight
		});

	ctx->virtualListStack.pop_back();
}

}