#include "horus.h"
#include "types.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
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
		ctx->layoutStack.back().width + ctx->settings.dockNodeSpacing,
		height);

	// tab group background
	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->pushClipRect(
		{
			round(ctx->penPosition.x),
			round(ctx->penPosition.y - ctx->settings.dockNodeSpacing),
			ctx->layoutStack.back().width + ctx->settings.dockNodeSpacing,
			ctx->containerRect.height + ctx->settings.dockNodeSpacing
		});

	Rect rcTop = {
		round(ctx->penPosition.x),
		round(ctx->penPosition.y - ctx->settings.dockNodeSpacing),
		ctx->layoutStack.back().width + ctx->settings.dockNodeSpacing,
		ctx->settings.dockNodeSpacing };

	ctx->renderer->cmdSetColor(Color::black);
	ctx->renderer->cmdDrawImageBordered(tabGroupElemState.image, tabGroupElemState.border, rcTop, ctx->globalScale);
	ctx->renderer->cmdSetColor(tabGroupElemState.color);

	ctx->renderer->cmdDrawImageBordered(tabGroupElemState.image, tabGroupElemState.border, ctx->widget.rect, ctx->globalScale);

	if (ctx->dockingState.drawingWindowTabs)
	{
		// vertical splitter
		//NOTE: horizontal splitter is not needed because there is tab group background
		ctx->renderer->cmdSetColor(Color::black);
		ctx->renderer->cmdDrawImage(
			tabGroupElemState.image,
			{
				ctx->widget.rect.right() - ctx->settings.dockNodeSpacing,
				ctx->widget.rect.y + height,
				ctx->settings.dockNodeSpacing,
				ctx->containerRect.height + ctx->settings.dockNodeSpacing
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

void tab(const char* labelText, HImage icon)
{
	auto tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	auto tabActiveElem = ctx->theme->getElement(WidgetElementId::TabBodyActive);
	auto tabInactiveElem = ctx->theme->getElement(WidgetElementId::TabBodyInactive);
	auto tabElemState = &tabActiveElem.normalState();
	
	Utf32String* uniStr = ctx->textCache->getText(labelText);
	FontTextSize fsize = tabElemState->font->computeTextSize(*uniStr);
	Image* ico = (Image*)icon;

	f32 width = 0;
	f32 iconWidth = 0;

	if (icon)
	{
		iconWidth = ico->rect.width;
	}

	const f32 iconTextSpacing = 4;
	f32 textAndIconWidth = (fsize.width + iconWidth + iconTextSpacing) * ctx->globalScale;
	
	width = textAndIconWidth + tabElemState->border * 2.0f * ctx->globalScale;

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

	bool isActive = ctx->currentTabIndex == ctx->selectedTabIndex;

	if (isActive)
	{
		if (ctx->widget.hovered)
			tabElemState = &tabActiveElem.hoveredState();
		else
			tabElemState = &tabActiveElem.normalState();
	}
	else
	{
		if (ctx->widget.hovered)
			tabElemState = &tabInactiveElem.hoveredState();
		else
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
		ctx->renderer->cmdSetColor(Color::white);
		ctx->renderer->cmdDrawImageScaledAligned(ico,
			rcTextAndIcon, HAlignType::Left, VAlignType::Center, ctx->globalScale);
	}

	ctx->renderer->cmdSetFont(tabElemState->font);
	ctx->renderer->cmdSetColor(tabElemState->textColor);

	Rect textRc = {
			ctx->widget.rect.x + (tabElemState->border + iconWidth + iconTextSpacing) * ctx->globalScale,
			ctx->widget.rect.y,
			width,
			ctx->widget.rect.height,
	};

	ctx->renderer->pushClipRect(textRc);

	ctx->renderer->cmdDrawTextInBox(labelText,
		textRc,
		HAlignType::Left, VAlignType::Bottom);

	ctx->renderer->popClipRect();
	ctx->currentTabIndex++;
	ctx->currentWidgetId++;
}

}
