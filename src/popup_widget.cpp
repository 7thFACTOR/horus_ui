#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_font.h"
#include "util.h"
#include "ui_context.h"
#include <limits.h>

namespace hui
{
//TODO: place into public settings
static f32 movePopupMaxDistanceTrigger = 5;

void beginPopup(
	f32 width,
	PopupPositionMode positionMode,
	const Point& customPosition,
	WidgetElementId widgetElementIdTheme,
	bool incrementLayer,
	bool topMost,
	bool isMenu)
{
	auto& popup = ctx->popupStack[ctx->popupIndex];

	popup.incrementLayer = incrementLayer;

	if (ctx->popupUseGlobalScale)
		width *= ctx->globalScale;

	ctx->popupUseGlobalScale = true;

	if (incrementLayer)
		incrementLayerIndex();

	if (topMost)
	{
		popup.topMost = topMost;
		popup.oldZOrder = ctx->renderer->getZOrder();
		ctx->renderer->setZOrder(INT_MAX - 1);
	}

	ctx->popupIndex++;

	// not active, first show, do not render anything, next frame
	if (!popup.active)
	{
		skipThisFrame();
		popup.active = true;
		popup.height = 0;
		popup.widgetElementId = widgetElementIdTheme;
	}

	if (positionMode == PopupPositionMode::BelowLastWidget)
	{
		popup.position = ctx->widget.rect.bottomLeft();
	}
	else
	{
		popup.position = customPosition;
	}

	popup.width = width;
	popup.positionMode = positionMode;

	f32 height = popup.height;
	auto bodyElemState = ctx->theme->getElement(widgetElementIdTheme).normalState();
	Point pos = { ctx->containerRect.x, ctx->containerRect.y };

	switch (positionMode)
	{
	case hui::PopupPositionMode::WindowCenter:
		pos = {
			(ctx->renderer->getWindowSize().x - width) / 2.0f,
			(ctx->renderer->getWindowSize().y - height) / 2.0f };
		pos += popup.moveOffset;
		break;
	case hui::PopupPositionMode::Custom:
	case hui::PopupPositionMode::BelowLastWidget:
		pos = { popup.position.x, popup.position.y };
		break;
	default:
		break;
	}

	if (isMenu && !ctx->rightSideMenu)
	{
		pos.x -= ctx->activeMenuBarItemWidgetWidth + width;
	}

	// limit to right side
	if (pos.x + width > ctx->renderer->getWindowSize().x)
	{
		if (isMenu)
		{
			ctx->rightSideMenu = false;
			pos.x -= ctx->activeMenuBarItemWidgetWidth + width;
		}
		else
		{
			pos.x = ctx->renderer->getWindowSize().x - width;
		}
	}

	// limit to top
	if (pos.y + height > ctx->renderer->getWindowSize().y)
	{
		pos.y = ctx->renderer->getWindowSize().y - height;
	}

	// limit to left
	if (pos.x < 0)
	{
		pos.x = 0;
	}

	// limit to bottom
	if (pos.y < 0)
	{
		pos.y = 0;
	}

	pos.x = round(pos.x);
	pos.y = round(pos.y);

	popup.position = pos;
	Rect popupRect = { pos.x, pos.y, width, height };

	ctx->layoutStack.push_back(LayoutState(LayoutType::Container));
	ctx->layoutStack.back().position =
	{
		pos.x + bodyElemState.border * ctx->globalScale,
		pos.y + bodyElemState.border * ctx->globalScale
	};
	ctx->layoutStack.back().width = width - bodyElemState.border * 2 * ctx->globalScale;
	ctx->layoutStack.back().savedPenPosition = ctx->penPosition;
	ctx->penPosition = ctx->layoutStack.back().position;
	ctx->sameLineStack.push_back(ctx->widget.sameLine);
	ctx->widget.sameLine = false; // reset the same line, we dont need that at the popup start
	popup.prevContainerRect = ctx->containerRect;
	ctx->containerRect = ctx->renderer->getWindowRect();

	ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState.image,
		bodyElemState.border,
		popupRect, ctx->globalScale);

	popup.widgetId = ctx->currentWidgetId;
	ctx->currentWidgetId++;
}

