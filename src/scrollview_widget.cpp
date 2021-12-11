#include "horus.h"
#include "types.h"
#include "theme.h"
#include "context.h"
#include "util.h"

namespace hui
{
void beginScrollView(f32 size, f32 scrollPos, f32 virtualHeight)
{
	auto scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).normalState();

	ctx->scrollViewStack[ctx->scrollViewDepth].size = size * ctx->globalScale;
	ctx->scrollViewStack[ctx->scrollViewDepth].virtualHeight = virtualHeight;
	ctx->scrollViewStack[ctx->scrollViewDepth].widgetId = ctx->currentWidgetId;
	ctx->penStack.push_back(ctx->penPosition);
	size *= ctx->globalScale;
	size -= ctx->padding * ctx->globalScale;

	Rect rect =
	{
		round(ctx->penPosition.x + ctx->padding * ctx->globalScale),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width - ctx->padding * 2,
		size
	};

	Rect clipRect =
	{
		round(ctx->penPosition.x + ctx->padding * ctx->globalScale),
		round(ctx->penPosition.y + (f32)scrollViewElemState.border * ctx->globalScale),
		ctx->layoutStack.back().width - ctx->padding * 2.0f * ctx->globalScale,
		size - (f32)scrollViewElemState.border * 2.0f * ctx->globalScale
	};

	ctx->renderer->cmdSetColor(scrollViewElemState.color);
	ctx->renderer->cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->globalScale);

	ctx->scrollViewStack[ctx->scrollViewDepth].scrollPosition = scrollPos;
	ctx->renderer->pushClipRect(clipRect);
	ctx->penPosition.y -= scrollPos;
	ctx->penPosition.y += scrollViewElemState.border * ctx->globalScale;

	ctx->layoutStack.push_back(LayoutState(LayoutType::ScrollView));
	ctx->layoutStack.back().position = { clipRect.x + (f32)scrollViewElemState.border * ctx->globalScale, clipRect.y };
	ctx->layoutStack.back().width = clipRect.width - (f32)scrollViewElemState.border * 2.0f * ctx->globalScale - scrollViewScrollThumbElemState.width * ctx->globalScale;
	ctx->layoutStack.back().savedPenPosition = ctx->penPosition;
	ctx->penPosition.x = ctx->layoutStack.back().position.x;

	ctx->scrollViewDepth++;
}

