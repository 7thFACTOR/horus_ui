#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
bool image(HImage img, f32 height, HAlignType horizontalAlign, VAlignType verticalAlign, ImageFitType fit)
{
	Image* imgPtr = (Image*)img;
	bool autoHeight = false;

	if (height <= 0)
	{
		height = imgPtr->rect.height * ctx->scale;
		autoHeight = true;
	}
	else
	{
		height *= ctx->scale;
	}

	f32 newWidth = imgPtr->rect.width * ctx->scale;
	f32 newHeight = height;

	if (fit == ImageFitType::KeepAspect)
	{
		viewportImageFitSize(
			imgPtr->rect.width * ctx->scale,
			imgPtr->rect.height * ctx->scale,
			ctx->layout.width,
			height, newWidth, newHeight, false, false);
	}
	else if (fit == ImageFitType::Stretch)
	{
		newWidth = ctx->layout.width;
	}

	if (autoHeight)
	{
		height = newHeight;
	}

	//TODO: img can be the id
	addWidget("", height);

	Point pos = ctx->widget.rect.topLeft();

	switch (horizontalAlign)
	{
	case hui::HAlignType::Right:
		pos.x = ctx->widget.rect.right() - newWidth;
		break;
	case hui::HAlignType::Center:
		pos.x += (ctx->widget.rect.width - newWidth) / 2.0f;
		break;
	}

	switch (verticalAlign)
	{
	case hui::VAlignType::Bottom:
		pos.y = ctx->widget.rect.bottom() - newHeight;
		break;
	case hui::VAlignType::Center:
		pos.y += (ctx->widget.rect.height - newHeight) / 2.0f;
		break;
	}

	ctx->widget.rect = { pos.x, pos.y, newWidth, newHeight };
	buttonBehavior();

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(Color::white);
		ctx->renderer->cmdDrawImage(imgPtr, ctx->widget.rect);
	}

	return ctx->widget.clicked;
}

}