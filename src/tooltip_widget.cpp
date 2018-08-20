#include "horus.h"
#include "types.h"
#include "ui_atlas.h"
#include "ui_theme.h"
#include "renderer.h"
#include "text_cache.h"
#include "ui_font.h"
#include "ui_context.h"
#include "util.h"
#include <limits.h>

namespace hui
{
bool tooltip(Utf8String text)
{
	if ((ctx->currentWidgetId - 1) == ctx->widget.hoveredWidgetId
		&& ctx->tooltip.show)
	{
		auto bodyElemState = ctx->theme->getElement(WidgetElementId::TooltipBody).normalState();

		FontTextSize fsize = bodyElemState.font->computeTextSize(text);
		Rect rect = {
			ctx->tooltip.position.x + ctx->tooltip.offsetFromCursor + bodyElemState.border,
			ctx->tooltip.position.y + ctx->tooltip.offsetFromCursor + bodyElemState.border,
			fsize.width + bodyElemState.border * 2.0f,	0 };

		if (rect.right() > ctx->renderer->getWindowRect().right())
		{
			rect.x = ctx->renderer->getWindowRect().right() - rect.width;
		}

		if (rect.bottom() > ctx->renderer->getWindowRect().bottom())
		{
			rect.y = ctx->renderer->getWindowRect().bottom() - rect.height;
		}

		if (rect.x < 0)
		{
			rect.x = 0;
		}

		if (rect.y < 0)
		{
			rect.y = 0;
		}

		u32 oldZ = ctx->renderer->getZOrder();
		ctx->renderer->setZOrder(INT_MAX);
		ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
		ctx->renderer->cmdSetColor(bodyElemState.textColor);
		ctx->renderer->cmdSetFont(bodyElemState.font);
		Rect rc = ctx->drawMultilineText(
			text, rect,
			HAlignType::Left, VAlignType::Center);
		// move back a layer
		ctx->renderer->setZOrder(INT_MAX - 1);
		ctx->renderer->cmdSetColor(bodyElemState.color);
		rc.x -= bodyElemState.border;
		rc.y -= bodyElemState.border;
		rc.height += bodyElemState.border * 2;
		ctx->renderer->cmdDrawImageBordered(
			bodyElemState.image, bodyElemState.border, rc, ctx->globalScale);
		ctx->renderer->popClipRect();
		ctx->renderer->setZOrder(oldZ);

		return true;
	}

	return false;
}

bool beginRichTooltip(f32 width)
{
	if ((ctx->currentWidgetId - 1) == ctx->widget.hoveredWidgetId
		&& ctx->tooltip.show)
	{
		auto bodyElemState = ctx->theme->getElement(WidgetElementId::TooltipBody).normalState();

		beginPopup(
			width,
			PopupPositionMode::Custom,
			{ ctx->tooltip.position.x + ctx->tooltip.offsetFromCursor, ctx->tooltip.position.y + ctx->tooltip.offsetFromCursor },
			WidgetElementId::TooltipBody,
			false, true);
		return true;
	}

	return false;
}

void endRichTooltip()
{
	if (ctx->tooltip.show)
	{
		endPopup();
	}
}

}
