#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"
#include "docking.h"

namespace hui
{
void beginTabGroup(TabIndex selectedIndex)
{
	auto& tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->scale;

	ctx->widget.rect.set(
		round(ctx->position.x),
		round(ctx->position.y),
		ctx->layout.width + ctx->settings.dockNodeSpacing, // extend so we can draw the dock spacing on right
		height);

	ctx->tabGroupWidgetRect = ctx->widget.rect;

	// tab group background
	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->pushClipRect(
		{
			round(ctx->position.x),
			round(ctx->position.y),
			ctx->layout.width + ctx->settings.dockNodeSpacing, // extend so we cover the dock spacing on right
			ctx->layout.height + ctx->settings.dockNodeSpacing + 1 // extend so we can draw the dock spacing on bottom
		});

	ctx->renderer->cmdSetColor(tabGroupElemState.color);
	ctx->renderer->cmdDrawImageBordered(tabGroupElemState.image, tabGroupElemState.border, ctx->widget.rect, ctx->scale);

	if (ctx->docking.drawingWindowTabs)
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
				ctx->docking.currentDockNode->rect.height - height + ctx->settings.dockNodeSpacing // add dock node spacing to cover that too
			}, ctx->scale);

		// draw the horizontal splitter for dock node resize
		auto& windowHorizontalSplitterElemState = ctx->theme->getElement(WidgetElementId::WindowHorizontalSplitter).normalState();
		ctx->renderer->cmdSetColor(windowHorizontalSplitterElemState.color);
		ctx->renderer->cmdDrawImageBordered(
			windowHorizontalSplitterElemState.image,
			windowHorizontalSplitterElemState.border,
			{
				ctx->widget.rect.x,
				ctx->widget.rect.y + ctx->docking.currentDockNode->rect.height,
				ctx->docking.currentDockNode->rect.width,
				ctx->settings.dockNodeSpacing,
			}, ctx->scale);
	}

	ctx->selectedTabIndex = selectedIndex;
	ctx->currentTabIndex = 0;
}

TabIndex endTabGroup()
{
	auto& tabGroupElemState = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState();
	f32 height = tabGroupElemState.height * ctx->scale;

	ctx->position.x = ctx->layout.savedPosition.x;
	ctx->position.y += height;
	ctx->position.y = round(ctx->position.y);
	ctx->renderer->popClipRect();

	if (ctx->event.type == InputEvent::Type::MouseDown)
	{
		if (ctx->tabGroupWidgetRect.contains(ctx->mousePosition) && ctx->docking.currentDockNode)
		{
			focusWindow(ctx->docking.currentDockNode->windows[ctx->selectedTabIndex]->id.c_str());
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
	auto& padding = getWidgetPadding();

	Utf32String* uniStr = ctx->textCache->getText(label);
	FontTextSize fsize = tabElemState->font->computeTextSize(*uniStr);
	Image* ico = (Image*)icon;

	f32 width = 0;
	f32 iconWidth = 0;

	if (icon)
	{
		iconWidth = ico->rect.width;
	}

	f32 textAndIconWidth = (fsize.width + iconWidth * 2.0f /* some space after text as icon width */ + ctx->settings.dockTabIconTextSpacing) * ctx->scale;
	
	width = textAndIconWidth + (tabElemState->border + padding.x) * 2.0f * ctx->scale;

	f32 height = (tabElemState->height + padding.y * 2.0f) * ctx->scale;

	ctx->widget.rect.set(
		round(ctx->position.x),
		round(ctx->position.y + std::max(tabGroupElemState.height * ctx->scale, height) - height),
		width,
		height);
	ctx->position.x += width;
	ctx->position.x = round(ctx->position.x);

	mouseDownOnlyButtonBehavior();

	if (ctx->widget.clicked)
	{
		ctx->selectedTabIndex = ctx->currentTabIndex;
	}

	bool isActive = ctx->selectedTabIndex == ctx->currentTabIndex;
	bool isFocused = ctx->currentWindow == ctx->docking.focusedWindow;

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
	ctx->renderer->cmdDrawImageBordered(tabElemState->image, tabElemState->border, ctx->widget.rect, ctx->scale);

	Rect rcTextAndIcon = {
		ctx->widget.rect.x + (tabElemState->border + padding.x) * ctx->scale,
		ctx->widget.rect.y,
		textAndIconWidth,
		ctx->widget.rect.height };

	if (ico)
	{
		ctx->renderer->cmdSetColor(tabElemState->textColor);
		ctx->renderer->cmdDrawImageScaledAligned(ico,
			rcTextAndIcon, HAlignType::Left, VAlignType::Center, ctx->scale);
	}

	ctx->renderer->cmdSetFont(tabElemState->font);
	ctx->renderer->cmdSetColor(tabElemState->textColor);

	Rect textRc = {
			ctx->widget.rect.x + (padding.x + tabElemState->border + iconWidth + ctx->settings.dockTabIconTextSpacing) * ctx->scale,
			ctx->widget.rect.y,
			width,
			ctx->widget.rect.height,
	};

	ctx->renderer->pushClipRect(textRc);
	ctx->renderer->cmdDrawTextInBox(
		label,
		textRc,
		HAlignType::Left,
		VAlignType::Center);
	ctx->renderer->popClipRect();
	ctx->currentTabIndex++;
}

}
