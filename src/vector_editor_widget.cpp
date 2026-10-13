#include <algorithm>
#include <float.h>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
static const char* vecEditorAxisImages[3] = { "axisBoxXImage", "axisBoxYImage", "axisBoxZImage" };
static const f32 vecEditorAxisIconHeight = 22;

static bool vecEditorInternal(f64& x, f64& y, f64& z, f64 scrollStep, bool useZ, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	WidgetId editorId = ctx->idStack.back();
	auto& editIndeterminate = ctx->vecEditor;

	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	char fmt[16];
	snprintf(fmt, sizeof(fmt), "%%.%ug", precision);

	f32 spacing = ctx->sameLine.spacing * ctx->scale;
	u32 axisCount = useZ ? 3 : 2;

	f32 consumedX = 0.0f;

	if (ctx->sameLine.enabled)
		consumedX = ctx->sameLine.wasEnabled
			? ctx->position.x - ctx->sameLine.currentPosition.x
			: ctx->sameLine.lastLineWidth + ctx->sameLine.nextSpacing * ctx->scale;

	f32 rowWidthPx = ctx->layout.width - std::max(consumedX, 0.0f);

	f32 iconsWidthPx = 0.0f;
	for (u32 i = 0; i < axisCount; i++)
		iconsWidthPx += ((Image*)ctx->theme->userElements[vecEditorAxisImages[i]]->normalState().image)->width * ctx->scale;

	f32 spacingsWidth = ((f32)axisCount * 2.0f - 1.0f) * spacing;

	f32 inputWidthPx = std::max((rowWidthPx - iconsWidthPx - spacingsWidth) / (f32)axisCount, 2.0f);
	f32 inputWidthUnscaled = inputWidthPx / ctx->scale;

	if (editIndeterminate.draggingValue
		&& hui::inputEventGet().type == hui::InputEvent::Type::MouseUp)
	{
		editIndeterminate.draggingValue = false;
		editIndeterminate.draggedId = 0;
		ctx->settings.services.setAbsoluteMousePosition(editIndeterminate.hiddenCursorPos);
		ctx->settings.services.showMouseCursor();
		hui::windowReleaseCapture();
		changedEndedX = changedEndedY = changedEndedZ = true;
	}

	for (u32 i = 0; i < axisCount; i++)
	{
		if (i > 0)
			hui::sameLine();

		const char* imgName = vecEditorAxisImages[i];
		const char* inputId = (i == 0) ? "axisEditX" : (i == 1) ? "axisEditY" : "axisEditZ";
		char* strAxis = (i == 0) ? editIndeterminate.strX : (i == 1) ? editIndeterminate.strY : editIndeterminate.strZ;
		f64* val = (i == 0) ? &x : (i == 1) ? &y : &z;
		bool* changeEnded = (i == 0) ? &changedEndedX : (i == 1) ? &changedEndedY : &changedEndedZ;

		bool indeterminateAxis = (i == 0) ? has(flags, VectorEditorFlags::IndeterminateX)
			: (i == 1) ? has(flags, VectorEditorFlags::IndeterminateY)
			: has(flags, VectorEditorFlags::IndeterminateZ);
		bool editingIndeterminate = (editIndeterminate.editingIndeterminateEditor == editorId) && editIndeterminate.editingIndeterminate[i];
		const char* hint = (indeterminateAxis && !editingIndeterminate) ? (indeterminateStr ? indeterminateStr : "Indeterminate") : nullptr;

		if (indeterminateAxis && !editingIndeterminate)
			strAxis[0] = 0;
		else if (!editingIndeterminate)
			snprintf(strAxis, VectorEditorState::maxStrSize, fmt, *val);

		auto elem = ctx->theme->userElements[imgName];

		hui::image(elem->normalState().image, vecEditorAxisIconHeight, hui::HAlignType::Left,
			hui::VAlignType::Center, hui::ImageFitType::None);
		WidgetId imageWidgetId = ctx->id;
		bool imageHovered = hui::widgetIsHovered();
		bool imagePressed = hui::widgetIsPressed();
		hui::sameLine();
		hui::widgetSetNextWidth(inputWidthUnscaled);
		auto tiFlags = has(flags, VectorEditorFlags::AutoSelectAll) ? TextInputFlags::AutoSelectAll : TextInputFlags::None;
		modified = hui::textInput(inputId, strAxis, VectorEditorState::maxStrSize, tiFlags, hint) || modified;

		if (indeterminateAxis && !editingIndeterminate && ctx->widget.focused)
		{
			editIndeterminate.editingIndeterminateEditor = editorId;
			editIndeterminate.editingIndeterminate[i] = true;
			editIndeterminate.indeterminateText[i][0] = 0;
		}

		if (!indeterminateAxis
			&& (imageHovered || (editIndeterminate.draggingValue && editIndeterminate.draggedId == imageWidgetId)))
		{
			hui::mouseCursorSetType(hui::MouseCursorType::SizeWE);

			if (imagePressed && !editIndeterminate.draggingValue)
			{
				editIndeterminate.draggingValue = true;
				editIndeterminate.draggedId = imageWidgetId;
				editIndeterminate.lastMousePos = ctx->mousePosition;
				editIndeterminate.hiddenCursorPos = ctx->settings.services.getAbsoluteMousePosition();
				ctx->settings.services.hideMouseCursor();
				ctx->widget.pressed = false;
				hui::windowSetCapture();
			}
		}

		if (!indeterminateAxis
			&& editIndeterminate.draggingValue
			&& editIndeterminate.draggedId == imageWidgetId)
		{
			*val = atof(strAxis);
			f32 dx = ctx->mousePosition.x - editIndeterminate.lastMousePos.x;
			f32 unitPerPixel = (f32)scrollStep;

			*val += (f64)dx * unitPerPixel;
			editIndeterminate.lastMousePos = ctx->mousePosition;

			if (ctx->lastHoveredNativeWindow)
			{
				auto wndPos = ctx->settings.services.getWindowPosition(ctx->lastHoveredNativeWindow);
				auto wndSize = ctx->settings.services.getWindowSize(ctx->lastHoveredNativeWindow);
				auto absPos = ctx->settings.services.getAbsoluteMousePosition();
				const f32 margin = 2.0f;

				if (absPos.x <= wndPos.x + margin || absPos.x >= wndPos.x + wndSize.x - margin)
				{
					Point centerScreen(wndPos.x + wndSize.x * 0.5f, absPos.y);
					ctx->settings.services.setAbsoluteMousePosition(centerScreen);
					Point warpDelta = centerScreen - absPos;
					editIndeterminate.lastMousePos += warpDelta;
					ctx->mousePosition += warpDelta;
				}
			}

			snprintf(strAxis, VectorEditorState::maxStrSize, fmt, *val);
			modified = true;
		}

		if (widgetIsChangeEnded())
			*changeEnded = true;

		if (indeterminateAxis && editingIndeterminate)
		{
			if (ctx->widget.focused && ctx->id == ctx->textInput.id && strcmp(strAxis, editIndeterminate.indeterminateText[i]) != 0)
			{
				strncpy(editIndeterminate.indeterminateText[i], strAxis, VectorEditorState::maxStrSize - 1);
				editIndeterminate.indeterminateText[i][VectorEditorState::maxStrSize - 1] = 0;
			}

			if (!ctx->widget.focused)
			{
				editIndeterminate.editingIndeterminate[i] = false;
				editIndeterminate.editingIndeterminateEditor = 0;

				if (editIndeterminate.indeterminateText[i][0] == 0)
					*val = FLT_MAX;
				else
					*val = atof(editIndeterminate.indeterminateText[i]);

				modified = true;
			}
		}
		else if (modified && !indeterminateAxis)
		{
			*val = atof(strAxis);
		}
	}

	ctx->widget.changeEnded = changedEndedX || changedEndedY || changedEndedZ;

	return modified;
}

bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	bool ret = vecEditorInternal(x, y, z, scrollStep, true, flags, precision, indeterminateStr);
	idPop();

	return ret;
}

bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	f64 xx = x, yy = y, zz = z;

	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, true, flags, precision, indeterminateStr);

	x = (f32)xx;
	y = (f32)yy;
	z = (f32)zz;

	idPop();

	return ret;
}

bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	
	f64 zz = 0;
	bool ret = vecEditorInternal(x, y, zz, scrollStep, false, flags, precision, indeterminateStr);
	idPop();

	return ret;
}

bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	f64 xx = x, yy = y, zz = 0;
	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, false, flags, precision, indeterminateStr);

	x = (f32)xx;
	y = (f32)yy;
	idPop();

	return ret;
}

}
