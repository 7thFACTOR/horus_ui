#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"
#include <limits.h>

namespace hui
{
void beginPopup(
	const char* id,
	f32 width,
	PopupFlags flags,
	const Point& position,
	WidgetElementId widgetElementId)
{
	ctx->id = genIdFromPosition(id);
	auto& popup = ctx->popupStack[ctx->popupIndex];

	popup.flags = flags;

	if (ctx->popupUseGlobalScale)
		width *= ctx->scale;

	if (!has(flags, PopupFlags::SameLayer))
		incrementLayerIndex();

	if (has(flags, PopupFlags::TopMost))
	{
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
		popup.widgetElementId = widgetElementId;
	}

	if (has(flags, PopupFlags::BelowLastWidget))
	{
		popup.position = ctx->widget.rect.bottomLeft();
	}
	else if (has(flags, PopupFlags::RightSideLastWidget))
	{
		popup.position = ctx->widget.rect.topRight();
	}
	else
	{
		popup.position = position;
	}

	popup.width = width;

	f32 height = popup.height;
	auto& bodyElemState = ctx->theme->getElement(widgetElementId).normalState();
	Point pos;

	if (has(flags, PopupFlags::Centered))
	{
		pos = {
			(ctx->renderer->getWindowSize().x - width) / 2.0f,
			(ctx->renderer->getWindowSize().y - height) / 2.0f 
		};
		pos += popup.moveOffset;
	}
	else if (has(flags, PopupFlags::CustomPosition)
		|| has(flags, PopupFlags::BelowLastWidget)
		|| has(flags, PopupFlags::RightSideLastWidget))
	{
		pos = { popup.position.x, popup.position.y };
	}

	if (has(flags, PopupFlags::IsMenu) && !ctx->rightSideMenu)
	{
		pos.x -= ctx->activeMenuBarItemWidgetWidth + width;
	}

	// limit to right side
	if (pos.x + width > ctx->renderer->getWindowSize().x)
	{
		if (has(flags, PopupFlags::IsMenu))
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

	pushLayout();
	pushPosition();

	ctx->layout = LayoutState(LayoutType::Generic);
	ctx->position =
	{
		pos.x + bodyElemState.border * ctx->scale,
		pos.y + bodyElemState.border * ctx->scale
	};
	ctx->layout.width = width - bodyElemState.border * 2 * ctx->scale;
	ctx->layout.savedPosition = ctx->position;
	ctx->sameLineStack.push_back(ctx->sameLine);
	ctx->sameLine = false; // reset the same line, we don't need that at the popup start

	ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);

	if (has(flags, PopupFlags::FadeBackground))
	{
		auto& behindElemState = ctx->theme->getElement(WidgetElementId::PopupBehind).normalState();

		ctx->renderer->cmdSetColor(behindElemState.color);
		ctx->renderer->cmdDrawImageBordered(
			behindElemState.image,
			behindElemState.border,
			ctx->renderer->getWindowRect(), ctx->scale);
	}

	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState.image,
		bodyElemState.border,
		popupRect, ctx->scale);

	popup.id = ctx->id;
}

