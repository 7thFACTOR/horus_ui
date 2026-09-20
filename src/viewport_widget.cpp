#include "context.h"
#include "theme.h"
#include "util.h"
#include "renderer.h"

namespace hui
{
Rect viewportBegin(const char* id, f32 height)
{
	if (height <= 0)
	{
		height = ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y);
	}

	ctx->setLabelAndId(id);
	addWidget(height);
	buttonBehavior();

	ctx->renderer.pushClipRect(ctx->widget.rect);

	// return the real (unclipped) viewport rect; the pushed clip rect only
	// restricts drawing of content inside the viewport.
	ctx->renderer.viewportOffset = ctx->widget.rect.topLeft();

	return ctx->widget.rect;
}

void viewportEnd()
{
	ctx->renderer.popClipRect();
	ctx->renderer.viewportOffset = Point();
}

}