#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
bool image(Image img, f32 height, HAlignType horizontalAlign, VAlignType verticalAlign, ImageFitType fit)
{
	UiImage* imgPtr = (UiImage*)img;
	bool autoHeight = false;

	if (height <= 0)
	{
		height = imgPtr->rect.height * ctx->globalScale;
		autoHeight = true;
	}
	else
	{
		height *= ctx->globalScale;
	}

	f32 newWidth = imgPtr->rect.width * ctx->globalScale;
	f32 newHeight = height;

	if (fit == ImageFitType::KeepAspect)
	{
		viewportImageFitSize(
			imgPtr->rect.width * ctx->globalScale,
			imgPtr->rect.height * ctx->globalScale,
			ctx->layoutStack.back().width - ctx->padding * 2 * ctx->globalScale,
			height, newWidth, newHeight, false, false);
	}
	else if (fit == ImageFitType::Stretch)
	{
		newWidth = ctx->layoutStack.back().width - ctx->padding * 2 * ctx->globalScale;
	}

	if (autoHeight)
	{
		height = newHeight;
	}

	addWidgetItem(height);

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

	ctx->currentWidgetId++;

	return false;
}

}