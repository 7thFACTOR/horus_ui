#include "horus.h"
#include "theme.h"
#include "builtin_font_data.h"

namespace hui {

#define HUI_DEFAULT_STYLE(elId) theme->getElement(WidgetElementId::elId).styles["default"]

HTheme createBuiltinTheme(u32 atlasTextureSize)
{
	Theme* theme = new Theme(atlasTextureSize);

	// Load the embedded font
	Font* font = theme->createFontFromMemory("default", roboto_regular_data, roboto_regular_data_size, 14);

	Color colorBg = Color::fromU8(45, 45, 45);
	Color colorBody = Color::fromU8(60, 60, 60);
	Color colorHovered = Color::fromU8(75, 75, 75);
	Color colorPressed = Color::fromU8(90, 90, 90);
	Color colorFocused = Color::fromU8(70, 70, 70);
	Color colorDisabled = Color::fromU8(40, 40, 40);
	Color colorText = Color::fromU8(255, 255, 255);
	Color colorDisabledText = Color::fromU8(120, 120, 120);
	Color colorWhite = Color::fromU8(255, 255, 255);
	Color colorSelection = Color::fromU8(100, 100, 100);

	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		WidgetElementId id = (WidgetElementId)i;
		ThemeElement& elem = theme->getElement(id);

		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
		{
			WidgetStateType state = (WidgetStateType)j;
			ThemeElement::State& s = elem.styles["default"].states[j];

			s.image = theme->whiteImage;
			s.font = font;
			s.textColor = colorText;
			s.color = colorBody;

			if (state == WidgetStateType::Hovered) s.color = colorHovered;
			if (state == WidgetStateType::Pressed) s.color = colorPressed;
			if (state == WidgetStateType::Focused) s.color = colorFocused;
			if (state == WidgetStateType::Disabled)
			{
				s.color = colorDisabled;
				s.textColor = colorDisabledText;
			}
		}
	}

	auto setSize = [&](WidgetElementId id, f32 width, f32 height, u32 border = 0)
	{
		ThemeElement& elem = theme->getElement(id);
		auto& style = elem.styles["default"];
		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
		{
			style.states[j].width = width;
			style.states[j].height = height;
			style.states[j].border = border;
		}
	};

	// Specialized element tweaks
	setSize(WidgetElementId::ButtonBody, 0, 22, 3);
	setSize(WidgetElementId::ButtonGroupLeftBody, 0, 22, 4);
	setSize(WidgetElementId::ButtonGroupMiddleBody, 0, 22, 0);
	setSize(WidgetElementId::ButtonGroupRightBody, 0, 22, 4);
	setSize(WidgetElementId::ImageButtonBody, 0, 22, 3);
	setSize(WidgetElementId::TextInputBody, 0, 22, 3);
	setSize(WidgetElementId::SliderBody, 0, 16, 2);
	setSize(WidgetElementId::ComboSliderBody, 0, 20, 2);
	setSize(WidgetElementId::CheckBody, 22, 22, 4);
	setSize(WidgetElementId::RadioBody, 22, 22, 7);
	setSize(WidgetElementId::DropdownBody, 0, 22, 3);
	setSize(WidgetElementId::MenuBarBody, 0, 20, 2);
	setSize(WidgetElementId::MenuBarItem, 0, 20, 1);
	setSize(WidgetElementId::ProgressBack, 0, 20, 6);
	setSize(WidgetElementId::ProgressFill, 0, 19, 6);
	setSize(WidgetElementId::SelectableBody, 0, 20, 3);
	setSize(WidgetElementId::TabBodyActive, 100, 30, 5);
	setSize(WidgetElementId::TabBodyInactive, 100, 30, 5);
	setSize(WidgetElementId::TooltipBody, 0, 0, 4);
	setSize(WidgetElementId::PopupBody, 0, 0, 10);
	setSize(WidgetElementId::WindowBody, 0, 0, 2);

	HUI_DEFAULT_STYLE(WindowBody).states[(u32)WidgetStateType::Normal].color = colorBg;
	HUI_DEFAULT_STYLE(PopupBody).states[(u32)WidgetStateType::Normal].color = colorBg;
	HUI_DEFAULT_STYLE(TooltipBody).states[(u32)WidgetStateType::Normal].color = colorBg;

	HUI_DEFAULT_STYLE(TextInputBody).states[(u32)WidgetStateType::Normal].color = colorBg;
	HUI_DEFAULT_STYLE(TextInputBody).states[(u32)WidgetStateType::Normal].textColor = colorWhite;
	HUI_DEFAULT_STYLE(TextInputBody).states[(u32)WidgetStateType::Focused].textColor = colorWhite;

	HUI_DEFAULT_STYLE(TextInputSelection).states[(u32)WidgetStateType::Normal].color = colorSelection;
	HUI_DEFAULT_STYLE(TextInputCaret).states[(u32)WidgetStateType::Normal].color = colorWhite;
	HUI_DEFAULT_STYLE(TextInputCaret).states[(u32)WidgetStateType::Normal].width = 2.0f;
	
	HUI_DEFAULT_STYLE(CheckMark).states[(u32)WidgetStateType::Normal].color = colorWhite;
	HUI_DEFAULT_STYLE(RadioMark).states[(u32)WidgetStateType::Normal].color = colorWhite;

	theme->build();

	return (HTheme)theme;
}

#undef HUI_DEFAULT_STYLE

}
