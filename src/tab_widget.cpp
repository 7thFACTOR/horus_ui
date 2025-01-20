#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"
#include "docking.h"
#include <math.h>

namespace hui
{
Rect tabGroupWidgetRect;

void beginTabGroup(TabIndex selectedIndex)
{
	auto& tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->globalScale;

	ctx->widget.rect.set(
		round(ctx->penPosition.x),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width + ctx->settings.dockNodeSpacing, // extend so we can draw the dock spacing on right
		height);

	tabGroupWidgetRect = ctx->widget.rect;

	// tab group background
	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->pushClipRect(
		{
			round(ctx->penPosition.x),
			round(ctx->penPosition.y),
			ctx->layoutStack.back().width + ctx->settings.dockNodeSpacing, // extend so we cover the dock spacing on right
			ctx->layoutStack.back().height + ctx->settings.dockNodeSpacing + 1 // extend so we can draw the dock spacing on bottom
		});

	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->cmdDrawImageBordered(tabGroupElemState.image, tabGroupElemState.border, ctx->widget.rect, ctx->globalScale);

	if (ctx->dockingState.drawingWindowTabs)
	{
		// draw the vertical splitter for dock node resize
		auto& windowVerticalSplitterElemState = ctx->theme->getElement(WidgetElementId::WindowVerticalSplitter).normalState();
		ctx->renderer->cmdSetColor(windowVerticalSplitterElemState.color);
		ctx->renderer->cmdDrawImageBordered(
			windowVerticalSplitterElemState.image,
			windowVerticalSplitterElemState.border,
			{
				ctx->widget.rect.right() - ctx->settings.dockNodeSpacing,
				ctx->widget.rect.y + height,
				ctx->settings.dockNodeSpacing,
				ctx->containerRect.height - height + ctx->settings.dockNodeSpacing // add dock node spacing to cover that too
			}, ctx->globalScale);

		// draw the horizontal splitter for dock node resize
		auto& windowHorizontalSplitterElemState = ctx->theme->getElement(WidgetElementId::WindowHorizontalSplitter).normalState();
		ctx->renderer->cmdSetColor(windowHorizontalSplitterElemState.color);
		ctx->renderer->cmdDrawImageBordered(
			windowHorizontalSplitterElemState.image,
			windowHorizontalSplitterElemState.border,
			{
				ctx->widget.rect.x,
				ctx->widget.rect.y + ctx->containerRect.height,
				ctx->containerRect.width,
				ctx->settings.dockNodeSpacing,
			}, ctx->globalScale);
	}

	ctx->selectedTabIndex = selectedIndex;
	ctx->currentTabIndex = 0;
}

TabIndex endTabGroup()
{
	auto& tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->globalScale;

	ctx->penPosition.x = ctx->layoutStack.back().position.x;
	ctx->penPosition.y += height;
	ctx->penPosition.y = round(ctx->penPosition.y);
	ctx->renderer->popClipRect();

	if (ctx->event.type == InputEvent::Type::MouseDown)
	{
		if (tabGroupWidgetRect.contains(ctx->mousePosition) && ctx->dockingState.currentDockNode)
		{
			focusWindow(ctx->dockingState.currentDockNode->windows[ctx->selectedTabIndex]->id.c_str());
		}
	}

	return ctx->selectedTabIndex;
}

void tab(const char* label, HImage icon)
{
	auto& tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	auto& tabActiveElem = ctx->theme->getElement(WidgetElementId::TabBodyActive);
	auto& tabInactiveElem = ctx->theme->getElement(WidgetElementId::TabBodyInactive);
	auto tabElemState = &tabActiveElem.normalState();
	
	Utf32String* uniStr = ctx->textCache->getText(label);
	FontTextSize fsize = tabElemState->font->computeTextSize(*uniStr);
	Image* ico = (Image*)icon;

	f32 width = 0;
	f32 iconWidth = 0;

	if (icon)
	{
		iconWidth = ico->rect.width;
	}

	const f32 iconTextSpacing = 4;
	f32 textAndIconWidth = (fsize.width + iconWidth * 2.0f /* some space after text as icon width */ + iconTextSpacing) * ctx->globalScale;
	
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

	bool isActive = ctx->selectedTabIndex == ctx->currentTabIndex;
	bool isFocused = ctx->currentWindow == ctx->dockingState.focusedWindow;

	if (isActive)
	{
		if (isFocused)
			tabElemState = &tabActiveElem.focusedState();
		else if (ctx->widget.hovered)
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
		ctx->renderer->cmdSetColor(tabElemState->textColor);
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

	ctx->renderer->cmdDrawTextInBox(label,
		textRc,
		HAlignType::Left, VAlignType::Center);

	ctx->renderer->popClipRect();
	ctx->currentTabIndex++;
}

}
