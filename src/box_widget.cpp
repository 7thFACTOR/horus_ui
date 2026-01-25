#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{

struct BoxState
{
	f32 width = 0.0f;
};

std::unordered_map<WidgetId, DrawCmdLayerSplitter> boxDrawCmdSplitter;
std::unordered_map<WidgetId, BoxState> boxState;

static void beginBoxInternal(const char* id, const Color& color, ThemeElement::State& state, f32 customHeight)
{
	const auto parentWidth = ctx->layout.width;
	auto& padding = getPadding(PaddingType::Layout);

	pushLayout();
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.id = ctx->id = genId(id);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.savedPadding = padding;
	ctx->layout.width = parentWidth - (state.border * 2.0f) * ctx->scale - padding.x * 2.0f;
	ctx->layout.themeWidgetElementState = &state;
	ctx->layout.themeElementColorTint = color;
	ctx->layout.firstWidgetInLayout = true;
	boxState[ctx->id].width = parentWidth;

	ctx->position.x += state.border * ctx->scale + padding.x;

	if (customHeight <= 0.0f)
	{
		ctx->position.y += state.border * ctx->scale + padding.y;
	}
	else
	{
		ctx->layout.height = customHeight * ctx->scale;
	}

	boxDrawCmdSplitter[ctx->id].clear();
	boxDrawCmdSplitter[ctx->id].split(2);
	boxDrawCmdSplitter[ctx->id].setLayer(1);
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

	if (ctx->wasSameLine)
	{
		ctx->position.y += ctx->sameLineHeight;
		ctx->wasSameLine = false;
	}

	auto contentHeight = ctx->position.y - ctx->layout.savedPosition.y;
	contentHeight -= boxElemState->border * ctx->scale + ctx->layout.savedPadding.y;
	auto height = contentHeight + boxElemState->border * 2.0f * ctx->scale + ctx->layout.savedPadding.y * 2.0f;

	if (ctx->layout.height > 0.0f)
	{
		//height = ctx->layout.height * ctx->scale;
	}

	ctx->id = ctx->layout.id;

	ctx->widget.rect = {
		ctx->layout.savedPosition.x,
		ctx->layout.savedPosition.y,
		boxState[ctx->id].width,
		height
	};

	buttonBehavior();

	boxDrawCmdSplitter[ctx->id].setLayer(0);
	ctx->renderer->cmdSetColor(boxElemState->color * ctx->layout.themeElementColorTint);
	ctx->renderer->cmdSetAtlas(ctx->theme->atlas);
	ctx->renderer->cmdDrawImageBordered(
		boxElemState->image,
		boxElemState->border,
		ctx->widget.rect,
		ctx->scale);

	ctx->position.x = ctx->layout.savedPosition.x;
	ctx->position.y = ctx->layout.savedPosition.y;

	if (ctx->layout.height <= 0.0f)
	{
		ctx->position.y += boxElemState->border * ctx->scale + ctx->layout.savedPadding.y;
	}
	
	ctx->position.y += height;
	boxDrawCmdSplitter[ctx->id].merge();
	popLayout();

	return ctx->widget.pressed;
}

}