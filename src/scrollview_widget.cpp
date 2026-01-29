#include <iostream>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
void beginScrollView(const char* id, f32 size, f32 scrollPos, f32 virtualHeight, ScrollViewFlags flags)
{
	auto& scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).normalState();

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
	ctx->scrollViewStack[ctx->scrollViewDepth].id = ctx->id;
	ctx->scrollViewStack[ctx->scrollViewDepth].flags = flags;

	const auto& padding = getPadding(PaddingType::ScrollView);
	const auto border = (has(flags, ScrollViewFlags::NoBorder) ? (f32)scrollViewElemState.border * ctx->scale : 0);
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
	clipRect.width -= scrollViewScrollThumbElemState.width + internalPadding * 2.0f;
	clipRect.height -= border * ctx->scale * 2.0f;

	if (!has(flags, ScrollViewFlags::NoBorder))
	{
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->scale);
	}

	ctx->scrollViewStack[ctx->scrollViewDepth].scrollPosition = scrollPos;
	ctx->renderer->pushClipRect(clipRect);
	pushPosition();
	ctx->position = { clipRect.x, clipRect.y };
	ctx->position.y -= scrollPos;
	pushLayout();
	ctx->layout = LayoutState(LayoutType::ScrollView);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.id = ctx->id;
	ctx->layout.width = clipRect.width;
	ctx->layout.height = clipRect.height;
	ctx->scrollViewDepth++;
}

f32 endScrollView()
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
	f32 size = scrollViewInfo.size;
	const auto& padding = getPadding(PaddingType::ScrollView);
	const auto border = (has(scrollViewInfo.flags, ScrollViewFlags::NoBorder) ? (f32)scrollViewElemState.border * ctx->scale : 0);
	auto internalPadding = border + padding.x;
	f32 scrollContentSize = ctx->position.y - prevPenPos.y + internalPadding; // Add bottom padding to prevent clipping
	f32 scrollAmount = 0;

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
		auto& scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBar).normalState();
		auto& scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).normalState();

		Rect rectScrollBar =
		{
			rect.right() - scrollViewScrollBarElemState.width * ctx->scale,
			rect.y,
			scrollViewScrollBarElemState.width * ctx->scale,
			rect.height
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
				scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).getState(WidgetStateType::Hovered);
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
		{
			if (rectScrollBarHandle.contains(ctx->mousePosition))
			{
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
			&& ctx->isActiveLayer()
			&& scrollViewInfo.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.id)
		{
			scrollViewInfo.draggingThumb = false;
			ctx->dragScrollViewHandleWidgetId = 0;
		}

		// draw scroll bar line
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollBarElemState.image, scrollViewScrollBarElemState.border, rectScrollBar, ctx->scale);

		// draw scroll bar thumb
		ctx->renderer->cmdSetColor(scrollViewScrollThumbElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollThumbElemState.image, scrollViewScrollThumbElemState.border, rectScrollBarHandle, ctx->scale);
	}

	scrollPos = (u32)scrollPos;
	popPosition();
	addWidget(size);
	popLayout();

	return scrollPos;
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