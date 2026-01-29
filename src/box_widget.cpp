#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{

struct BoxState
{
	f32 width = 0.0f;
	ThemeElement::State* themeWidgetElementState = nullptr;
	Color themeElementColorTint;
	Point savedPadding;
};

std::unordered_map<WidgetId, DrawCmdLayerSplitter> boxDrawCmdSplitter;
std::unordered_map<WidgetId, BoxState> boxState;

static void beginBoxLayoutInternal(const char* id, const Color& color, ThemeElement::State& state, f32 customHeight)
{
	const auto parentWidth = ctx->layout.width;
	const auto& padding = getPadding(PaddingType::Layout);

	pushLayout();
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.id = ctx->id = genId(id);
	ctx->layout.savedPosition = ctx->position;
	ctx->layout.width = parentWidth - (state.border + padding.x) * ctx->scale * 2.0f;
	ctx->layout.firstWidgetInLayout = true;
	boxState[ctx->id].savedPadding = padding;
	boxState[ctx->id].themeWidgetElementState = &state;
	boxState[ctx->id].themeElementColorTint = color;
	boxState[ctx->id].width = parentWidth;
	ctx->position.x += (state.border + padding.x) * ctx->scale;

	if (customHeight <= 0.0f)
	{
		ctx->position.y += (state.border + padding.y) * ctx->scale;
	}
	else
	{
		ctx->layout.height = customHeight;
	}

	boxDrawCmdSplitter[ctx->id].clear();
	boxDrawCmdSplitter[ctx->id].split(2);
	boxDrawCmdSplitter[ctx->id].setLayer(1);
}

void beginBoxLayout(
	const char* id,
	const Color& tintColor,
	WidgetElementId widgetElementId,
	WidgetStateType state,
	f32 customHeight)
{
	auto& boxElemState = ctx->theme->getElement(widgetElementId).getState(state);

	beginBoxLayoutInternal(id, tintColor, boxElemState, customHeight);
}

void beginBoxLayoutUserElement(
	const char* id,
	const Color& tintColor,
	const char* userElementName,
	WidgetStateType state,
	f32 customHeight)
{
	auto elem = ctx->theme->userElements[userElementName];

	if (elem)
	{
		auto& boxElemState = elem->getState(state);
		beginBoxLayoutInternal(id, tintColor, boxElemState, customHeight);
	}
}

bool endBoxLayout()
{
	ctx->id = ctx->layout.id;

	auto& boxElemState = boxState[ctx->id].themeWidgetElementState;

	// finish same line
	if (ctx->sameLine.wasEnabled)
	{
		ctx->position.y += ctx->sameLine.maxHeight;
		ctx->sameLine.wasEnabled = false;
	}

	auto contentHeight = ctx->position.y - ctx->layout.savedPosition.y;
	contentHeight -= (boxElemState->border + boxState[ctx->id].savedPadding.y) * ctx->scale;
	auto height = contentHeight + (boxElemState->border + boxState[ctx->id].savedPadding.y) * ctx->scale * 2.0f;

	ctx->widget.rect = {
		ctx->layout.savedPosition.x,
		ctx->layout.savedPosition.y,
		boxState[ctx->id].width,
		height
	};

	buttonBehavior();
	boxDrawCmdSplitter[ctx->id].setLayer(0);
	ctx->renderer->cmdSetColor(boxElemState->color * boxState[ctx->id].themeElementColorTint);
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
		ctx->position.y += (boxElemState->border + boxState[ctx->id].savedPadding.y ) * ctx->scale;
	}
	
	ctx->position.y += height;
	boxDrawCmdSplitter[ctx->id].merge();
	popLayout();

	return ctx->widget.pressed;
}

}