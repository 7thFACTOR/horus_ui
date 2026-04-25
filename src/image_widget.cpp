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
		height = imgPtr->height * ctx->scale;
		autoHeight = true;
	}
	else
	{
		height *= ctx->scale;
	}

	f32 newWidth = imgPtr->width * ctx->scale;
	f32 newHeight = height;

	auto& padding = widgetGetPadding();

	if (!ctx->sameLine.enabled)
	{
		if (fit == ImageFitType::KeepAspect)
		{
			viewportImageSizeFit(
				imgPtr->width * ctx->scale,
				imgPtr->height * ctx->scale,
				ctx->layout.width - padding.x * 2.0f * ctx->scale,
				height, newWidth, newHeight, false, false);
		}
		else if (fit == ImageFitType::Stretch)
		{
			newWidth = ctx->layout.width - padding.x * 2.0f * ctx->scale;
		}
	}
	else
	{
		if (fit == ImageFitType::KeepAspect)
		{
			viewportImageSizeFit(
				imgPtr->width * ctx->scale,
				imgPtr->height * ctx->scale,
				0.0f,
				height, newWidth, newHeight, false, true);
		}
	}

	if (autoHeight)
	{
		height = newHeight;
	}

	ctx->widget.customWidth = newWidth;
	ctx->widget.hasCustomWidth = true;

	ctx->id = genId(img);
	addWidget(height);

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
		ctx->renderer.cmdSetColor(Color::white);
		ctx->renderer.cmdDrawImage(imgPtr, ctx->widget.rect);
	}

	return ctx->widget.clicked;
}

bool texture(HTexture texture, f32 textureWidth, f32 textureHeight, f32 height, HAlignType horizontalAlign, VAlignType verticalAlign, ImageFitType fit)
{
	bool autoHeight = false;

	if (height <= 0)
	{
		height = textureHeight * ctx->scale;
		autoHeight = true;
	}
	else
	{
		height *= ctx->scale;
	}

	f32 newWidth = textureWidth * ctx->scale;
	f32 newHeight = height;

	auto& padding = widgetGetPadding();

	if (!ctx->sameLine.enabled)
	{
		if (fit == ImageFitType::KeepAspect)
		{
			viewportImageSizeFit(
				textureWidth * ctx->scale,
				textureHeight * ctx->scale,
				ctx->layout.width - padding.x * 2.0f * ctx->scale,
				height, newWidth, newHeight, false, false);
		}
		else if (fit == ImageFitType::Stretch)
		{
			newWidth = ctx->layout.width - padding.x * 2.0f * ctx->scale;
		}
	}
	else
	{
		if (fit == ImageFitType::KeepAspect)
		{
			viewportImageSizeFit(
				textureWidth * ctx->scale,
				textureHeight * ctx->scale,
				0.0f,
				height, newWidth, newHeight, false, true);
		}
	}

	if (autoHeight)
	{
		height = newHeight;
	}

	ctx->widget.customWidth = newWidth;
	ctx->widget.hasCustomWidth = true;

	ctx->id = genId(texture);
	addWidget(height);

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
		ctx->renderer.cmdSetColor(Color::white);

		HTexture crtTex = ctx->renderer.currentTexture;
		u32 crtTexWidth = ctx->renderer.currentTextureWidth;
		u32 crtTexHeight = ctx->renderer.currentTextureHeight;
		ctx->renderer.cmdSetTexture(texture, textureWidth, textureHeight);
		Image img;
		img.uvRect.set(0, 0, 1, 1);
		ctx->renderer.cmdDrawImage(&img, ctx->widget.rect);
		ctx->renderer.cmdSetTexture(crtTex, crtTexWidth, crtTexHeight);
	}

	return ctx->widget.clicked;
}

}