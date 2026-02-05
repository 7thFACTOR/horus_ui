#include <iostream>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
// Backward compatibility overloads
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
	auto scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();

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

	ctx->scrollViewStack[ctx->scrollViewDepth].size = size;
	ctx->scrollViewStack[ctx->scrollViewDepth].virtualHeight = virtualHeight;
	ctx->scrollViewStack[ctx->scrollViewDepth].virtualWidth = virtualWidth;
	ctx->scrollViewStack[ctx->scrollViewDepth].id = ctx->id;
	ctx->scrollViewStack[ctx->scrollViewDepth].flags = flags;

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

	ctx->scrollViewStack[ctx->scrollViewDepth].rect = rect;

	Rect clipRect = rect;

	clipRect.x += internalPadding;
	clipRect.y += border;
	clipRect.width -= scrollViewScrollThumbElemStateV.width + internalPadding * 2.0f;
	clipRect.height -= border * ctx->scale * 2.0f;
	
	// Always reserve space for horizontal scrollbar to prevent overlap
	// Even if content doesn't need it, this ensures consistency
	auto scrollViewScrollThumbElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).normalState();
	clipRect.height -= scrollViewScrollThumbElemStateH.height * ctx->scale + padding.y;

	if (!has(flags, ScrollViewFlags::NoBorder))
	{
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->scale);
	}

	ctx->scrollViewStack[ctx->scrollViewDepth].scrollPosition = scrollPos;
	ctx->scrollViewStack[ctx->scrollViewDepth].scrollPositionX = scrollPosX;
	ctx->scrollViewStack[ctx->scrollViewDepth].maxContentX = 0.0f; // Reset max content X
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
	auto internalPadding = border + padding.x;
	f32 scrollContentSize = ctx->position.y - prevPenPos.y + internalPadding; // Add bottom padding to prevent clipping
	
	// Use maxContentX (highest X reached) for horizontal content width, not final position
	f32 scrollContentWidth = scrollViewInfo.maxContentX - prevPenPos.x + internalPadding;
	f32 scrollAmount = 0;
	f32 scrollAmountX = 0;

	// make the rect for the scrollbars, without the UI element border
	auto rect = fullRect.contract(scrollViewElemState.border);

	// scroll view with mouse wheel
	if (ctx->event.type == InputEvent::Type::MouseWheel
		&& ctx->isActiveLayer())
	{
		if (rect.contains(ctx->mousePosition))
		{
			scrollAmount = ctx->event.mouse.wheel.y * (fullRect.height * ctx->scrollViewSpeed) * ctx->scale;
			scrollPos -= scrollAmount;
			ctx->event.type = InputEvent::Type::None;
			forceRepaint();
		}
	}

	// auto scroll to the focused widget if curent widget changed
	if (ctx->focusChanged && ctx->widget.focusedId == ctx->id)
	{
		if (ctx->widget.focusedWidgetRect.y > rect.bottom())
		{
			scrollPos = (ctx->widget.focusedWidgetRect.y + scrollPos) - rect.y;
		}
	}

	// if we reached the top and trying to scroll more, just set to 0
	if (scrollPos < 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	// if content is smaller than scroll view, just set pos to 0
	if (scrollContentSize < rect.height && fabs(scrollPos) > 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	// if we reached bottom of the content, stop
	if (ctx->position.y + scrollAmount < rect.bottom())
	{
		if (scrollContentSize > rect.height)
		{
			scrollPos = scrollContentSize - rect.height;
		}
	}

	// draw scrollbar if content is bigger than scroll view
	if (scrollContentSize > rect.height)
	{
		auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();  // Use auto, not auto& to avoid mutating cache
		auto& scrollViewScrollBarElemStateH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

		// Adjust height to not overlap with horizontal scrollbar if present
		f32 scrollBarHeight = rect.height;
		if (scrollViewInfo.virtualWidth > 0 || scrollContentWidth > rect.width)
		{
			scrollBarHeight -= scrollViewScrollBarElemStateH.height * ctx->scale;
		}

		Rect rectScrollBar =
		{
			rect.right() - scrollViewScrollBarElemState.width * ctx->scale,
			rect.y,
			scrollViewScrollBarElemState.width * ctx->scale,
			scrollBarHeight
		};

		f32 handleSize = rectScrollBar.height * rectScrollBar.height / scrollContentSize;

		if (handleSize < ctx->settings.minScrollViewHandleSize)
			handleSize = ctx->settings.minScrollViewHandleSize;

		f32 maxScroll = scrollContentSize - rectScrollBar.height;
		f32 handleOffset = (scrollPos / maxScroll) * (rectScrollBar.height - handleSize);

		Rect rectScrollBarHandle =
		{
			rect.right() - scrollViewScrollThumbElemState.width * ctx->scale,
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
				f32 pageSize = (rect.height * ctx->scrollViewScrollPageSize);

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
			f32 crtLocalY = ctx->mousePosition.y - scrollViewInfo.dragDelta.y - rect.y;
			f32 trackSize = rect.height - handleSize;
			f32 percent = crtLocalY / trackSize;
			f32 oldScrollPos = scrollPos;

			// kill event, only we're dragging now
			hui::cancelEvent();
			scrollPos = percent * (scrollContentSize - rect.height);
			scrollAmount = oldScrollPos - scrollPos;

			//TODO: duplicated code see above scrollPos correction
			if (scrollPos < 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (scrollContentSize < rect.height && fabs(scrollPos) > 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (ctx->position.y + scrollAmount < rect.bottom())
			{
				if (scrollContentSize > rect.height)
				{
					scrollPos = scrollContentSize - rect.height;
				}
			}
			// end duplicated code

			maxScroll = scrollContentSize - rectScrollBar.height;
			handleOffset = (scrollPos / maxScroll) * (rectScrollBar.height - handleSize);

			rectScrollBarHandle =
			{
				rect.right() - scrollViewScrollThumbElemState.width * ctx->scale,
				rect.y + handleOffset,
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

	// ========== HORIZONTAL SCROLLBAR ==========
	// Draw horizontal scrollbar if content is wider than view and virtualWidth is set
	if (scrollViewInfo.virtualWidth > 0 || scrollContentWidth > rect.width)
	{
		auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();
		auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbH).normalState();  // Use auto, not auto& to avoid mutating cache

		// Adjust width to not overlap with vertical scrollbar if present
		f32 scrollBarWidth = rect.width;
		if (scrollContentSize > rect.height)
		{
			auto& scrollViewScrollThumbElemStateV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumbV).normalState();
			scrollBarWidth -= scrollViewScrollThumbElemStateV.width * ctx->scale;
		}

		Rect rectScrollBarX =
		{
			rect.x,
			rect.bottom() - border - scrollViewScrollBarElemState.height * ctx->scale,
			scrollBarWidth,
			scrollViewScrollBarElemState.height * ctx->scale
		};

		f32 handleSizeX = rectScrollBarX.width * rectScrollBarX.width / scrollContentWidth;

		if (handleSizeX < ctx->settings.minScrollViewHandleSize)
			handleSizeX = ctx->settings.minScrollViewHandleSize;

		f32 maxScrollX = scrollContentWidth - rect.width;  // Use rect.width for scroll range
		f32 handleOffsetX = (scrollPosX / maxScrollX) * (scrollBarWidth - handleSizeX);  // Use scrollBarWidth for visual positioning

		Rect rectScrollBarHandleX =
		{
			rectScrollBarX.x + handleOffsetX,
			rect.bottom() - border - scrollViewScrollThumbElemState.height * ctx->scale,
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
				f32 pageSize = (rect.width * ctx->scrollViewScrollPageSize);

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
			f32 crtLocalX = ctx->mousePosition.x - scrollViewInfo.dragDeltaX.x - rect.x;
			f32 trackSizeX = scrollBarWidth - handleSizeX;  // Use scrollBarWidth, not rect.width
			f32 percentX = crtLocalX / trackSizeX;
			f32 oldScrollPosX = scrollPosX;

			// kill event, only we're dragging now
			hui::cancelEvent();
			scrollPosX = percentX * (scrollContentWidth - rect.width);
			scrollAmountX = oldScrollPosX - scrollPosX;

			// Bounds checking (similar to vertical)
			if (scrollPosX < 0)
			{
				scrollPosX = 0;
				forceRepaint();
			}

			if (scrollContentWidth < rect.width && fabs(scrollPosX) > 0)
			{
				scrollPosX = 0;
				forceRepaint();
			}

			// Clamp to maximum scroll position
			if (scrollContentWidth > rect.width && scrollPosX > (scrollContentWidth - rect.width))
			{
				scrollPosX = scrollContentWidth - rect.width;
				forceRepaint();
			}

			maxScrollX = scrollContentWidth - rect.width;  // Use rect.width, not scrollBarWidth
			handleOffsetX = (scrollPosX / maxScrollX) * (scrollBarWidth - handleSizeX);

			rectScrollBarHandleX =
			{
				rectScrollBarX.x + handleOffsetX,
				rect.bottom() - border - scrollViewScrollThumbElemState.height * ctx->scale,
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

	// Apply horizontal scroll bounds checking
	if (scrollPosX < 0)
	{
		scrollPosX = 0;
		forceRepaint();
	}

	if (scrollContentWidth < rect.width && fabs(scrollPosX) > 0)
	{
		scrollPosX = 0;
		forceRepaint();
	}

	if (scrollContentWidth > rect.width && scrollPosX > (scrollContentWidth - rect.width))
	{
		scrollPosX = scrollContentWidth - rect.width;
		forceRepaint();
	}

	scrollPos = (u32)scrollPos;
	scrollPosX = (u32)scrollPosX;
	popPosition();
	addWidget(size);
	popLayout();

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