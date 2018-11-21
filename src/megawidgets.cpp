#include "horus.h"
#include "types.h"
#include "ui_context.h"
#include "ui_theme.h"
#include "util.h"

namespace hui
{
bool vecEditorInternal(f64& x, f64& y, f64& z, f64 scrollStep, bool useZ)
{
	static bool draggingValue;
	static Point lastMousePos;
	static u32 draggedWidgetId = 0;

	const int maxStrSize = 50;
	char strX[maxStrSize] = { 0 };
	char strY[maxStrSize] = { 0 };
	char strZ[maxStrSize] = { 0 };

	sprintf(strX, "%.8g", x);
	sprintf(strY, "%.8g", y);
	sprintf(strZ, "%.8g", z);

	f32 colWidthsPRS[] = { 13, -1, 13, -1, 13, -1 };

	hui::beginColumns(useZ ? 6 : 4, colWidthsPRS);

	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	auto editValue = [maxStrSize](
		char* axisName,
		char* axisImageName,
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
		hui::beginBox((draggingValue && (draggedWidgetId == (ctx->currentWidgetId + 2))) ? dragColor : normalColor, "axisBoxBody");
		u32 imageWidgetId = hui::getWidgetId();
		hui::image(elem->normalState().image, 13);
		bool imageHovered = hui::isHovered();
		bool imagePressed = hui::isPressed();
		hui::endBox();

		if (hui::isHovered() || imageHovered || draggingValue)
		{
			hui::setMouseCursor(hui::MouseCursorType::SizeWE);

			if (hui::isPressed() || imagePressed)
			{
				draggingValue = true;
				draggedWidgetId = ctx->currentWidgetId;
				lastMousePos = hui::getInputEvent().mouse.point;
				hui::setCapture(hui::getWindow());
			}
		}

		if (hui::getInputEvent().type == hui::InputEvent::Type::MouseUp)
		{
			draggingValue = false;
			draggedWidgetId = 0;
			hui::releaseCapture();
			changeEnded = true;
		}

		if (draggingValue
			&& draggedWidgetId == ctx->currentWidgetId)
		{
			value = atof(strAxis);
			f32 dx = hui::getInputEvent().mouse.point.x - lastMousePos.x;

			f32 unitPerPixel = scrollStep;

			value += dx * unitPerPixel;
			lastMousePos = hui::getInputEvent().mouse.point;
			hui::toString((f32)value, strAxis, maxStrSize);
			modified = true;
		}

		hui::nextColumn();
		modified = hui::textInput(strAxis, maxStrSize) || modified;

		if (isChangeEnded())
			changeEnded = true;

		if (modified)
		{
			value = atof(strAxis);
		}
	};

	editValue("X", "axisBoxXImage", strX, x, Color::veryDarkRed, Color::red, scrollStep, modified, changedEndedX);
	hui::nextColumn();
	editValue("Y", "axisBoxYImage", strY, y, Color::veryDarkGreen, Color::green, scrollStep, modified, changedEndedY);

	if (useZ)
	{
		hui::nextColumn();
		editValue("Z", "axisBoxZImage", strZ, z, Color::veryDarkCyan, Color::cyan, scrollStep, modified, changedEndedZ);
	}

	endColumns();
	ctx->widget.changeEnded = changedEndedX || changedEndedY || changedEndedZ;

	return modified;
}

bool vec3Editor(f64& x, f64& y, f64& z, f64 scrollStep)
{
	return vecEditorInternal(x, y, z, scrollStep, true);
}

bool vec3Editor(f32& x, f32& y, f32& z, f32 scrollStep)
{
	f64 xx = x, yy = y, zz = z;

	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, true);

	x = xx;
	y = yy;
	z = zz;

	return ret;
}

bool vec2Editor(f64& x, f64& y, f64 scrollStep)
{
	f64 zz;
	return vecEditorInternal(x, y, zz, scrollStep, false);
}

bool vec2Editor(f32& x, f32& y, f32 scrollStep)
{
	f64 xx = x, yy = y, zz;
	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, false);

	x = xx;
	y = yy;

	return ret;
}

bool objectRefEditor(Image targetIcon, Image clearIcon, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified)
{
	bool returnValue = false;
	bool changeEnded = false;
	pushPadding(0);
	f32 tgtRowIcons[] = { -1, 30, 20 };

	if (objectValueWasModified)
		*objectValueWasModified = false;

	beginColumns(3, tgtRowIcons);
	WidgetElementInfo targetElemInfo;

	hui::getThemeUserWidgetElementInfo("targetObjectBody", WidgetStateType::Normal, targetElemInfo);

	beginBox(
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
		pushTint(Color::yellow);

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
		popTint();

	endBox();

	if (getDragDropObjectType() == objectType)
		allowDragDrop();

	if (droppedOnWidget() && getDragDropObjectType() == objectType)
	{
		*outObject = getDragDropObject();
		endDragDrop();

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
		forceRepaint();
	}

	nextColumn();
	returnValue = iconButton(targetIcon, targetElemInfo.height);
	nextColumn();
	pushTint(Color::darkRed);

	if (iconButton(clearIcon, targetElemInfo.height))
	{
		*outObject = nullptr;

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
	}

	popTint();
	endColumns();
	popPadding();

	ctx->widget.changeEnded = changeEnded;

	return returnValue;
}

}