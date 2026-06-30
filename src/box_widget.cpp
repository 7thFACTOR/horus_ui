#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
static void beginBoxLayoutInternal(const char* id, const Color& color, ThemeElement::State* state, f32 customHeight)
{
	const auto parentWidth = ctx->layout.width;
	const auto& padding = paddingGet(PaddingType::Layout);

	layoutPush();
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.id = ctx->id = genId(id);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.width = parentWidth - (state->border + padding.x) * ctx->scale * 2.0f;
	
	auto& boxState = ctx->boxState[ctx->id];

	boxState.savedPadding = padding;
	boxState.themeWidgetElementState = state;
	boxState.themeElementColorTint = color;
	boxState.width = parentWidth;
	ctx->position.x += (state->border + padding.x) * ctx->scale;

	boxState.customHeight = customHeight;

	if (customHeight <= 0.0f)
	{
		ctx->position.y += (state->border + padding.y) * ctx->scale;
	}
	else
	{
		ctx->layout.height = customHeight;
	}

	ctx->boxDrawCmdSplitter[ctx->id].clear();
	ctx->boxDrawCmdSplitter[ctx->id].split(2);
	ctx->boxDrawCmdSplitter[ctx->id].setLayer(1);
}

void boxBegin(
	const char* id,
	const Color& tintColor,
	WidgetElementId widgetElementId,
	WidgetStateType state,
	f32 customHeight)
{
	auto& boxElemState = ctx->theme->getElement(widgetElementId).getState(state);

	beginBoxLayoutInternal(id, tintColor, &boxElemState, customHeight);
}

void boxBeginUserElement(
	const char* id,
	const Color& tintColor,
	const char* userElementName,
	WidgetStateType state,
	f32 customHeight)
{
	auto elem = ctx->theme->userElements[userElementName];

	if (!elem)
		elem = &ctx->theme->getElement(WidgetElementId::TextInputBody);

	auto& boxElemState = elem->getState(state);
	beginBoxLayoutInternal(id, tintColor, &boxElemState, customHeight);
}

bool boxEnd()
{
	ctx->id = ctx->layout.id;
	auto& boxState = ctx->boxState[ctx->id];

	if (!boxState.themeWidgetElementState)
		boxState.themeWidgetElementState = &ctx->theme->getElement(WidgetElementId::BoxBody).normalState();

	auto& boxElemState = boxState.themeWidgetElementState;

	// finish same line
	if (ctx->sameLine.wasEnabled)
	{
		ctx->position.y += ctx->sameLine.maxHeight;
		ctx->sameLine.wasEnabled = false;
	}

	f32 height;

	if (boxState.customHeight > 0.0f)
	{
		height = boxState.customHeight;
	}
	else
	{
		auto contentHeight = ctx->position.y - ctx->layout.savedPosition.y;
		contentHeight -= (boxElemState->border + boxState.savedPadding.y) * ctx->scale;
		height = contentHeight + (boxElemState->border + boxState.savedPadding.y) * ctx->scale * 2.0f;
	}

	ctx->widget.rect = {
		ctx->layout.savedPosition.x,
		ctx->layout.savedPosition.y,
		boxState.width,
		height
	};

	buttonBehavior();
	ctx->boxDrawCmdSplitter[ctx->id].setLayer(0);
	ctx->renderer.cmdSetColor(boxElemState->color * boxState.themeElementColorTint);
	ctx->renderer.cmdDrawImageBordered(
		boxElemState->image,
		boxElemState->border,
		ctx->widget.rect,
		ctx->scale);

	ctx->position.x = ctx->layout.savedPosition.x;
	ctx->position.y = ctx->layout.savedPosition.y;

	if (ctx->layout.height <= 0.0f)
	{
		ctx->position.y += (boxElemState->border + boxState.savedPadding.y ) * ctx->scale;
	}
	
	ctx->position.y += height;
	ctx->boxDrawCmdSplitter[ctx->id].merge();
	layoutPop();

	return ctx->widget.pressed;
}

}