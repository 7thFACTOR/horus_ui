#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"
#include <math.h>

namespace hui
{
void progress(f32 value)
{
	//TODO: animated frames like barbershop progress?
	auto backElem = ctx->theme->getElement(WidgetElementId::ProgressBack);
	auto fillElem = ctx->theme->getElement(WidgetElementId::ProgressFill);

	// clamp
	const f32 maxValue = 1.0f;
	value = fmaxf(0, fminf(maxValue, value));

	addWidgetItem(backElem.normalState().height * ctx->globalScale);

	f32 percentFilled = value / maxValue;
	f32 valueWidth = ctx->widget.rect.width;

	auto backElemState = backElem.normalState();
	auto fillElemState = fillElem.normalState();

	ctx->renderer->cmdSetColor(backElemState.color);
	ctx->renderer->cmdDrawImageBordered(backElemState.image, backElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (backElemState.height - backElemState.image->rect.height) / 2.0f * ctx->globalScale,
			ctx->widget.rect.width,
			backElemState.image->rect.height * ctx->globalScale }, ctx->globalScale);

	ctx->renderer->cmdSetColor(fillElemState.color);
	ctx->renderer->cmdDrawImageBordered(fillElemState.image, fillElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (backElemState.height - fillElemState.image->rect.height) / 2.0f * ctx->globalScale,
			ctx->widget.rect.width * percentFilled,
			fillElemState.image->rect.height * ctx->globalScale }, ctx->globalScale);
	ctx->currentWidgetId++;
}

}
