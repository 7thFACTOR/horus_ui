#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
//TODO
bool colorPickerPopup(const Color& currentColor, Color& outNewColor)
{
	Color newColor = currentColor;
	f32 height = 250;

	ctx->extractLabelAndId(nullptr);
	addWidget(height);

	ctx->renderer->cmdDrawQuad4Colors(
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height
		}
		, Color::white, Color::red
		, Color::red, Color::white);
	ctx->renderer->cmdDrawQuad4Colors(
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height
		}
		, Color::transparent, Color::transparent
		, Color::black, Color::black);
	auto rc = ctx->widget.rect;
	rc.width = 32;
	rc.height = 32;

	Color hsv;

	hsv.r = 0;
	hsv.g = (ctx->mousePosition.x - rc.x) / ctx->widget.rect.width;
	hsv.b = 1.0f - (ctx->mousePosition.y - rc.y) / ctx->widget.rect.height;

	ctx->renderer->cmdSetColor(hsvToRgb(hsv));
	ctx->renderer->cmdDrawSolidRectangle(rc);

	/*
	ctx->renderer->cmdDrawSpectrumColors(
		{
			ctx->widget.rect.x + ctx->widget.rect.width / 2,
			ctx->widget.rect.y + ctx->widget.rect.height / 2,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2
		},
		Renderer::DrawSpectrumBrightness::On,
		Renderer::DrawSpectrumDirection::Horizontal);

	ctx->renderer->cmdDrawInterpolatedColorsTopBottom(
		{
			ctx->widget.rect.x + ctx->widget.rect.width / 2,
			ctx->widget.rect.y,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2
		}
		, Color::red, Color::black);
	ctx->renderer->cmdDrawInterpolatedColorsLeftRight(
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + ctx->widget.rect.height / 2,
			ctx->widget.rect.width / 2,
			ctx->widget.rect.height / 2
		},
		Color::white, Color::black);
		*/
	return true;
}

}