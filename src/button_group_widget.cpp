#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"
#include <stdio.h>

namespace hui
{

bool buttonGroup(const char** labels, u32 count, u32* currentIndex)
{
	if (count == 0 || !labels || !currentIndex)
		return false;

	auto& leftElem = ctx->theme->getElement(WidgetElementId::ButtonGroupLeftBody);
	auto& middleElem = ctx->theme->getElement(WidgetElementId::ButtonGroupMiddleBody);
	auto& rightElem = ctx->theme->getElement(WidgetElementId::ButtonGroupRightBody);

	auto& padding = widgetGetPadding();
	f32 labelSideSpacing = leftElem.currentStyle->getParameter("labelSideSpacing", ctx->settings.defaultButtonGroupLabelSideSpacing);
	f32 elemHeightUnscaled = leftElem.normalState().height;
	f32 elemHeight = (elemHeightUnscaled + padding.y * 2.0f) * ctx->scale;

	f32 segmentWidths[256];
	f32 totalWidth = 0;

	for (u32 i = 0; i < count; i++)
	{
		auto& elem = i == 0 ? leftElem : i == count - 1 ? rightElem : middleElem;
		auto& state = elem.normalState();
		f32 textWidth = state.font->computeTextSize(labels[i]).width;
		f32 segWidth = textWidth + (state.border + labelSideSpacing) * 2.0f;
		segmentWidths[i] = segWidth;
		totalWidth += segWidth;
	}

	f32 availableWidth = ctx->layout.width / ctx->scale;
	f32 centeringOffset = 0;
	if (totalWidth < availableWidth)
	{
		centeringOffset = (availableWidth - totalWidth) / 2.0f;
	}

	// Handle transition from sameLine back to normal layout
	if (!ctx->sameLine.enabled && ctx->sameLine.wasEnabled)
	{
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
		ctx->sameLine.wasEnabled = false;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.lastLineWidth = 0;
	}

	if (!ctx->sameLine.enabled && !ctx->sameLine.wasEnabled)
	{
		ctx->sameLine.currentPosition = ctx->position;
		ctx->sameLine.lastLineWidth = 0;
	}

	f32 startX = ctx->position.x + centeringOffset * ctx->scale;
	f32 groupWidth = 0;

	bool changed = false;

	for (u32 i = 0; i < count; i++)
	{
		auto& elem = i == 0 ? leftElem : i == count - 1 ? rightElem : middleElem;

		char idBuf[256];
		snprintf(idBuf, sizeof(idBuf), "%s##btnGrp_%u", labels[i], i);
		ctx->setLabelAndId(idBuf);
		ctx->id = genIdFromPosition(labels[i]);

		ctx->widget.disabled = ctx->widget.nextDisabled || (ctx->disabledNesting > 0);
		ctx->widget.nextDisabled = false;
		ctx->widget.changeEnded = false;
		ctx->widget.hasNextWidth = false;
		ctx->widget.hasCustomWidth = false;
		ctx->sameLine.enabled = false;

		f32 segPixelWidth = segmentWidths[i] * ctx->scale;

		ctx->widget.rect.set(
			startX + groupWidth,
			ctx->position.y,
			segPixelWidth,
			elemHeight);

		mouseDownOnlyButtonBehavior();

		auto elemState = &elem.normalState();

		if (ctx->widget.disabled)
		{
			elemState = &elem.getState(WidgetStateType::Disabled);
		}
		else if (*currentIndex == i)
		{
			elemState = &elem.getState(WidgetStateType::Pressed);
		}
		else if (ctx->widget.hovered)
		{
			elemState = &elem.getState(WidgetStateType::Hovered);
		}

		if (ctx->widget.clicked && *currentIndex != i)
		{
			*currentIndex = i;
			ctx->widget.changeEnded = true;
			changed = true;
			forceRepaint();
		}

		ctx->renderer.cmdSetColor(tintApply(elemState->color, TintColorType::Body));

		Image* image = elemState->image;
		if (!image && ctx->widget.disabled)
		{
			image = elem.normalState().image;
		}

		ctx->renderer.cmdDrawImageBordered(image, elemState->border, ctx->widget.rect, ctx->scale);

		ctx->renderer.cmdSetColor(tintApply(elemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(elemState->font);
		ctx->renderer.cmdDrawTextInBox(
			labels[i],
			ctx->widget.rect,
			HAlignType::Center,
			VAlignType::Center,
			true);

		widgetSetFocusable();

		groupWidth += segPixelWidth;
	}

	ctx->position.x = ctx->sameLine.currentPosition.x;
	ctx->position.y += elemHeight + ctx->spacing * ctx->scale;
	ctx->sameLine.lastLineWidth = groupWidth;

	if (ctx->layout.type == LayoutType::ScrollView)
	{
		f32 rightEdge = startX + groupWidth;
		if (rightEdge > ctx->maxContentWidth)
			ctx->maxContentWidth = rightEdge;
	}

	return changed;
}

}