void endPopup()
{
	auto& popup = ctx->popupStack[ctx->popupIndex - 1];

	//TODO: make a better popup move
	if (ctx->isActiveLayer()
		&&
		(ctx->widget.hoveredId == popup.id
			|| popup.startedToDrag
			|| ctx->widget.hoveredType == WidgetType::Label))
	{
		auto rect = Rect(popup.position.x, popup.position.y, popup.width, popup.height);

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& rect.contains(ctx->mousePosition)
			&& !popup.startedToDrag)
		{
			popup.startedToDrag = true;
			popup.lastMouseDownPoint = ctx->mousePosition;
			popup.lastMousePoint = ctx->mousePosition;
		}

		// popup drag by mouse
		if (popup.startedToDrag || popup.draggingPopup)
		{
			auto mousePos = ctx->providers->input->getAbsoluteMousePosition();

			if (popup.startedToDrag
				&& popup.lastMouseDownPoint.getDistance(mousePos) >= ctx->settings.movePopupMaxDistanceTrigger
				&& !popup.draggingPopup
				&& ctx->widget.hoveredId == popup.id)
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
			popup.moveOffset += ctx->mousePosition - popup.lastMousePoint;
			popup.lastMousePoint = ctx->mousePosition;
			// clear the event so other widgets will not use it
			ctx->event = InputEvent();
		}

		popup.startedToDrag = false;
		popup.draggingPopup = false;
		forceRepaint();
	}

	auto& bodyElemState = ctx->theme->getElement(popup.widgetElementId).normalState();
	popup.height = (ctx->position.y - ctx->layout.savedPosition.y) + bodyElemState.border * 2.0f * ctx->scale - ctx->spacing * ctx->scale;
	
	ctx->position = ctx->layout.savedPosition;
	ctx->renderer->popClipRect();
	popPosition();
	popLayout();
	ctx->sameLine = ctx->sameLineStack.back();
	ctx->sameLineStack.pop_back();

	if (has(popup.flags, PopupFlags::TopMost))
		ctx->renderer->setZOrder(popup.oldZOrder);

	if (!has(popup.flags, PopupFlags::SameLayer))
		decrementLayerIndex();

	ctx->popupIndex--;
}

void closePopup()
{
	auto& popup = ctx->popupStack[(size_t)ctx->popupIndex - 1];

	popup.active = false;

	if (!has(popup.flags, PopupFlags::SameLayer))
		decrementWindowMaxLayerIndex();

	ctx->event.type = InputEvent::Type::None;
	ctx->widget.focusedId = 0;
	skipThisFrame();
	forceRepaint();
}

bool clickedOutsidePopup()
{
	if (ctx->event.type != InputEvent::Type::MouseDown)
		return false;

	if (ctx->layout.type == LayoutType::Generic
		&& ctx->isActiveLayer())
	{
		auto& popup = ctx->popupStack[ctx->popupIndex - 1];

		Rect rc = {
			popup.position.x,
			popup.position.y,
			popup.width,
			popup.height };

		if (!rc.contains(ctx->mousePosition))
		{
			return true;
		}
	}

	return false;
}

bool mouseOutsidePopup()
{
	if (ctx->layout.type == LayoutType::Generic
		&& ctx->isActiveLayer())
	{
		auto& popup = ctx->popupStack[ctx->popupIndex - 1];

		Rect rc = {
			popup.position.x,
			popup.position.y,
			popup.width,
			popup.height };

		if (!rc.contains(ctx->mousePosition))
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

	if (ctx->layout.type == LayoutType::Generic
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
	MessageBoxImage img,
	u32 width,
	HImage customImg)
{
	ThemeElement* imageElem = nullptr;

	switch (img)
	{
	case hui::MessageBoxImage::Error:
		imageElem = &ctx->theme->getElement(WidgetElementId::MessageBoxImageError);
		break;
	case hui::MessageBoxImage::Info:
		imageElem = &ctx->theme->getElement(WidgetElementId::MessageBoxImageInfo);
		break;
	case hui::MessageBoxImage::Question:
		imageElem = &ctx->theme->getElement(WidgetElementId::MessageBoxImageQuestion);
		break;
	case hui::MessageBoxImage::Warning:
		imageElem = &ctx->theme->getElement(WidgetElementId::MessageBoxImageWarning);
		break;
	default:
		break;
	}

	hui::beginPopup(title, 500, PopupFlags::FadeBackground | PopupFlags::Centered);
	auto iterFnt = ctx->theme->fonts.find("title");
	hui::pushTint(Color::cyan);

	if (iterFnt != ctx->theme->fonts.end())
	{
		hui::labelCustomFont(title, (HImage)iterFnt->second);
	}
	else
	{
		hui::label(title);
	}

	hui::popTint();
	hui::line();

	// body and image
	f32 titleColWidths[2] = { 0.8, 0.2 };
	beginColumns(2, titleColWidths);
	hui::labelMultiline(message, HAlignType::Left);
	nextColumn();
	hui::image((HImage)imageElem->normalState().image, 0, hui::HAlignType::Right);
	endColumns();

	hui::customSpace(10);

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
