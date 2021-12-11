#include "horus.h"
#include "types.h"
#include "theme.h"
#include "context.h"
#include "util.h"

namespace hui
{
void beginToolbar(ToolbarDirection dir)
{
	ctx->verticalToolbar = dir == ToolbarDirection::Vertical;
	ctx->sameLineStack.push_back(ctx->widget.sameLine);

	if (!ctx->verticalToolbar)
		beginSameLine();
	else
		ctx->widget.sameLine = false;

	ctx->verticalToolbarStack.push_back(ctx->verticalToolbar);
}

void endToolbar()
{
	ctx->verticalToolbar = ctx->verticalToolbarStack.back();
	ctx->verticalToolbarStack.pop_back();
	ctx->sameLineStack.pop_back();

	if (!ctx->verticalToolbar)
		endSameLine();

	ctx->verticalToolbar = false;

	if (ctx->sameLineStack.size())
		ctx->widget.sameLine = ctx->sameLineStack.back();
	else
		ctx->widget.sameLine = false;
}

bool toolbarButton(HImage normalIcon, HImage disabledIcon, bool down)
{
	auto el = &ctx->theme->getElement(WidgetElementId::ToolbarButtonBody);

	pushWidth(el->normalState().width);

	bool ret = iconButtonInternal(
		normalIcon, disabledIcon,
		el->normalState().height,
		down,
		el, false);

	popWidth();
	
	return ret;
}

bool toolbarDropdown(const char* label, HImage normalIcon, HImage disabledIcon)
{
	return false;
}

void toolbarSeparator()
{
	auto elId = ctx->verticalToolbar ? WidgetElementId::ToolbarSeparatorHorizontalBody : WidgetElementId::ToolbarSeparatorVerticalBody;
	auto bodyElemState = ctx->theme->getElement(elId).normalState();

	addWidgetItem(bodyElemState.height * ctx->globalScale);
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState.image, bodyElemState.border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			(f32)bodyElemState.width,
			ctx->widget.rect.height }, ctx->globalScale);
	ctx->currentWidgetId++;

	toolbarGap();
}

void toolbarGap(f32 gapSize)
{
	if (ctx->verticalToolbar)
		gap(gapSize);
	else
		ctx->penPosition.x += gapSize;
}

bool toolbarTextInputFilter(char* outText, u32 maxOutTextSize, u32& filterIndex, const char** filterNames, u32 filterNameCount)
{
	return false;
}

bool toolbarTextInput(char* outText, u32 maxOutTextSize, const char* hint, HImage icon)
{
	return textInput(outText, maxOutTextSize, TextInputValueMode::Any, hint, icon);
}

}