void endPopup()
{
	auto& popup = ctx->popupStack[ctx->popupIndex - 1];

	//TODO: make a better popup move
	if (ctx->isActiveLayer()
		&&
		(ctx->widget.hoveredWidgetId == popup.widgetId
			|| popup.startedToDrag
			|| ctx->widget.hoveredWidgetType == WidgetType::Label))
	{
		auto rect = Rect(popup.position.x, popup.position.y, popup.width, popup.height);

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& rect.contains(ctx->event.mouse.point)
			&& !popup.startedToDrag)
		{
			popup.startedToDrag = true;
			popup.lastMouseDownPoint = ctx->event.mouse.point;
			popup.lastMousePoint = ctx->event.mouse.point;
		}

		// popup drag by mouse
		if (popup.startedToDrag || popup.draggingPopup)
		{
			auto mousePos = ctx->inputProvider->getMousePosition();

			if (popup.startedToDrag
				&& popup.lastMouseDownPoint.getDistance(mousePos) >= movePopupMaxDistanceTrigger
				&& !popup.draggingPopup
				&& ctx->widget.hoveredWidgetId == popup.widgetId)
			{
				popup.dragDelta = popup.position - mousePos;
				// clear the event so other widgets will not use it
				ctx->event = InputEvent();
				popup.draggingPopup = true;
				popup.startedToDrag = false;
			}
			else if (popup.draggingPopup)
			{
				popup.position = mousePos + popup.dragDelta;
				popup.moveOffset += mousePos - popup.lastMousePoint;
				popup.lastMousePoint = mousePos;
			}
		}
	}

	if (ctx->event.type == InputEvent::Type::MouseUp)
	{
		if (popup.draggingPopup)
		{
			popup.moveOffset += ctx->event.mouse.point - popup.lastMousePoint;
			popup.lastMousePoint = ctx->event.mouse.point;
			// clear the event so other widgets will not use it
			ctx->event = InputEvent();
		}

		popup.startedToDrag = false;
		popup.draggingPopup = false;
		forceRepaint();
	}

	auto bodyElemState = ctx->theme->getElement(popup.widgetElementId).normalState();
	popup.height = (ctx->penPosition.y - ctx->layoutStack.back().position.y) + bodyElemState.border * 2.0f * ctx->globalScale - ctx->spacing * ctx->globalScale;
	
	ctx->penPosition = ctx->layoutStack.back().savedPenPosition;
	ctx->containerRect = popup.prevContainerRect;
	ctx->renderer->popClipRect();
	ctx->layoutStack.pop_back();
	ctx->widget.sameLine = ctx->sameLineStack.back();
	ctx->sameLineStack.pop_back();

	if (popup.incrementLayer)
		decrementLayerIndex();

	if (popup.topMost)
		ctx->renderer->setZOrder(popup.oldZOrder);

	ctx->popupIndex--;
}

void closePopup()
{
	auto& popup = ctx->popupStack[ctx->popupIndex - 1];

	popup.active = false;

	if (popup.incrementLayer)
		decrementWindowMaxLayerIndex();

	ctx->event.type = InputEvent::Type::None;
	ctx->widget.focusedWidgetId = 0;
	skipThisFrame();
	forceRepaint();
}

bool clickedOutsidePopup()
{
	if (ctx->event.type != InputEvent::Type::MouseDown)
		return false;

	if (ctx->layoutStack.back().type == LayoutType::Container
		&& ctx->isActiveLayer())
	{
		auto& popup = ctx->popupStack[ctx->popupIndex - 1];

		Rect rc = {
			popup.position.x,
			popup.position.y,
			popup.width,
			popup.height };

		if (!rc.contains(ctx->event.mouse.point))
		{
			return true;
		}
	}

	return false;
}

bool mouseOutsidePopup()
{
	if (ctx->layoutStack.back().type == LayoutType::Container
		&& ctx->isActiveLayer())
	{
		auto& popup = ctx->popupStack[ctx->popupIndex - 1];

		Rect rc = {
			popup.position.x,
			popup.position.y,
			popup.width,
			popup.height };

		if (!rc.contains(ctx->event.mouse.point))
		{
			return true;
		}
	}

	return false;
}

bool pressedEscapeOnPopup()
{
	auto& popup = ctx->popupStack[ctx->popupIndex - 1];

	if (popup.alreadyClosedWithEscape)
		return false;

	if (ctx->layoutStack.back().type == LayoutType::Container
		&& ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.down
		&& ctx->event.key.code == KeyCode::Esc
		&& ctx->isActiveLayer())
	{
		popup.alreadyClosedWithEscape = true;
		return true;
	}

	return false;
}

