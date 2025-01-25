#include "context.h"
#include "theme.h"
#include "font.h"
#include "unicode_text_cache.h"
#include "util.h"

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

void listItem(const char* label, SelectableFlags stateFlags, HImage icon)
{

}

bool selectableInternal(const char* label, HFont font, SelectableFlags stateFlags)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	Font* fnt = font ? (Font*)font : bodyElem.normalState().font;

	ctx->extractLabelAndId(label);
	addWidget(fmaxf(
		bodyElem.normalState().height,
		fnt->getMetrics().height) * ctx->scale);
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
		ctx->renderer->cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
		ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer->cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(fnt);
		ctx->renderer->cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
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

	setFocusable();
	ctx->menuItemTextWidth = fnt->computeTextSize(ctx->widgetLabel.c_str()).width + bodyElemState->border * 2.0f;

	return ctx->widget.clicked;
}

bool selectable(const char* label, SelectableFlags stateFlags)
{
	return selectableInternal(label, nullptr, stateFlags);
}

bool selectableCustomFont(const char* label, HFont font, SelectableFlags stateFlags)
{
	return selectableInternal(label, font, stateFlags);
}

}
