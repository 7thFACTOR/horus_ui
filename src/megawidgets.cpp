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

	hui::beginColumns(useZ ? 6 : 4, ctx->vecEditor.colWidthsPRS);

	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	auto editValue = [](
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
		//TODO: not working, since widget id is not incremental
		hui::beginBox((ctx->vecEditor.draggingValue && (ctx->vecEditor.draggedId == (ctx->id + 2))) ? dragColor : normalColor, "axisBoxBody");
		WidgetId imageWidgetId = hui::getWidgetId();
		hui::image(elem->normalState().image, 14);
		bool imageHovered = hui::isHovered();
		bool imagePressed = hui::isPressed();
		hui::endBox();

		if (hui::isHovered() || imageHovered || ctx->vecEditor.draggingValue)
		{
			hui::setMouseCursor(hui::MouseCursorType::SizeWE);

			if (hui::isPressed() || imagePressed)
			{
				ctx->vecEditor.draggingValue = true;
				ctx->vecEditor.draggedId = ctx->id;
				ctx->vecEditor.lastMousePos = hui::getInputEvent().mouse.point;
				hui::setCapture();
			}
		}

		if (hui::getInputEvent().type == hui::InputEvent::Type::MouseUp)
		{
			ctx->vecEditor.draggingValue = false;
			ctx->vecEditor.draggedId = 0;
			hui::releaseCapture();
			changeEnded = true;
		}

		if (ctx->vecEditor.draggingValue
			&& ctx->vecEditor.draggedId == ctx->id)
		{
			value = atof(strAxis);
			f32 dx = hui::getInputEvent().mouse.point.x - ctx->vecEditor.lastMousePos.x;
			f32 unitPerPixel = scrollStep;

			value += (f64)dx * unitPerPixel;
			ctx->vecEditor.lastMousePos = hui::getInputEvent().mouse.point;
			hui::toString((f32)value, strAxis, VectorEditorState::maxStrSize);
			modified = true;
		}

		hui::nextColumn();
		modified = hui::textInput(strAxis, VectorEditorState::maxStrSize) || modified;

		if (isChangeEnded())
			changeEnded = true;

		if (modified)
		{
			value = atof(strAxis);
		}
	};

	editValue("X", "axisBoxXImage", ctx->vecEditor.strX, x, Color::veryDarkRed, Color::red, scrollStep, modified, changedEndedX);
	hui::nextColumn();
	editValue("Y", "axisBoxYImage", ctx->vecEditor.strY, y, Color::veryDarkGreen, Color::green, scrollStep, modified, changedEndedY);

	if (useZ)
	{
		hui::nextColumn();
		editValue("Z", "axisBoxZImage", ctx->vecEditor.strZ, z, Color::veryDarkCyan, Color::cyan, scrollStep, modified, changedEndedZ);
	}

	endColumns();
	ctx->widget.changeEnded = changedEndedX || changedEndedY || changedEndedZ;

	return modified;
}

bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep)
{
	pushId(id);
	bool ret = vecEditorInternal(x, y, z, scrollStep, true);
	popId();

	return ret;
}

bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep)
{
	pushId(id);
	f64 xx = x, yy = y, zz = z;

	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, true);

	x = xx;
	y = yy;
	z = zz;

	popId();

	return ret;
}

bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep)
{
	pushId(id);
	
	f64 zz = 0;
	bool ret = vecEditorInternal(x, y, zz, scrollStep, false);

	return ret;
}

bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep)
{
	pushId(id);
	f64 xx = x, yy = y, zz;
	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, false);

	x = xx;
	y = yy;
	popId();

	return ret;
}

bool objectRefEditor(const char* id, HImage targetIcon, HImage clearIcon, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified)
{
	pushId(id);
	bool returnValue = false;
	bool changeEnded = false;

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

	ctx->widget.changeEnded = changeEnded;

	popId();

	return returnValue;
}

}