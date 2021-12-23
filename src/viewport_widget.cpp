#include "horus.h"
#include "types.h"
#include "theme.h"
#include "context.h"
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
	ctx->renderer->viewportOffset = ctx->widget.rect.topLeft();

	return scissor;
}

void endViewport()
{
	ctx->renderer->popClipRect();
	ctx->renderer->viewportOffset = Point();
}

}