#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
bool vecEditorInternal(f64& x, f64& y, f64& z, f64 scrollStep, bool useZ)
{
	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	f32 spacing = ctx->sameLine.spacing * ctx->scale;
	u32 axisCount = useZ ? 3 : 2;

	auto imgElem = ctx->theme->userElements["axisBoxXImage"];
	f32 imgWidthPx = ((Image*)imgElem->normalState().image)->width * ctx->scale;

	f32 totalWidth = (f32)axisCount * (imgWidthPx + spacing) + spacing;
	f32 inputWidthPx = (ctx->layout.width - totalWidth) / (f32)axisCount;
	f32 inputWidthUnscaled = inputWidthPx / ctx->scale;

	// Handle MouseUp outside the per-axis loop so it only fires once
	// and doesn't interfere with the wrong axis iteration
	if (ctx->vecEditor.draggingValue
		&& hui::inputEventGet().type == hui::InputEvent::Type::MouseUp)
	{
		ctx->vecEditor.draggingValue = false;
		ctx->vecEditor.draggedId = 0;
		hui::windowReleaseCapture();
		changedEndedX = changedEndedY = changedEndedZ = true;
	}

	for (u32 i = 0; i < axisCount; i++)
	{
		if (i > 0)
			hui::sameLine();

		const char* imgName = (i == 0) ? "axisBoxXImage" : (i == 1) ? "axisBoxYImage" : "axisBoxZImage";
		const char* inputId = (i == 0) ? "axisEditX" : (i == 1) ? "axisEditY" : "axisEditZ";
		char* strAxis = (i == 0) ? ctx->vecEditor.strX : (i == 1) ? ctx->vecEditor.strY : ctx->vecEditor.strZ;
		f64* val = (i == 0) ? &x : (i == 1) ? &y : &z;
		bool* changeEnded = (i == 0) ? &changedEndedX : (i == 1) ? &changedEndedY : &changedEndedZ;

		sprintf(strAxis, "%.8g", *val);
		auto elem = ctx->theme->userElements[imgName];
		hui::image(elem->normalState().image, 22, hui::HAlignType::Left);
		WidgetId imageWidgetId = ctx->id;
		bool imageHovered = hui::widgetIsHovered();
		bool imagePressed = hui::widgetIsPressed();
		hui::sameLine();
		hui::widgetSetNextWidth(inputWidthUnscaled);
		modified = hui::textInput(inputId, strAxis, VectorEditorState::maxStrSize) || modified;

		if (imageHovered || (ctx->vecEditor.draggingValue && ctx->vecEditor.draggedId == imageWidgetId))
		{
			hui::mouseCursorSetType(hui::MouseCursorType::SizeWE);

			if (imagePressed && !ctx->vecEditor.draggingValue)
			{
				ctx->vecEditor.draggingValue = true;
				ctx->vecEditor.draggedId = imageWidgetId;
				ctx->vecEditor.lastMousePos = ctx->mousePosition;
				ctx->widget.pressed = false;
				hui::windowSetCapture();
			}
		}

		if (ctx->vecEditor.draggingValue
			&& ctx->vecEditor.draggedId == imageWidgetId)
		{
			*val = atof(strAxis);
			f32 dx = ctx->mousePosition.x - ctx->vecEditor.lastMousePos.x;
			f32 unitPerPixel = (f32)scrollStep;

			*val += (f64)dx * unitPerPixel;
			ctx->vecEditor.lastMousePos = ctx->mousePosition;
			sprintf(strAxis, "%.8g", *val);
			modified = true;
		}

		if (widgetIsChangeEnded())
			*changeEnded = true;

		if (modified)
		{
			*val = atof(strAxis);
		}
	}

	ctx->widget.changeEnded = changedEndedX || changedEndedY || changedEndedZ;

	return modified;
}

bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep)
{
	idPush(id);
	bool ret = vecEditorInternal(x, y, z, scrollStep, true);
	idPop();

	return ret;
}

bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep)
{
	idPush(id);
	f64 xx = x, yy = y, zz = z;

	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, true);

	x = (f32)xx;
	y = (f32)yy;
	z = (f32)zz;

	idPop();

	return ret;
}

bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep)
{
	idPush(id);
	
	f64 zz = 0;
	bool ret = vecEditorInternal(x, y, zz, scrollStep, false);
	idPop();

	return ret;
}

bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep)
{
	idPush(id);
	f64 xx = x, yy = y, zz = 0;
	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, false);

	x = (f32)xx;
	y = (f32)yy;
	idPop();

	return ret;
}

bool objectRefEditor(const char* id, HImage targetImg, HImage clearImg, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified)
{
	idPush(id);
	bool returnValue = false;
	bool changeEnded = false;

	f32 tgtRowImgs[] = { -1, 30, 20 };

	if (objectValueWasModified)
		*objectValueWasModified = false;

	//beginColumns(3, tgtRowImgs);
	WidgetElementInfo targetElemInfo;

	hui::themeGetUserWidgetElementInfo("targetObjectBody", WidgetStateType::Normal, targetElemInfo);

	boxBegin(
		"targetObjectBody",
		Color::white,
		WidgetElementId::TextInputBody,
		WidgetStateType::Normal,
		targetElemInfo.height);

	bool noVal = true;

	if (*outObject)
	{
		noVal = false;
	}

	if (!noVal)
		tintPush(Color::yellow);

	std::string str;

	if (noVal)
	{
		str += "None (";
		str += objectTypeName;
		str += ")";
		label(str.c_str());
	}
	else
	{
		label(valueAsString);
	}

	if (!noVal)
		tintPop();

	boxEnd();

	if (dragDropGetObjectType() == objectType)
		dragDropAllow();

	if (dragDropDroppedOnWidget() && dragDropGetObjectType() == objectType)
	{
		*outObject = dragDropGetObject();
		dragDropEnd();

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
		forceRepaint();
	}

	//nextColumn();
	returnValue = imageButton(targetImg, targetElemInfo.height, targetElemInfo.height);
	//nextColumn();
	tintPush(Color::darkRed);

	if (imageButton(clearImg, targetElemInfo.height, targetElemInfo.height))
	{
		*outObject = nullptr;

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
	}

	tintPop();
	//endColumns();

	ctx->widget.changeEnded = changeEnded;

	idPop();

	return returnValue;
}

}