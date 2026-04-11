#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"

namespace hui
{
void line()
{
	auto& bodyElemState = ctx->theme->getElement(WidgetElementId::LineBody).normalState();
	auto& padding = widgetGetPadding();

	ctx->labelAndIdSet(nullptr);
	widgetAdd(bodyElemState.image->height + padding.y * 2.0f);
	ctx->renderer.cmdSetColor(bodyElemState.color);
	ctx->renderer.cmdDrawImageBordered(bodyElemState.image, bodyElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height }, ctx->scale);
}

void space(f32 customSpacing)
{
	if (ctx->sameLine.enabled)
	{
		f32 spacing = customSpacing > 0 ? customSpacing : ctx->sameLine.spacing;

		ctx->position.x += spacing * ctx->scale;

		return;
	}

	f32 spacing = customSpacing > 0 ? customSpacing : ctx->spacing;
	ctx->position.y += spacing * ctx->scale;
}

void widgetSetNextWidth(f32 width)
{
	ctx->widget.nextWidth = width;
	ctx->widget.hasNextWidth = true;
}

void sameLine(f32 offsetX, f32 spacing)
{
	f32 actualSpacing = spacing > 0 ? spacing : ctx->sameLine.spacing;
	actualSpacing += offsetX;
	
	ctx->sameLine.nextSpacing = actualSpacing;
	ctx->sameLine.enabled = true;
}

}