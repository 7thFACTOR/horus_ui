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

	// the returned rect is in window coordinates; draw content using it directly,
	// the pushed clip rect only restricts drawing of content inside the viewport.
	return ctx->widget.rect;
}

void viewportEnd()
{
	ctx->renderer.popClipRect();
}

}