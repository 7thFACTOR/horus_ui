#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
void line()
{
	auto bodyElemState = ctx->theme->getElement(WidgetElementId::LineBody).normalState();

	addWidgetItem(bodyElemState.image->rect.height * ctx->globalScale);
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState.image, bodyElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height }, ctx->globalScale);
	ctx->currentWidgetId++;
}

void gap(f32 size)
{
	ctx->penPosition.y += size * ctx->globalScale;
}

void space()
{
	ctx->penPosition.y += ctx->spacing * ctx->globalScale;
}

void sameLine()
{
	ctx->sameLine = true;
}

void pushWidth(f32 width)
{
	ctx->widthStack.push_back(ctx->widget.width);
	ctx->widget.width = width;
}

f32 popWidth()
{
	if (ctx->widthStack.size())
	{
		ctx->widget.width = ctx->widthStack.back();
		ctx->widthStack.pop_back();
		return ctx->widget.width;
	}

	return 0;
}

}