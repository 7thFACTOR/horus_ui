#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool link(const char* label, HAlignType horizontalAlign)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::LinkBody);
	f32 height = 0;
	auto bodyElemState = &bodyElem.normalState();

	if (widgetGetDisabled())
	{
		bodyElemState = &bodyElem.disabledState();
	}

	ctx->setLabelAndId(label);
	auto fsize = ctx->renderer.computeSizeOrDrawText(ctx->widgetLabel.c_str(), Rect(0, 0, ctx->layout.width, FLT_MAX), HAlignType::Left, VAlignType::Top, false, bodyElemState->font, true);
	height = (bodyElemState->height > fsize.height ? bodyElemState->height : fsize.height);

	if (!ctx->widget.hasNextWidth)
	{
		if (horizontalAlign == HAlignType::Left)
		{
			ctx->widget.customWidth = fsize.width;
			ctx->widget.hasCustomWidth = true;
		}
	}

	addWidget(height);
	buttonBehavior();

	if (ctx->widget.hoveredId == ctx->id)
	{
		ctx->widget.hoveredType = WidgetType::Link;
	}

	if (ctx->widget.hovered && !ctx->widget.disabled)
	{
		mouseCursorSetType(MouseCursorType::HandPointing);
	}

	if (ctx->widget.visible)
	{
		auto elemState = bodyElemState;

		if (ctx->widget.hovered && !ctx->widget.disabled)
		{
			elemState = &bodyElem.getState(WidgetStateType::Hovered);
		}

		Rect textRc = {
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height
		};

		Color finalColor = tintApply(elemState->textColor, TintColorType::Text);
		ctx->renderer.cmdSetColor(finalColor);
		ctx->renderer.cmdSetFont(bodyElemState->font);
		ctx->renderer.cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			textRc,
			horizontalAlign,
			VAlignType::Center, true);

		// draw underline
		LineStyle underlineStyle;
		underlineStyle.color = finalColor;
		underlineStyle.width = 1.0f;
		ctx->renderer.cmdSetLineStyle(underlineStyle);

		f32 underlineY = textRc.y + textRc.height - 2.0f * ctx->scale;
		f32 underlineStartX = textRc.x;
		f32 underlineEndX = textRc.x + fsize.width;

		ctx->renderer.cmdDrawLine(Point(underlineStartX, underlineY), Point(underlineEndX, underlineY));
	}

	return ctx->widget.clicked;
}

}
