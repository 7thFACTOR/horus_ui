#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
bool vecEditorInternal(f64& x, f64& y, f64& z, f64 scrollStep, bool useZ)
{
	sprintf(ctx->vecEditor.strX, "%.8g", x);
	sprintf(ctx->vecEditor.strY, "%.8g", y);
	sprintf(ctx->vecEditor.strZ, "%.8g", z);

	//hui::beginColumns(useZ ? 6 : 4, ctx->vecEditor.colWidthsPRS);

	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	auto editValue = [](
		const char* axisName,
		const char* axisImageName,
		char* strAxis,
		f64& value,
		const Color& normalColor,
		const Color& dragColor,
		f64 scrollStep,
		bool& modified,
		bool& changeEnded)
	{
		auto elem = ctx->theme->userElements[axisImageName];

		// current widget + 2 since widget is computed in endBox and we have 1 image widget
		//TODO: not working, since widget id is not incremental
		hui::boxBeginUserElement("axisBody", (ctx->vecEditor.draggingValue && (ctx->vecEditor.draggedId == (ctx->id + 2))) ? dragColor : normalColor, "axisBoxBody");
		WidgetId imageWidgetId = hui::widgetIdGet();
		hui::image(elem->normalState().image, 14);
		bool imageHovered = hui::widgetIsHovered();
		bool imagePressed = hui::widgetIsPressed();
		hui::boxEnd();

		if (hui::widgetIsHovered() || imageHovered || ctx->vecEditor.draggingValue)
		{
			hui::cursorTypeSet(hui::MouseCursorType::SizeWE);

			if (hui::widgetIsPressed() || imagePressed)
			{
				ctx->vecEditor.draggingValue = true;
				ctx->vecEditor.draggedId = ctx->id;
				ctx->vecEditor.lastMousePos = hui::inputEventGet().mouse.point;
				hui::windowCaptureSet();
			}
		}

		if (hui::inputEventGet().type == hui::InputEvent::Type::MouseUp)
		{
			ctx->vecEditor.draggingValue = false;
			ctx->vecEditor.draggedId = 0;
			hui::windowCaptureRelease();
			changeEnded = true;
		}

		if (ctx->vecEditor.draggingValue
			&& ctx->vecEditor.draggedId == ctx->id)
		{
			value = atof(strAxis);
			f32 dx = hui::inputEventGet().mouse.point.x - ctx->vecEditor.lastMousePos.x;
			f32 unitPerPixel = (f32)scrollStep;

			value += (f64)dx * unitPerPixel;
			ctx->vecEditor.lastMousePos = hui::inputEventGet().mouse.point;
			hui::toStringF32((f32)value, strAxis, VectorEditorState::maxStrSize, 4);
			modified = true;
		}

		//hui::nextColumn();
		modified = hui::textInput("axisEdit", strAxis, VectorEditorState::maxStrSize) || modified;

		if (widgetIsChangeEnded())
			changeEnded = true;

		if (modified)
		{
			value = atof(strAxis);
		}
	};

	editValue("X", "axisBoxXImage", ctx->vecEditor.strX, x, Color::veryDarkRed, Color::red, scrollStep, modified, changedEndedX);
	//hui::nextColumn();
	editValue("Y", "axisBoxYImage", ctx->vecEditor.strY, y, Color::veryDarkGreen, Color::green, scrollStep, modified, changedEndedY);

	if (useZ)
	{
		//hui::nextColumn();
		editValue("Z", "axisBoxZImage", ctx->vecEditor.strZ, z, Color::veryDarkCyan, Color::cyan, scrollStep, modified, changedEndedZ);
	}

	//endColumns();
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

	hui::themeUserWidgetElementInfoGet("targetObjectBody", WidgetStateType::Normal, targetElemInfo);

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

	if (dragDropObjectTypeGet() == objectType)
		dragDropAllow();

	if (widgetDroppedOn() && dragDropObjectTypeGet() == objectType)
	{
		*outObject = dragDropObjectGet();
		dragDropEnd();

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
		forceRepaint();
	}

	//nextColumn();
	returnValue = buttonImage(targetImg, targetElemInfo.height, targetElemInfo.height);
	//nextColumn();
	tintPush(Color::darkRed);

	if (buttonImage(clearImg, targetElemInfo.height, targetElemInfo.height))
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