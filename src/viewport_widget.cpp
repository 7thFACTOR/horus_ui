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

	//ctx->gfx->setViewport(ctx->renderer->getWindowSize(), ctx->widget.rect);
	ctx->currentWidgetId++;

	return scissor;
}

void endViewport()
{
	ctx->renderer->popClipRect();
	auto wnd = ctx->inputProvider->getCurrentWindow();
	auto rc = ctx->inputProvider->getWindowRect(wnd);
	rc.x = rc.y = 0;
	//ctx->gfx->setViewport({ rc.width, rc.height }, rc);
	//TODO: restore any render targets we use, the shader is set anyway when drawing the things
}

}