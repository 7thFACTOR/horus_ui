#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
bool colorPickerPopup(const Color& currentColor, Color& outNewColor)
{
	Color newColor = currentColor;
	f32 height = 250;
	addWidgetItem(height);

	ctx->renderer->cmdDrawInterpolatedColors(
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2 }
	, Color::white, Color::black, Color::blue, Color::black);

	ctx->renderer->cmdDrawSpectrumColors({
		ctx->widget.rect.x + ctx->widget.rect.width / 2,
		ctx->widget.rect.y + ctx->widget.rect.height / 2,
		ctx->widget.rect.width / 2,
		ctx->widget.rect.height / 2 },
		Renderer::DrawSpectrumBrightness::On,
		Renderer::DrawSpectrumDirection::Horizontal);

	ctx->renderer->cmdDrawInterpolatedColorsTopBottom(
		{
			ctx->widget.rect.x + ctx->widget.rect.width / 2,
			ctx->widget.rect.y,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2 }
	, Color::red, Color::black);
	ctx->renderer->cmdDrawInterpolatedColorsLeftRight(
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + ctx->widget.rect.height / 2,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2 },
			Color::white, Color::black);

	ctx->currentWidgetId++;
	return true;
}

}