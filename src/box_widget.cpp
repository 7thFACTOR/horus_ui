#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
std::unordered_map<WidgetId, DrawCmdLayerSplitter> splitter;

static void beginBoxInternal(const char* id, const Color& color, ThemeElement::State& state, f32 customHeight)
{
	const auto parentWidth = ctx->layout.width;

	pushLayout();
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.id = ctx->id = genId(id);
	ctx->layout.savedPosition = ctx->position;
	auto& padding = getWidgetPadding();
	ctx->layout.savedPadding = padding;
	ctx->position.x += state.border * ctx->scale + padding.x;
	ctx->layout.width = parentWidth - padding.x * 2.0f - (state.border * 2.0f) * ctx->scale;
	ctx->layout.themeWidgetElementState = &state;
	ctx->layout.themeElementColorTint = color;

	if (customHeight <= 0.0f)
	{
		ctx->position.y += state.border * ctx->scale + padding.y;
	}
	else
	{
		ctx->layout.height = customHeight * ctx->scale;
	}

	splitter[ctx->id].clear();
	splitter[ctx->id].split(2);
	splitter[ctx->id].setLayer(1);
}

void beginBox(
	const char* id,
	const Color& color,
	WidgetElementId widgetElementId,
	WidgetStateType state,
	f32 customHeight)
{
	auto& boxElemState = ctx->theme->getElement(widgetElementId).getState(state);

	beginBoxInternal(id, color, boxElemState, customHeight);
}

void beginBox(
	const char* id,
	const Color& color,
	const char* userElementName,
	WidgetStateType state,
	f32 customHeight)
{
	auto elem = ctx->theme->userElements[userElementName];

	if (elem)
	{
		auto& boxElemState = elem->getState(state);
		beginBoxInternal(id, color, boxElemState, customHeight);
	}
}

bool endBox()
{
	auto& boxElemState = ctx->layout.themeWidgetElementState;
	auto contentHeight = ctx->position.y - ctx->layout.savedPosition.y;
	contentHeight -= ctx->spacing * ctx->scale;
	contentHeight -= boxElemState->border * ctx->scale;
	auto height = contentHeight + boxElemState->border * 2.0f * ctx->scale + ctx->layout.savedPadding.y * 2.0f;

	if (ctx->layout.height > 0.0f)
	{
		//height = ctx->layout.height * ctx->scale;
	}

	ctx->widget.rect = {
		ctx->layout.savedPosition.x,
		ctx->layout.savedPosition.y,
		ctx->layout.width + (boxElemState->border * 2.0f) * ctx->scale + ctx->layout.savedPadding.x * 2.0f,
		height
	};

	ctx->id = ctx->layout.id;
	buttonBehavior();

	splitter[ctx->id].setLayer(0);

	// insert box draw commands at previous saved draw cmd index
	//auto cmdIndex = popDrawCommandIndex();
	//beginInsertDrawCommands(cmdIndex);
	ctx->renderer->cmdSetColor(boxElemState->color * ctx->layout.themeElementColorTint);
	ctx->renderer->cmdSetAtlas(ctx->theme->atlas);
	ctx->renderer->cmdDrawImageBordered(
		boxElemState->image,
		boxElemState->border,
		ctx->widget.rect,
		ctx->scale);
	//endInsertDrawCommands();

	ctx->position.x = ctx->layout.savedPosition.x;

	if (ctx->layout.height <= 0.0f)
	{
		ctx->position.y += boxElemState->border * ctx->scale + ctx->layout.savedPadding.y;
	}

	splitter[ctx->id].merge();
	popLayout();

	return ctx->widget.pressed;
}

}