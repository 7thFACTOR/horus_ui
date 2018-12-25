#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
void beginToolbar(ToolbarDirection dir)
{
	auto el = &ctx->theme->getElement(WidgetElementId::ToolbarBody);
	ctx->verticalToolbar = dir == ToolbarDirection::Vertical;
	// we set the highest Y so we can center vertically the other widgets on the toolbar, the toolbar buttons are of this height

	if (!ctx->verticalToolbar)
		beginSameLine();
}

void endToolbar()
{
	if (!ctx->verticalToolbar)
		endSameLine();
	
	ctx->verticalToolbar = false;
}

bool toolbarButton(Image normalIcon, Image disabledIcon, bool down)
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

bool toolbarDropdown(const char* label, Image normalIcon, Image disabledIcon)
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

bool toolbarTextInput(char* outText, u32 maxOutTextSize, const char* hint, Image icon)
{
	return textInput(outText, maxOutTextSize, TextInputValueMode::Any, hint, icon);
}

}