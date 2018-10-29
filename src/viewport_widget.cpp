#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
Rect beginViewport(f32 height)
{
	if (height <= 0)
	{
		height = ctx->layoutStack.back().height - (ctx->penPosition.y - ctx->layoutStack.back().position.y);
	}

	addWidgetItem(height);
	buttonBehavior();

	auto scissor = ctx->renderer->pushClipRect(ctx->widget.rect);

	ctx->currentWidgetId++;

	return scissor;
}

void endViewport()
{
	ctx->renderer->popClipRect();
}

}