#include <limits.h>
#include "context.h"
#include "atlas.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool tooltip(const char* text)
{
	if (ctx->id != ctx->tooltip.id && ctx->id == ctx->widget.hoveredId && !ctx->tooltip.show)
	{
		ctx->tooltip.id = ctx->id;
		ctx->tooltip.timer = 0;
	}

	if (ctx->id == ctx->widget.hoveredId && ctx->tooltip.id
		&& ctx->tooltip.show)
	{
		ctx->tooltip.wasShown = true;
		auto& bodyElemState = ctx->theme->getElement(WidgetElementId::TooltipBody).normalState();

		FontTextSize fsize = bodyElemState.font->computeTextSize(text);
		Rect rect = {
			ctx->tooltip.position.x + ctx->tooltip.offsetFromCursor + bodyElemState.border * ctx->scale,
			ctx->tooltip.position.y + ctx->tooltip.offsetFromCursor + bodyElemState.border * ctx->scale,
			fsize.width, fsize.height };

		if (rect.right() > ctx->renderer.getWindowRect().right())
		{
			rect.x = ctx->renderer.getWindowRect().right() - rect.width;
		}

		if (rect.bottom() > ctx->renderer.getWindowRect().bottom())
		{
			rect.y = ctx->renderer.getWindowRect().bottom() - rect.height;
		}

		if (rect.x < 0)
		{
			rect.x = 0;
		}

		if (rect.y < 0)
		{
			rect.y = 0;
		}

		ctx->renderer.pushWindowDrawCmdLayer(DrawCmdLayerType::Overlay);
		ctx->renderer.pushClipRect(ctx->renderer.getWindowRect(), false);

		Rect rc = rect;
	
		ctx->renderer.cmdSetColor(bodyElemState.color);
		rc.x -= bodyElemState.border * ctx->scale;
		rc.y -= bodyElemState.border * ctx->scale;
		rc.width += bodyElemState.border * 2 * ctx->scale;
		rc.height += bodyElemState.border * 2 * ctx->scale;
		ctx->renderer.cmdDrawImageBordered(
			bodyElemState.image, bodyElemState.border, rc, ctx->scale);

		ctx->renderer.cmdSetColor(bodyElemState.textColor);
		ctx->renderer.cmdSetFont(bodyElemState.font);
		ctx->renderer.cmdDrawTextInBox(
			text, rect,
			HAlignType::Center, VAlignType::Center);

		ctx->renderer.popClipRect();
		ctx->renderer.popWindowDrawCmdLayer();

		return true;
	}

	return false;
}

bool customTooltipBegin(f32 width)
{
	if (ctx->id != ctx->tooltip.id && ctx->id == ctx->widget.hoveredId && !ctx->tooltip.show)
	{
		ctx->tooltip.id = ctx->id;
		ctx->tooltip.timer = 0;
	}

	if ((ctx->id == ctx->widget.hoveredId && ctx->tooltip.id
		&& ctx->tooltip.show) || ctx->tooltip.closeTooltipPopup)
	{
		ctx->tooltip.wasShown = true;
		auto& bodyElemState = ctx->theme->getElement(WidgetElementId::TooltipBody).normalState();
		popupBegin(
			"##customTooltip",
			width,
			PopupFlags::CustomPosition | PopupFlags::TopMost | PopupFlags::SameLayer,
			{ ctx->tooltip.position.x + ctx->tooltip.offsetFromCursor, ctx->tooltip.position.y + ctx->tooltip.offsetFromCursor },
			WidgetElementId::TooltipBody);

		if (ctx->tooltip.closeTooltipPopup)
		{
			popupClose();
			ctx->tooltip.closeTooltipPopup = false;
		}

		return true;
	}

	return false;
}

void customTooltipEnd()
{
	if (ctx->tooltip.wasShown)
	{
		popupEnd();
	}
}

}
