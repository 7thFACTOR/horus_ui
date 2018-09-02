#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_font.h"
#include "ui_context.h"
#include "text_cache.h"
#include "util.h"
#include <math.h>

namespace hui
{
bool list(i32* selectedIndices, u32 maxSelectedIndices, ListSelectionMode selectionType, const char** items, u32 itemCount)
{
	return 0;
}

bool list(i32* selectedIndices, u32 maxSelectedIndices, ListSelectionMode selectionType, void* userdata,
	bool(*itemSource)(void* userdata, i32 index, char** outItemText))
{

	return 0;
}

bool beginList(ListSelectionMode selectionType)
{

	return 0;
}

void endList()
{
}

void listItem(const char* labelText, SelectableFlags stateFlags, Image icon)
{

}

bool selectableInternal(const char* labelText, Font font, SelectableFlags stateFlags)
{
	auto bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	UiFont* fnt = font ? (UiFont*)font : bodyElem.normalState().font;

	addWidgetItem(fmaxf(
		bodyElem.normalState().height,
		fnt->getMetrics().height) * ctx->globalScale);
	buttonBehavior();

	auto bodyElemState = &bodyElem.normalState();

	if (ctx->widget.pressed || ((u32)stateFlags & (u32)SelectableFlags::Selected))
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused)
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer->pushClipRect(ctx->widget.rect);
		ctx->renderer->cmdSetColor(bodyElemState->color * ctx->tint[(int)TintColorType::Body]);
		ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->globalScale);
		ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(fnt);
		ctx->renderer->cmdDrawTextInBox(
			labelText,
			Rect(
				ctx->widget.rect.x + bodyElemState->border,
				ctx->widget.rect.y + bodyElemState->border,
				ctx->widget.rect.width - bodyElemState->border * 2,
				ctx->widget.rect.height - bodyElemState->border * 2)
			,
			HAlignType::Left,
			VAlignType::Center);
		ctx->renderer->popClipRect();
	}

	setAsFocusable();
	ctx->currentWidgetId++;
	ctx->menuItemTextWidth = fnt->computeTextSize(labelText).width + bodyElemState->border * 2.0f;

	return ctx->widget.clicked;
}

bool selectable(const char* labelText, SelectableFlags stateFlags)
{
	return selectableInternal(labelText, nullptr, stateFlags);
}

bool selectableCustomFont(const char* labelText, Font font, SelectableFlags stateFlags)
{
	return selectableInternal(labelText, font, stateFlags);
}

}
