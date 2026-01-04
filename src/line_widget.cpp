#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"

namespace hui
{
void line()
{
	auto& bodyElemState = ctx->theme->getElement(WidgetElementId::LineBody).normalState();
	auto& padding = getWidgetPadding();

	ctx->extractLabelAndId(nullptr);
	addWidget((bodyElemState.image->height + padding.y * 2.0f) * ctx->scale);
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState.image, bodyElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height }, ctx->scale);
}

void customSpace(f32 size)
{
	ctx->position.y += size * ctx->scale;
}

void space()
{
	if (ctx->sameLine)
	{
		ctx->position.x += ctx->sameLineSpacing * ctx->scale;
		return;
	}

	ctx->position.y += ctx->spacing * ctx->scale;
}

void beginSameLine(f32 spacing)
{
	ctx->sameLineInfoIndex = ctx->sameLineInfoCount;

	if (ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight == 0)
	{
		ctx->sameLineInfo[ctx->sameLineInfoIndex].computeHeight = true;
		skipThisFrame();
		forceRepaint();
	}
	else
	{
		ctx->sameLineInfo[ctx->sameLineInfoIndex].computeHeight = false;
	}

	//// only a root same line can start a new line, the others will just follow
	//if (ctx->sameLineInfoIndexStack.size() <= 1)
	//	ctx->position.x = ctx->layout.savedPosition.x;

	// push current line index to stack, so we recover it
	ctx->sameLineInfoIndexStack.push_back(ctx->sameLineInfoIndex);
	ctx->sameLine = true;
	ctx->sameLineInfoCount++;

	if (spacing > 0.0f)
		ctx->sameLineSpacing = spacing;

	pushPosition();
}

void endSameLine()
{
	popPosition();
	ctx->sameLineInfoIndex = ctx->sameLineInfoIndexStack.back();
	ctx->sameLineInfoIndexStack.pop_back();

	// we stop same line if this is a root same line
	//if (!ctx->sameLineInfoIndexStack.size())
		ctx->sameLine = false;

	if (!ctx->sameLineInfoIndexStack.size())
		ctx->position.y += ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight;
	else
	{
		//TODO: not working here, needs a proper stack
		ctx->sameLineInfo[ctx->sameLineInfoIndex-1].lineHeight = fmaxf(
			ctx->sameLineInfo[ctx->sameLineInfoIndex - 1].lineHeight,
			ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight
		);
	}

	ctx->widget.width = 0;
}

void pushSameLineSpacing(f32 horizontalSpace)
{
	ctx->sameLineSpacingStack.push_back(ctx->sameLineSpacing);
	ctx->sameLineSpacing = horizontalSpace;
}

f32 popSameLineSpacing()
{
	if (ctx->sameLineSpacingStack.size())
	{
		ctx->sameLineSpacing = ctx->sameLineSpacingStack.back();
		ctx->sameLineSpacingStack.pop_back();
		return ctx->sameLineSpacing;
	}

	return 0;
}

void pushWidth(f32 width)
{
	ctx->sameLineWidthStack.push_back(ctx->widget.width);
	ctx->widget.width = width;
}

f32 popWidth()
{
	if (!ctx->sameLineWidthStack.empty())
	{
		ctx->widget.width = ctx->sameLineWidthStack.back();
		ctx->sameLineWidthStack.pop_back();

		return ctx->widget.width;
	}

	return 0;
}

void setNextWidth(f32 width)
{
	ctx->widget.nextWidth = width;
	ctx->widget.hasNextWidth = true;
}

}