bool mustClosePopup()
{
	return pressedEscapeOnPopup() || clickedOutsidePopup();
}

MessageBoxButtons messageBox(
	const char* title,
	const char* message,
	MessageBoxButtons buttons,
	MessageBoxIcon icon,
	u32 width,
	Image customIcon)
{
	UiThemeElement* iconElem = nullptr;

	switch (icon)
	{
	case hui::MessageBoxIcon::Error:
		iconElem = &ctx->theme->getElement(WidgetElementId::MessageBoxIconError);
		break;
	case hui::MessageBoxIcon::Info:
		iconElem = &ctx->theme->getElement(WidgetElementId::MessageBoxIconInfo);
		break;
	case hui::MessageBoxIcon::Question:
		iconElem = &ctx->theme->getElement(WidgetElementId::MessageBoxIconQuestion);
		break;
	case hui::MessageBoxIcon::Warning:
		iconElem = &ctx->theme->getElement(WidgetElementId::MessageBoxIconWarning);
		break;
	default:
		break;
	}

	hui::beginPopup(500, PopupPositionMode::WindowCenter);
	auto iterFnt = ctx->theme->fonts.find("title");
	hui::pushTint(Color::cyan);

	if (iterFnt != ctx->theme->fonts.end())
	{
		hui::labelCustomFont(title, (Image)iterFnt->second);
	}
	else
	{
		hui::label(title);
	}

	hui::popTint();
	hui::line();

	// body and icon
	f32 titleColWidths[2] = { 0.8, 0.2 };
	beginColumns(2, titleColWidths);
	hui::multilineLabel(message, HAlignType::Left);
	nextColumn();
	hui::image((Image)iconElem->normalState().image, 0, hui::HAlignType::Right);
	endColumns();

	hui::gap(10);

	MessageBoxButtons returnBtns = MessageBoxButtons::None;

	u32 colCount = 0;
	f32 colWidths[6] = { -1,-1,-1,-1,-1,-1 };

	if (!!(buttons & MessageBoxButtons::Ok)) colCount++;
	if (!!(buttons & MessageBoxButtons::Cancel)) colCount++;
	if (!!(buttons & MessageBoxButtons::Yes)) colCount++;
	if (!!(buttons & MessageBoxButtons::No)) colCount++;
	if (!!(buttons & MessageBoxButtons::Retry)) colCount++;
	if (!!(buttons & MessageBoxButtons::Abort)) colCount++;

	hui::beginColumns(colCount, colWidths);

	if (!!(buttons & MessageBoxButtons::Ok))
	{
		if (hui::button("OK"))
		{
			returnBtns |= MessageBoxButtons::Ok;
		}

		hui::nextColumn();
	}

	if (!!(buttons & MessageBoxButtons::Cancel))
	{
		if (hui::button("Cancel"))
		{
			returnBtns |= MessageBoxButtons::Cancel;
		}

		hui::nextColumn();
	}

	if (!!(buttons & MessageBoxButtons::Yes))
	{
		if (hui::button("Yes"))
		{
			returnBtns |= buttons & MessageBoxButtons::Yes;
		}

		hui::nextColumn();
	}

	if (!!(buttons & MessageBoxButtons::No))
	{
		if (hui::button("No"))
		{
			returnBtns |= MessageBoxButtons::No;
		}

		hui::nextColumn();
	}

	if (!!(buttons & MessageBoxButtons::Retry))
	{
		if (hui::button("Retry"))
		{
			returnBtns |= MessageBoxButtons::Retry;
		}

		hui::nextColumn();
	}

	if (!!(buttons & MessageBoxButtons::Abort))
	{
		if (hui::button("Abort"))
		{
			returnBtns |= MessageBoxButtons::Abort;
		}

		hui::nextColumn();
	}

	hui::endColumns();

	if (mustClosePopup())
	{
		returnBtns = MessageBoxButtons::Abort
			| MessageBoxButtons::Cancel
			| MessageBoxButtons::ClosedByEscape
			| MessageBoxButtons::No;
		closePopup();
	}
	else if (!!returnBtns)
	{
		closePopup();
	}

	hui::endPopup();

	return returnBtns;
}

}
