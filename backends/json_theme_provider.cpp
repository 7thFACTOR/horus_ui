#include "json_theme_provider.h"
#include "json/json.h"
#include "json/reader.h"
#include "horus_interfaces.h"
#include <assert.h>

namespace hui
{
static std::string readTextFile(const char* path)
{
	auto file = HORUS_FILE->open(path, "rb");

	if (!file)
		return std::string("");

	HORUS_FILE->seek(file, FileSeekMode::End, 0);
	auto size = HORUS_FILE->tell(file);
	std::string text;

	if (size != -1)
	{
		HORUS_FILE->seek(file, FileSeekMode::Set, 0);

		char* buffer = new char[size + 1];
		buffer[size] = 0;
		auto readBytes = HORUS_FILE->read(file, buffer, size, size);

		if (readBytes == size)
			text = buffer;

		delete[] buffer;
	}

	HORUS_FILE->close(file);

	return text;
}

WidgetType getWidgetTypeFromName(std::string name)
{
	if (name == "window") return WidgetType::Window;
	if (name == "tooltip") return WidgetType::Tooltip;
	if (name == "button") return WidgetType::Button;
	if (name == "iconButton") return WidgetType::IconButton;
	if (name == "textInput") return WidgetType::TextInput;
	if (name == "slider") return WidgetType::Slider;
	if (name == "progress") return WidgetType::Progress;
	if (name == "image") return WidgetType::Image;
	if (name == "check") return WidgetType::Check;
	if (name == "radio") return WidgetType::Radio;
	if (name == "label") return WidgetType::Label;
	if (name == "panel") return WidgetType::Panel;
	if (name == "popup") return WidgetType::Popup;
	if (name == "dropdown") return WidgetType::Dropdown;
	if (name == "list") return WidgetType::List;
	if (name == "resizeGrip") return WidgetType::ResizeGrip;
	if (name == "line") return WidgetType::Line;
	if (name == "space") return WidgetType::Space;
	if (name == "scrollView") return WidgetType::ScrollView;
	if (name == "menuBar") return WidgetType::MenuBar;
	if (name == "menu") return WidgetType::Menu;
	if (name == "tabGroup") return WidgetType::TabGroup;
	if (name == "tab") return WidgetType::Tab;
	if (name == "viewport") return WidgetType::Viewport;
	if (name == "viewPane") return WidgetType::ViewPane;
	if (name == "messageBox") return WidgetType::MsgBox;
	if (name == "selectable") return WidgetType::Selectable;
	if (name == "box") return WidgetType::Box;
	if (name == "toolbar") return WidgetType::Toolbar;
	if (name == "toolbarButton") return WidgetType::ToolbarButton;
	if (name == "toolbarSeparator") return WidgetType::ToolbarSeparator;
	if (name == "columnsHeader") return WidgetType::ColumnsHeader;
	if (name == "comboSlider") return WidgetType::ComboSlider;
	if (name == "rotarySlider") return WidgetType::RotarySlider;

	return WidgetType::None;
}

WidgetElementId getWidgetElementFromName(std::string name)
{
	if (name == "windowBody") return WidgetElementId::WindowBody;
	if (name == "buttonBody") return WidgetElementId::ButtonBody;
	if (name == "checkBody") return WidgetElementId::CheckBody;
	if (name == "checkMark") return WidgetElementId::CheckMark;
	if (name == "radioBody") return WidgetElementId::RadioBody;
	if (name == "radioMark") return WidgetElementId::RadioMark;
	if (name == "lineBody") return WidgetElementId::LineBody;
	if (name == "labelBody") return WidgetElementId::LabelBody;
	if (name == "panelBody") return WidgetElementId::PanelBody;
	if (name == "panelCollapsedArrow") return WidgetElementId::PanelCollapsedArrow;
	if (name == "panelExpandedArrow") return WidgetElementId::PanelExpandedArrow;
	if (name == "textInputBody") return WidgetElementId::TextInputBody;
	if (name == "textInputCaret") return WidgetElementId::TextInputCaret;
	if (name == "textInputSelection") return WidgetElementId::TextInputSelection;
	if (name == "textInputDefaultText") return WidgetElementId::TextInputDefaultText;
	if (name == "sliderBody") return WidgetElementId::SliderBody;
	if (name == "sliderBodyFilled") return WidgetElementId::SliderBodyFilled;
	if (name == "sliderKnob") return WidgetElementId::SliderKnob;
	if (name == "progressBack") return WidgetElementId::ProgressBack;
	if (name == "progressFill") return WidgetElementId::ProgressFill;
	if (name == "tooltipBody") return WidgetElementId::TooltipBody;
	if (name == "popupBody") return WidgetElementId::PopupBody;
	if (name == "popupBehind") return WidgetElementId::PopupBehind;
	if (name == "dropdownBody") return WidgetElementId::DropdownBody;
	if (name == "dropdownArrow") return WidgetElementId::DropdownArrow;
	if (name == "scrollViewBody") return WidgetElementId::ScrollViewBody;
	if (name == "scrollViewScrollBar") return WidgetElementId::ScrollViewScrollBar;
	if (name == "scrollViewScrollThumb") return WidgetElementId::ScrollViewScrollThumb;
	if (name == "tabGroupBody") return WidgetElementId::TabGroupBody;
	if (name == "tabBodyActive") return WidgetElementId::TabBodyActive;
	if (name == "tabBodyInactive") return WidgetElementId::TabBodyInactive;
	if (name == "viewPaneDockRect") return WidgetElementId::ViewPaneDockRect;
	if (name == "viewPaneDockDialRect") return WidgetElementId::ViewPaneDockDialRect;
	if (name == "menuBarBody") return WidgetElementId::MenuBarBody;
	if (name == "menuBarItem") return WidgetElementId::MenuBarItem;
	if (name == "menuBody") return WidgetElementId::MenuBody;
	if (name == "menuItemSeparator") return WidgetElementId::MenuItemSeparator;
	if (name == "menuItemBody") return WidgetElementId::MenuItemBody;
	if (name == "menuItemShortcut") return WidgetElementId::MenuItemShortcut;
	if (name == "menuItemCheckMark") return WidgetElementId::MenuItemCheckMark;
	if (name == "menuItemNoCheckMark") return WidgetElementId::MenuItemNoCheckMark;
	if (name == "subMenuItemArrow") return WidgetElementId::SubMenuItemArrow;
	if (name == "errorIcon") return WidgetElementId::MessageBoxIconError;
	if (name == "infoIcon") return WidgetElementId::MessageBoxIconInfo;
	if (name == "questionIcon") return WidgetElementId::MessageBoxIconQuestion;
	if (name == "warningIcon") return WidgetElementId::MessageBoxIconWarning;
	if (name == "selectableBody") return WidgetElementId::SelectableBody;
	if (name == "boxBody") return WidgetElementId::BoxBody;
	if (name == "toolbarBody") return WidgetElementId::ToolbarBody;
	if (name == "toolbarButtonBody") return WidgetElementId::ToolbarButtonBody;
	if (name == "toolbarSeparatorVerticalBody") return WidgetElementId::ToolbarSeparatorVerticalBody;
	if (name == "toolbarSeparatorHorizontalBody") return WidgetElementId::ToolbarSeparatorHorizontalBody;
	if (name == "columnsHeaderBody") return WidgetElementId::ColumnsHeaderBody;
	if (name == "comboSliderBody") return WidgetElementId::ComboSliderBody;
	if (name == "comboSliderLeftArrow") return WidgetElementId::ComboSliderLeftArrow;
	if (name == "comboSliderRightArrow") return WidgetElementId::ComboSliderRightArrow;
	if (name == "comboSliderRangeBar") return WidgetElementId::ComboSliderRangeBar;
	if (name == "rotarySliderBody") return WidgetElementId::RotarySliderBody;
	if (name == "rotarySliderMark") return WidgetElementId::RotarySliderMark;
	if (name == "rotarySliderValueDot") return WidgetElementId::RotarySliderValueDot;

	return WidgetElementId::Custom;
}

std::string getPath(const std::string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : fname.substr(0, pos);
}

bool getColorFromText(std::string colorText, Color& color)
{
	if (colorText == "white") { color = Color::white; return true; }
	if (colorText == "black") { color = Color::black; return true; }
	if (colorText == "red") { color = Color::red; return true; }
	if (colorText == "darkRed") { color = Color::darkRed; return true; }
	if (colorText == "veryDarkRed") { color = Color::veryDarkRed; return true; }
	if (colorText == "green") { color = Color::green; return true; }
	if (colorText == "darkGreen") { color = Color::darkGreen; return true; }
	if (colorText == "veryDarkGreen") { color = Color::veryDarkGreen; return true; }
	if (colorText == "blue") { color = Color::blue; return true; }
	if (colorText == "yellow") { color = Color::yellow; return true; }
	if (colorText == "magenta") { color = Color::magenta; return true; }
	if (colorText == "cyan") { color = Color::cyan; return true; }
	if (colorText == "darkCyan") { color = Color::darkCyan; return true; }
	if (colorText == "veryDarkCyan") { color = Color::veryDarkCyan; return true; }
	if (colorText == "orange") { color = Color::orange; return true; }
	if (colorText == "darkOrange") { color = Color::darkOrange; return true; }
	if (colorText == "lightGray") { color = Color::lightGray; return true; }
	if (colorText == "gray") { color = Color::gray; return true; }
	if (colorText == "darkGray") { color = Color::darkGray; return true; }
	if (colorText == "sky") { color = Color::sky; return true; }

	return false;
}

bool getColorFromText(const char* colorText, Color& color)
{
	return getColorFromText(std::string(colorText), color);
}

void setThemeElement(
	HTheme theme,
	const std::string& themePath,
	const char* styleName,
	WidgetType widgetType,
	WidgetElementId elemId,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	HImage image = hui::getThemeImage(theme, imageFilename.c_str());

	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (!image)
	{
		auto imageData = hui::loadImageData(imageFilename.c_str());
		image = addThemeImage(theme, imageData);
		deleteImageData(imageData);
		hui::setThemeImage(theme, imageFilename.c_str(), image);
	}

	auto font = hui::getThemeFont(theme, fontName.c_str());
	assert(font);

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	if (!getColorFromText(color, bgColor))
	{
		sscanf(color.c_str(), "%d %d %d %d", &r, &g, &b, &a);
		bgColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	if (!getColorFromText(textColor, txtColor))
	{
		sscanf(textColor.c_str(), "%d %d %d %d", &r, &g, &b, &a);
		txtColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	WidgetElementInfo elemInfo;

	elemInfo.image = image;
	elemInfo.border = border;
	elemInfo.color = bgColor;
	elemInfo.textColor = txtColor;
	elemInfo.font = font;
	elemInfo.width = width;
	elemInfo.height = height;

	hui::setThemeWidgetElement(theme, elemId, widgetStateType, elemInfo, styleName);
}

void setUserElement(
	HTheme theme,
	const std::string& themePath,
	const char* styleName,
	const std::string& widgetName,
	const std::string& elemName,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	HImage image = hui::getThemeImage(theme, imageFilename.c_str());
	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (!image)
	{
		auto imageData = loadImageData(imageFilename.c_str());
		image = addThemeImage(theme, imageData);
		deleteImageData(imageData);
		hui::setThemeImage(theme, imageFilename.c_str(), image);
	}

	auto font = hui::getThemeFont(theme, fontName.c_str());

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	if (!getColorFromText(color, bgColor))
	{
		sscanf(color.c_str(), "%d %d %d %d", &r, &g, &b, &a);
		bgColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	if (!getColorFromText(textColor, txtColor))
	{
		sscanf(textColor.c_str(), "%d %d %d %d", &r, &g, &b, &a);
		txtColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	WidgetElementInfo elemInfo;

	elemInfo.image = image;
	elemInfo.border = border;
	elemInfo.color = bgColor;
	elemInfo.textColor = txtColor;
	elemInfo.font = font;
	elemInfo.width = width,
	elemInfo.height = height;

	setThemeUserWidgetElement(theme, elemName.c_str(), widgetStateType, elemInfo, styleName);
}

WidgetStateType widgetStateFromText(const std::string& stateName)
{
	if (stateName == "normal")
		return WidgetStateType::Normal;
	else if (stateName == "focused")
		return WidgetStateType::Focused;
	else if (stateName == "pressed")
		return WidgetStateType::Pressed;
	else if (stateName == "hovered")
		return  WidgetStateType::Hovered;
	else if (stateName == "disabled")
		return  WidgetStateType::Disabled;

	return WidgetStateType::Unknown;
}

HTheme loadThemeFromJson(const char* filename, char* errorTextBuffer, size_t errorTextBufferSize)
{
	HTheme theme = hui::createTheme(hui::getContextSettings().defaultAtlasSize);

	Json::Reader reader;
	Json::Value root;
	auto json = readTextFile(filename);
	bool ok = reader.parse(json, root);
	std::string themePath = getPath(filename) + "/";

	if (!ok)
	{
		if (errorTextBuffer)
		{
			strncpy(errorTextBuffer, reader.getFormatedErrorMessages().c_str(), )
		}

		deleteTheme(theme);
		return 0;
	}

	Json::Value fonts = root.get("fonts", Json::Value());
	auto fontNames = fonts.getMemberNames();

	for (size_t i = 0; i < fontNames.size(); i++)
	{
		auto name = fontNames[i];
		auto fnt = fonts.get(name.c_str(), Json::Value());

		std::string fontFilename = fnt.get("file", "").asString();

		if (fontFilename.find_first_of(':') == std::string::npos)
		{
			fontFilename = themePath + fontFilename;
		}

		createThemeFont(theme, name.c_str(), fontFilename.c_str(), fnt.get("size", 0).asInt());
	}

	Json::Value settings = root.get("settings", Json::Value());
	auto settingNames = settings.getMemberNames();

	for (size_t i = 0; i < settingNames.size(); i++)
	{
		auto name = settingNames[i];
		auto val = settings.get(name.c_str(), Json::Value());
		hui::setThemeUserSetting(theme, name.c_str(), val.asCString());
	}

	Json::Value widgets = root.get("widgets", Json::Value());
	auto widgetNames = widgets.getMemberNames();

	for (size_t i = 0; i < widgetNames.size(); i++)
	{
		auto widgetName = widgetNames[i];
		auto widget = widgets.get(widgetName.c_str(), Json::Value());
		WidgetType widgetType = getWidgetTypeFromName(widgetName);
		auto widgetMemberNames = widget.getMemberNames();
		auto styles = widget.get("styles", Json::Value());

		auto readElements = [theme, themePath, widgetType](const std::string& styleName, Json::Value& parentElem)
		{
			auto elementNames = parentElem.getMemberNames();
			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto elemType = getWidgetElementFromName(elementNames[l]);
				auto elem = parentElem.get(elementNames[l], Json::Value());
				auto width = elem.get("width", 0).asInt();
				auto height = elem.get("height", 0).asInt();
				auto elemStates = elem.getMemberNames();

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = widgetStateFromText(stateName);

					if (widgetStateType != WidgetStateType::Unknown)
						setThemeElement(theme, themePath, styleName.c_str(), widgetType, elemType, widgetStateType, elemState, width, height);
					else
					{
						hui::setThemeWidgetElementParameter(theme, elemType, styleName.c_str(), stateName.c_str(), elemState.asString().c_str());
					}
				}
			}
		};

		auto readUserElements = [theme, themePath, widgetName](const std::string& styleName, Json::Value& parentElem)
		{
			auto elementNames = parentElem.getMemberNames();

			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto& elementName = elementNames[l];
				auto elem = parentElem.get(elementName, Json::Value());
				auto width = elem.get("width", 0).asInt();
				auto height = elem.get("height", 0).asInt();
				auto elemStates = elem.getMemberNames();

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = widgetStateFromText(stateName);

					if (widgetStateType != WidgetStateType::Unknown)
					{
						setUserElement(theme, themePath, styleName.c_str(), widgetName, elementName, widgetStateType, elemState, width, height);
					}
					else
					{
						hui::setThemeUserWidgetElementParameter(theme, elementName.c_str(), styleName.c_str(), stateName.c_str(), elemState.asString().c_str());
					}
				}
			}
		};

		if (styles.isObject())
		{
			auto styleNames = styles.getMemberNames();

			// read all styles
			for (size_t k = 0; k < styleNames.size(); k++)
			{
				auto styleName = styleNames[k];
				auto styleElem = styles.get(styleName, Json::Value());

				if (widgetType != WidgetType::None)
					readElements(styleName, styleElem);
				else
					readUserElements(styleName, styleElem);
			}
		}
		else
		{
			if (widgetType != WidgetType::None)
				readElements("default", widget);
			else
				readUserElements("default", widget);
		}
	}

	buildTheme(theme);

	return theme;
}

}