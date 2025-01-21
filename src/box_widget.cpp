#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
void beginBoxInternal(const Color& color, ThemeElement::State& state, f32 customHeight)
{
	const auto width = ctx->layoutStack.back().width;

	ctx->layoutStack.push_back(LayoutState(LayoutType::Container));
	ctx->layoutStack.back().savedPenPosition = ctx->penPosition;
	ctx->penStack.push_back(ctx->penPosition);
	// move with padding
	ctx->penPosition.x += ctx->layoutPadding * ctx->scale + state.border * ctx->scale;
	ctx->layoutStack.back().position = ctx->penPosition;
	// take some padding and border from width
	ctx->layoutStack.back().width = width - (state.border * 2.0f + ctx->layoutPadding * 2.0f) * ctx->scale;
	ctx->layoutStack.back().height = customHeight * ctx->scale;
	ctx->layoutStack.back().themeWidgetElementState = &state;
	ctx->layoutStack.back().themeElementColorTint = color;

	if (customHeight <= 0.0f)
		ctx->penPosition.y += state.border * ctx->scale;

	pushDrawCommandIndex();
}

void beginBox(
	const Color& color,
	WidgetElementId widgetElementId,
	WidgetStateType state,
	f32 customHeight)
{
	auto& boxElemState = ctx->theme->getElement(widgetElementId).getState(state);

	beginBoxInternal(color, boxElemState, customHeight);
}

void beginBox(
	const Color& color,
	const char* userElementName,
	WidgetStateType state,
	f32 customHeight)
{
	auto elem = ctx->theme->userElements[userElementName];

	if (elem)
	{
		auto& boxElemState = elem->getState(state);
		beginBoxInternal(color, boxElemState, customHeight);
	}
}

bool endBox()
{
	auto& boxElemState = ctx->layoutStack.back().themeWidgetElementState;
	auto contentHeight = ctx->penPosition.y - ctx->layoutStack.back().position.y;
	contentHeight -= ctx->spacing * ctx->scale;
	contentHeight -= boxElemState->border * ctx->scale;
	auto height = contentHeight + boxElemState->border * 2.0f * ctx->scale;

	if (ctx->layoutStack.back().height > 0.0f)
	{
		height = ctx->layoutStack.back().height * ctx->scale;
	}

	ctx->widget.rect = {
		ctx->layoutStack.back().savedPenPosition.x,
		ctx->layoutStack.back().savedPenPosition.y,
		ctx->layoutStack.back().width + (boxElemState->border * 2.0f + ctx->layoutPadding * 2.0f) * ctx->scale,
		height
	};

	buttonBehavior();

	auto cmdIndex = popDrawCommandIndex();
	beginInsertDrawCommands(cmdIndex);
	ctx->renderer->cmdSetColor(boxElemState->color * ctx->layoutStack.back().themeElementColorTint);
	ctx->renderer->cmdSetAtlas(ctx->theme->atlas);
	ctx->renderer->cmdDrawImageBordered(
		boxElemState->image,
		boxElemState->border,
		ctx->widget.rect,
		ctx->scale);
	endInsertDrawCommands();

	ctx->penPosition.x = ctx->penStack.back().x;

	if (ctx->layoutStack.back().height <= 0.0f)
	{
		ctx->penPosition.y += boxElemState->border * ctx->scale;
	}

	ctx->layoutStack.pop_back();
	ctx->penStack.pop_back();

	return ctx->widget.pressed;
}

}