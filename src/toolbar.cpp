#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
void beginToolbar(u32 itemCount, const f32 preferredWidths[], const f32 minWidths[], const f32 maxWidths[])
{
	beginColumns(itemCount, preferredWidths, minWidths, maxWidths);
}

void endToolbar()
{
}

bool toolbarButton(Image normalIcon, Image disabledIcon, bool down)
{
	auto el = &ctx->theme->getElement(WidgetElementId::ToolbarButtonBody);

	bool ret = iconButtonInternal(
		normalIcon, disabledIcon,
		el->normalState().width,
		el->normalState().height,
		down,
		el, false);

	nextColumn();
	return ret;
}

bool toolbarDropdown(const char* label, Image normalIcon, Image disabledIcon)
{
	return false;
}

void toolbarSeparator()
{

}

void toolbarGap()
{

}

bool toolbarTextInputFilter(char* outText, u32 maxOutTextSize, u32& filterIndex, const char** filterNames, u32 filterNameCount)
{
	return false;
}

bool toolbarTextInput(char* outText, u32 maxOutTextSize, const char* hint, Image icon)
{
	return textInput(outText, maxOutTextSize, TextInputValueMode::Any, hint, icon);
}

}