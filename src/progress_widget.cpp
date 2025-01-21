#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
void progress(f32 value)
{
	//TODO: animated frames like barbershop progress?
	auto& backElem = ctx->theme->getElement(WidgetElementId::ProgressBack);
	auto& fillElem = ctx->theme->getElement(WidgetElementId::ProgressFill);

	// clamp
	const f32 maxValue = 1.0f;
	value = fmaxf(0, fminf(maxValue, value));

	addWidgetItem("", backElem.normalState().height * ctx->scale);

	f32 percentFilled = value / maxValue;
	f32 valueWidth = ctx->widget.rect.width;

	auto& backElemState = backElem.normalState();
	auto& fillElemState = fillElem.normalState();

	ctx->renderer->cmdSetColor(backElemState.color);
	ctx->renderer->cmdDrawImageBordered(backElemState.image, backElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (backElemState.height - backElemState.image->rect.height) / 2.0f * ctx->scale,
			ctx->widget.rect.width,
			backElemState.image->rect.height * ctx->scale }, ctx->scale);

	ctx->renderer->cmdSetColor(fillElemState.color);
	ctx->renderer->cmdDrawImageBordered(fillElemState.image, fillElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (backElemState.height - fillElemState.image->rect.height) / 2.0f * ctx->scale,
			ctx->widget.rect.width * percentFilled,
			fillElemState.image->rect.height * ctx->scale }, ctx->scale);
}

}
