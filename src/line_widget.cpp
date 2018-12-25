#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "ui_theme.h"
#include "ui_context.h"
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

	// only a root sameline can start a new line, the others will just follow
	if (ctx->sameLineInfoStack.size() <= 1)
		ctx->penPosition.x = ctx->layoutStack.back().position.x;

	// push current line to stack
	ctx->sameLineInfoStack.push_back(ctx->sameLineInfo[ctx->sameLineInfoIndex]);

	ctx->widget.sameLine = true;
	ctx->sameLineInfoIndex++;
}

void endSameLine()
{
	auto sli = ctx->sameLineInfoStack.back();
	
	ctx->sameLineInfoStack.pop_back();

	// we stop same line if this is a root same line
	if (!ctx->sameLineInfoStack.size())
		ctx->widget.sameLine = false;

	if (ctx->sameLineInfoStack.size() == 0)
		ctx->penPosition.y += ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight;
	else
		ctx->sameLineInfo[ctx->sameLineInfoIndex - 1].lineHeight = fmaxf(
			ctx->sameLineInfo[ctx->sameLineInfoIndex - 1].lineHeight,
			ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight
		);
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