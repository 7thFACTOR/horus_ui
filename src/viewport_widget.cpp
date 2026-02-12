#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
Rect beginViewport(const char* id, f32 height)
{
	if (height <= 0)
	{
		height = ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y);
	}

	ctx->setLabelAndId(id);
	addWidget(height);
	buttonBehavior();

	auto scissor = ctx->renderer.pushClipRect(ctx->widget.rect);

	ctx->renderer.viewportOffset = ctx->widget.rect.topLeft();

	return scissor;
}

void endViewport()
{
	ctx->renderer.popClipRect();
	ctx->renderer.viewportOffset = Point();
}

}