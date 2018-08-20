#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "text_cache.h"
#include "ui_font.h"
#include "ui_context.h"
#include "util.h"
#include <math.h>

namespace hui
{
void beginTabGroup(TabIndex selectedIndex)
{
	auto tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->globalScale;

	ctx->widget.rect.set(
		round(ctx->penPosition.x),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width,
		height);

	// tab group background
	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->pushClipRect(
	{
		round(ctx->penPosition.x),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width,
		ctx->containerRect.height
	});

	ctx->renderer->cmdDrawImageBordered(tabGroupElemState.image, tabGroupElemState.border, ctx->widget.rect, ctx->globalScale);

	if (ctx->drawingViewPaneTabs)
	{
		// vertical splitter
		//NOTE: horizontal splitter is not needed because there is tab group background
		ctx->renderer->cmdDrawImage(
			tabGroupElemState.image,
			{
				ctx->widget.rect.right() - 1,
				ctx->widget.rect.y + height,
				1,
				ctx->containerRect.height
			});
	}

	ctx->selectedTabIndex = selectedIndex;
	ctx->currentTabIndex = 0;
}

TabIndex endTabGroup()
{
	auto tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->globalScale;

	ctx->penPosition.x = ctx->layoutStack.back().position.x;
	ctx->penPosition.y += height;
	ctx->penPosition.y = round(ctx->penPosition.y);
	ctx->currentWidgetId++;
	ctx->renderer->popClipRect();

	return ctx->selectedTabIndex;
}

void tab(Utf8String labelText, Image icon)
{
	auto tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	auto tabActiveElem = ctx->theme->getElement(WidgetElementId::TabBodyActive);
	auto tabInactiveElem = ctx->theme->getElement(WidgetElementId::TabBodyInactive);
	auto tabElemState = &tabActiveElem.normalState();
	UnicodeString* uniStr = ctx->textCache->getText(labelText);
	FontTextSize fsize = tabElemState->font->computeTextSize(*uniStr);
	UiImage* ico = (UiImage*)icon;

	f32 width = 0;
	f32 iconWidth = 0;
	
	if (icon)
	{
		iconWidth = ico->rect.width;
	}

	f32 textAndIconWidth = fmaxf(fsize.width + iconWidth, tabElemState->width) * ctx->globalScale;

	if (ctx->paneGroupState.forceSqueezeTabs)
	{
		width = ctx->paneGroupState.forceTabWidth;
	}
	else
	{
		width = (ctx->paneGroupState.sideSpacing + ctx->paneGroupState.tabWidth) * ctx->globalScale;
	}

	f32 height = tabElemState->height * ctx->globalScale;

	ctx->widget.rect.set(
		round(ctx->penPosition.x),
		round(ctx->penPosition.y + tabGroupElemState.height * ctx->globalScale - height),
		width,
		height);
	ctx->penPosition.x += width;
	ctx->penPosition.x = round(ctx->penPosition.x);

	mouseDownOnlyButtonBehavior();

	if (ctx->widget.clicked)
	{
		ctx->selectedTabIndex = ctx->currentTabIndex;
	}

	//TODO: deal with hover also

	if (ctx->currentTabIndex != ctx->selectedTabIndex)
	{
		tabElemState = &tabInactiveElem.normalState();
	}

	ctx->renderer->cmdSetColor(tabElemState->color);
	ctx->renderer->cmdDrawImageBordered(tabElemState->image, tabElemState->border, ctx->widget.rect, ctx->globalScale);

	Rect rcTextAndIcon = {
		ctx->widget.rect.x + (tabElemState->border) * ctx->globalScale,
		ctx->widget.rect.y,
		textAndIconWidth,
		ctx->widget.rect.height };

	if (ico)
	{
		ctx->renderer->cmdDrawImageScaledAligned(ico,
		rcTextAndIcon, HAlignType::Left, VAlignType::Center, ctx->globalScale);
	}

	ctx->renderer->cmdSetFont(tabElemState->font);
	ctx->renderer->cmdSetColor(tabElemState->textColor);
	ctx->renderer->cmdDrawTextInBox(labelText,
	{
		ctx->widget.rect.x + (tabElemState->border + iconWidth) * ctx->globalScale,
		ctx->widget.rect.y,
		width, 
		ctx->widget.rect.height,
	},
	HAlignType::Left, VAlignType::Bottom);

	ctx->currentTabIndex++;
	ctx->currentWidgetId++;
}

}
