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

	ctx->scrollViewStack[ctx->scrollViewDepth].size = size /* * ctx->globalScale*/; //TODO: scale height with UI scale ?
	ctx->scrollViewStack[ctx->scrollViewDepth].virtualHeight = virtualHeight;
	ctx->scrollViewStack[ctx->scrollViewDepth].widgetId = ctx->currentWidgetId;
	ctx->penStack.push_back(ctx->penPosition);
	//TODO: scale height with UI scale ?
	//size *= ctx->globalScale;
	const f32 scrollViewPadding = 10;
	auto internalPadding = scrollViewPadding * ctx->globalScale + (f32)scrollViewElemState.border * ctx->globalScale;

	Rect rect =
	{
		round(ctx->penPosition.x),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width,
		size
	};

	ctx->scrollViewStack[ctx->scrollViewDepth].rect = rect;

	Rect clipRect = rect.contract(internalPadding);

	clipRect.width -= scrollViewScrollThumbElemState.width;

	ctx->renderer->cmdSetColor(scrollViewElemState.color);
	ctx->renderer->cmdDrawImageBordered(scrollViewElemState.image, scrollViewElemState.border, rect, ctx->globalScale);

	ctx->scrollViewStack[ctx->scrollViewDepth].scrollPosition = scrollPos;
	ctx->renderer->pushClipRect(clipRect);
	ctx->penPosition.y -= scrollPos;
	ctx->penPosition.y += scrollViewElemState.border * ctx->globalScale;

	ctx->layoutStack.push_back(LayoutState(LayoutType::ScrollView));
	ctx->layoutStack.back().position = { clipRect.x, clipRect.y };
	ctx->layoutStack.back().width = clipRect.width;
	ctx->layoutStack.back().height = clipRect.height;
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
	auto fullRect = scrollViewInfo.rect;
	auto scrollViewElemState = ctx->theme->getElement(WidgetElementId::ScrollViewBody).normalState();
	f32 scrollPos = scrollViewInfo.scrollPosition;
	f32 size = scrollViewInfo.size;
	f32 contentY = ctx->penStack.back().y - scrollPos;
	f32 scrollContentSize = ctx->penPosition.y - contentY;
	f32 scrollAmount = 0;


	// make the rect for the scrollbars, without the UI element border
	auto rect = fullRect.contract(scrollViewElemState.border);

	if (ctx->event.type == InputEvent::Type::MouseWheel
		&& ctx->isActiveLayer())
	{
		if (fullRect.contains(ctx->event.mouse.point))
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
			scrollPos = (ctx->widget.focusedWidgetRect.y + scrollPos) - clipRect.y;
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
			rect.right() - scrollViewScrollBarElemState.width * ctx->globalScale,
			rect.y,
			scrollViewScrollBarElemState.width * ctx->globalScale,
			rect.height
		};

		f32 handleSize = rectScrollBar.height * rectScrollBar.height / scrollContentSize;

		if (handleSize < ctx->settings.minScrollViewHandleSize)
			handleSize = ctx->settings.minScrollViewHandleSize;

		f32 handleOffset = (rectScrollBar.height - handleSize) * scrollPos / (scrollContentSize - rect.height);

		Rect rectScrollBarHandle =
		{
			rect.right() - scrollViewScrollThumbElemState.width * ctx->globalScale,
			rect.y + handleOffset,
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
				f32 pageSize = (rect.height * ctx->scrollViewScrollPageSize);

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
			f32 crtLocalY = ctx->event.mouse.point.y - scrollViewInfo.dragDelta.y - rect.y;
			f32 trackSize = rect.height - handleSize;
			f32 percent = crtLocalY / trackSize;
			f32 oldScrollPos = scrollPos;

			// kill event, only we're dragging now
			ctx->event.type = InputEvent::Type::None;
			scrollPos = percent * (scrollContentSize - clipRect.height);
			scrollAmount = oldScrollPos - scrollPos;

			//TODO: duplicated code see above scrollPos correction
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
				rect.right() - scrollViewScrollThumbElemState.width * ctx->globalScale,
				rect.y + handleOffset,
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