f32 endScrollView()
{
	ctx->scrollViewDepth--;
	ctx->layoutStack.pop_back();

	auto clipRect = ctx->renderer->getClipRect();
	ctx->renderer->popClipRect();
	auto& scrollViewInfo = ctx->scrollViewStack[ctx->scrollViewDepth];

	auto scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	f32 scrollPos = scrollViewInfo.scrollPosition;
	f32 size = scrollViewInfo.size;
	f32 contentY = ctx->penStack.back().y - scrollPos;
	f32 scrollContentSize = ctx->penPosition.y - contentY;
	f32 scrollAmount = 0;

	if (ctx->event.type == InputEvent::Type::MouseWheel
		&& ctx->isActiveLayer())
	{
		if (clipRect.contains(ctx->event.mouse.point))
		{
			scrollAmount = ctx->event.mouse.wheel.y * (clipRect.height * ctx->scrollViewSpeed) * ctx->globalScale;
			scrollPos -= scrollAmount;
			forceRepaint();
		}
	}

	if (ctx->focusChanged && ctx->widget.focusedWidgetId == ctx->currentWidgetId)
	{
		if (ctx->widget.focusedWidgetRect.y > clipRect.bottom())
		{
			scrollPos =
				(ctx->widget.focusedWidgetRect.top() + scrollPos) - clipRect.y;
		}
	}

	if (scrollPos < 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	if (scrollContentSize < clipRect.height && fabs(scrollPos) > 0)
	{
		scrollPos = 0;
		forceRepaint();
	}

	if (ctx->penPosition.y + scrollAmount < clipRect.bottom())
	{
		if (scrollContentSize > clipRect.height)
		{
			scrollPos = scrollContentSize - clipRect.height;
		}
	}

	if (scrollContentSize > clipRect.height)
	{
		auto scrollViewScrollBarElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBar).normalState();
		auto scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).normalState();

		Rect rectScrollBar =
		{
			clipRect.right() - scrollViewScrollBarElemState.width * ctx->globalScale,
			clipRect.y,
			scrollViewScrollBarElemState.width * ctx->globalScale,
			clipRect.height
		};

		f32 handleSize = rectScrollBar.height * rectScrollBar.height / scrollContentSize;

		if (handleSize < ctx->settings.minScrollViewHandleSize)
			handleSize = ctx->settings.minScrollViewHandleSize;

		f32 handleOffset = (rectScrollBar.height - handleSize) * scrollPos / (scrollContentSize - clipRect.height);

		Rect rectScrollBarHandle =
		{
			clipRect.right() - scrollViewScrollThumbElemState.width * ctx->globalScale,
			clipRect.y + handleOffset,
			scrollViewScrollThumbElemState.width * ctx->globalScale,
			handleSize
		};

		if (ctx->isActiveLayer())
		if (rectScrollBarHandle.contains(ctx->event.mouse.point) || (scrollViewInfo.draggingThumb && ctx->dragScrollViewHandleWidgetId == scrollViewInfo.widgetId))
		{
			scrollViewScrollThumbElemState = ctx->theme->getElement(WidgetElementId::ScrollViewScrollThumb).getState(WidgetStateType::Hovered);
		}

		if (ctx->event.type == InputEvent::Type::MouseDown && ctx->isActiveLayer())
		{
			if (rectScrollBarHandle.contains(ctx->event.mouse.point))
			{
				scrollViewInfo.draggingThumb = true;
				scrollViewInfo.dragDelta = ctx->event.mouse.point - rectScrollBarHandle.topLeft();
				ctx->dragScrollViewHandleWidgetId = scrollViewInfo.widgetId;
				ctx->widget.focusedWidgetId = ctx->currentWidgetId;
			}
			else if (rectScrollBar.contains(ctx->event.mouse.point))
			{
				f32 pageSize = (clipRect.height * ctx->scrollViewScrollPageSize);

				// page up
				if (ctx->event.mouse.point.y < rectScrollBarHandle.y)
				{
					scrollPos -= pageSize;
				}
				// page down
				else if (ctx->event.mouse.point.y > rectScrollBarHandle.bottom())
				{
					scrollPos += pageSize;
				}
			}
		}
		else if (ctx->mouseMoved
			&& scrollViewInfo.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.widgetId)
		{
			f32 crtLocalY = ctx->event.mouse.point.y - scrollViewInfo.dragDelta.y - clipRect.y;
			f32 trackSize = clipRect.height - handleSize;
			f32 percent = crtLocalY / trackSize;
			f32 oldScrollPos = scrollPos;

			// kill event, only we're dragging now
			ctx->event.type = InputEvent::Type::None;
			scrollPos = percent * (scrollContentSize - clipRect.height);
			scrollAmount = oldScrollPos - scrollPos;

			//TODO: duplicated code see above scrollPos corRecton
			if (scrollPos < 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (scrollContentSize < clipRect.height && fabs(scrollPos) > 0)
			{
				scrollPos = 0;
				forceRepaint();
			}

			if (ctx->penPosition.y + scrollAmount < clipRect.bottom())
			{
				if (scrollContentSize > clipRect.height)
				{
					scrollPos = scrollContentSize - clipRect.height;
				}
			}
			// end duplicated code

			handleOffset = (rectScrollBar.height - handleSize) * scrollPos / (scrollContentSize - clipRect.height);

			rectScrollBarHandle =
			{
				clipRect.right() - scrollViewScrollThumbElemState.width * ctx->globalScale,
				clipRect.y + handleOffset,
				scrollViewScrollThumbElemState.width * ctx->globalScale,
				handleSize
			};
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& ctx->isActiveLayer()
			&& scrollViewInfo.draggingThumb
			&& ctx->dragScrollViewHandleWidgetId == scrollViewInfo.widgetId)
		{
			scrollViewInfo.draggingThumb = false;
			ctx->dragScrollViewHandleWidgetId = 0;
		}

		// draw scroll bar line
		ctx->renderer->cmdSetColor(scrollViewElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollBarElemState.image, scrollViewScrollBarElemState.border, rectScrollBar, ctx->globalScale);

		// draw scroll bar thumb
		ctx->renderer->cmdSetColor(scrollViewScrollThumbElemState.color);
		ctx->renderer->cmdDrawImageBordered(scrollViewScrollThumbElemState.image, scrollViewScrollThumbElemState.border, rectScrollBarHandle, ctx->globalScale);
	}

	scrollPos = (u32)scrollPos;
	ctx->penPosition = ctx->penStack.back();
	ctx->penStack.pop_back();
	addWidgetItem(size);
	ctx->currentWidgetId++;

	return scrollPos;
}

void beginVirtualListContent(u32 totalRowCount, u32 itemHeight, f32 scrollPos)
{
	f32 skipRows = scrollPos / itemHeight;
	auto penPos = hui::getPenPosition();
	hui::setPenPosition({ penPos.x, penPos.y + (int)skipRows * itemHeight });
	ctx->virtualListStack.push_back(VirtualListContentState());
	ctx->virtualListStack.back().totalRowCount = totalRowCount;
	ctx->virtualListStack.back().itemHeight = itemHeight;
	ctx->virtualListStack.back().lastPenPosition = penPos;
}

void endVirtualListContent()
{
	hui::setPenPosition(
		{
			ctx->virtualListStack.back().lastPenPosition.x,
			ctx->virtualListStack.back().lastPenPosition.y + ctx->virtualListStack.back().totalRowCount * ctx->virtualListStack.back().itemHeight
		});

	ctx->virtualListStack.pop_back();
}

}