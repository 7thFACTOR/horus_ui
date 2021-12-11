#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include "context.h"
#include "util.h"

namespace hui
{
void line()
{
	auto bodyElemState = ctx->theme->getElement(WidgetElementId::LineBody).normalState();

	addWidgetItem(bodyElemState.image->rect.height * ctx->globalScale);
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState.image, bodyElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height }, ctx->globalScale);
	ctx->currentWidgetId++;
}

void gap(f32 size)
{
	ctx->penPosition.y += size * ctx->globalScale;
}

void space()
{
	ctx->penPosition.y += ctx->spacing * ctx->globalScale;
}

void beginSameLine()
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
	//	ctx->penPosition.x = ctx->layoutStack.back().position.x;

	// push current line index to stack, so we recover it
	ctx->sameLineInfoIndexStack.push_back(ctx->sameLineInfoIndex);
	ctx->widget.sameLine = true;
	ctx->sameLineInfoCount++;
}

void endSameLine()
{
	ctx->sameLineInfoIndex = ctx->sameLineInfoIndexStack.back();
	ctx->sameLineInfoIndexStack.pop_back();

	// we stop same line if this is a root same line
	if (!ctx->sameLineInfoIndexStack.size())
		ctx->widget.sameLine = false;

	if (!ctx->sameLineInfoIndexStack.size())
		ctx->penPosition.y += ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight;
	else
	{
		//TODO: not working here, needs a proper stack
		ctx->sameLineInfo[ctx->sameLineInfoIndex-1].lineHeight = fmaxf(
			ctx->sameLineInfo[ctx->sameLineInfoIndex - 1].lineHeight,
			ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight
		);
	}
}

void pushSameLineSpacing(f32 horizontalSpace)
{
	ctx->sameLineSpacingStack.push_back(ctx->widget.sameLineSpacing);
	ctx->widget.sameLineSpacing = horizontalSpace;
}

f32 popSameLineSpacing()
{
	if (ctx->sameLineSpacingStack.size())
	{
		ctx->widget.sameLineSpacing = ctx->sameLineSpacingStack.back();
		ctx->sameLineSpacingStack.pop_back();
		return ctx->widget.sameLineSpacing;
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
	if (ctx->sameLineWidthStack.size())
	{
		ctx->widget.width = ctx->sameLineWidthStack.back();
		ctx->sameLineWidthStack.pop_back();
		return ctx->widget.width;
	}

	return 0;
}

}