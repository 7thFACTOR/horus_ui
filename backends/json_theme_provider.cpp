#include "json_theme_provider.h"
#include <json/json.h>
#include <json/reader.h>
#include <unordered_map>

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
		auto readBytes = HORUS_FILE->read(file, buffer, size);

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
	if (name == "imageButton") return WidgetType::ImageButton;
	if (name == "textInput") return WidgetType::TextInput;
	if (name == "slider") return WidgetType::Slider;
	if (name == "progress") return WidgetType::Progress;
	if (name == "image") return WidgetType::Image;
	if (name == "check") return WidgetType::Check;
	if (name == "radio") return WidgetType::Radio;
	if (name == "label") return WidgetType::Label;
	if (name == "panel") return WidgetType::Panel;
	if (name == "expandable") return WidgetType::Expandable;
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
	if (name == "messageBox") return WidgetType::MsgBox;
	if (name == "selectable") return WidgetType::Selectable;
	if (name == "box") return WidgetType::Box;
	if (name == "comboSlider") return WidgetType::ComboSlider;
	if (name == "rotarySlider") return WidgetType::RotarySlider;
	if (name == "colorPicker") return WidgetType::ColorPicker;
	if (name == "table") return WidgetType::Table;

	return WidgetType::None;
}

WidgetElementId getWidgetElementFromName(std::string name)
{
	if (name == "windowBody") return WidgetElementId::WindowBody;
	if (name == "buttonBody") return WidgetElementId::ButtonBody;
	if (name == "imageButtonBody") return WidgetElementId::ImageButtonBody;
	if (name == "checkBody") return WidgetElementId::CheckBody;
	if (name == "checkMark") return WidgetElementId::CheckMark;
	if (name == "radioBody") return WidgetElementId::RadioBody;
	if (name == "radioMark") return WidgetElementId::RadioMark;
	if (name == "lineBody") return WidgetElementId::LineBody;
	if (name == "labelBody") return WidgetElementId::LabelBody;
	if (name == "expandableBody") return WidgetElementId::ExpandableBody;
	if (name == "expandableCollapsedArrow") return WidgetElementId::ExpandableCollapsedArrow;
	if (name == "expandableExpandedArrow") return WidgetElementId::ExpandableExpandedArrow;
	if (name == "textInputBody") return WidgetElementId::TextInputBody;
	if (name == "textInputCaret") return WidgetElementId::TextInputCaret;
	if (name == "textInputSelection") return WidgetElementId::TextInputSelection;
	if (name == "textInputDefaultText") return WidgetElementId::TextInputDefaultText;
	if (name == "textInputFilterClearImage") return WidgetElementId::TextInputFilterClearImage;
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
	if (name == "windowHorizontalSplitter") return WidgetElementId::WindowHorizontalSplitter;
	if (name == "windowVerticalSplitter") return WidgetElementId::WindowVerticalSplitter;
	if (name == "windowDockGuideAsTab") return WidgetElementId::WindowDockGuideAsTab;
	if (name == "windowDockGuideVerticalSplit") return WidgetElementId::WindowDockGuideVerticalSplit;
	if (name == "windowDockGuideHorizontalSplit") return WidgetElementId::WindowDockGuideHorizontalSplit;
	if (name == "menuBarBody") return WidgetElementId::MenuBarBody;
	if (name == "menuBarItem") return WidgetElementId::MenuBarItem;
	if (name == "menuBody") return WidgetElementId::MenuBody;
	if (name == "menuItemSeparator") return WidgetElementId::MenuItemSeparator;
	if (name == "menuItemBody") return WidgetElementId::MenuItemBody;
	if (name == "menuItemShortcut") return WidgetElementId::MenuItemShortcut;
	if (name == "menuItemCheckMark") return WidgetElementId::MenuItemCheckMark;
	if (name == "menuItemNoCheckMark") return WidgetElementId::MenuItemNoCheckMark;
	if (name == "subMenuItemArrow") return WidgetElementId::SubMenuItemArrow;
	if (name == "errorImage") return WidgetElementId::MessageBoxImageError;
	if (name == "infoImage") return WidgetElementId::MessageBoxImageInfo;
	if (name == "questionImage") return WidgetElementId::MessageBoxImageQuestion;
	if (name == "warningImage") return WidgetElementId::MessageBoxImageWarning;
	if (name == "selectableBody") return WidgetElementId::SelectableBody;
	if (name == "boxBody") return WidgetElementId::BoxBody;
	if (name == "comboSliderBody") return WidgetElementId::ComboSliderBody;
	if (name == "comboSliderLeftArrow") return WidgetElementId::ComboSliderLeftArrow;
	if (name == "comboSliderRightArrow") return WidgetElementId::ComboSliderRightArrow;
	if (name == "comboSliderRangeBar") return WidgetElementId::ComboSliderRangeBar;
	if (name == "comboSliderVerticalLine") return WidgetElementId::ComboSliderVerticalLine;
	if (name == "rotarySliderBody") return WidgetElementId::RotarySliderBody;
	if (name == "rotarySliderMark") return WidgetElementId::RotarySliderMark;
	if (name == "rotarySliderValueDot") return WidgetElementId::RotarySliderValueDot;
	if (name == "colorPickerCheckers") return WidgetElementId::ColorPickerCheckers;
	if (name == "colorPickerBody") return WidgetElementId::ColorPickerBody;
	if (name == "tableBody") return WidgetElementId::TableBody;
	if (name == "tableHeaderBody") return WidgetElementId::TableHeaderBody;

	return WidgetElementId::Custom;
}

static std::string getPath(const std::string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : fname.substr(0, pos);
}

static Color getColorFromText(std::string colorText)
{
	if (colorText == "white") { return Color::white; }
	if (colorText == "black") { return Color::black; }
	if (colorText == "red") { return Color::red; }
	if (colorText == "darkRed") { return Color::darkRed; }
	if (colorText == "veryDarkRed") { return Color::veryDarkRed; }
	if (colorText == "green") { return Color::green; }
	if (colorText == "darkGreen") { return Color::darkGreen; }
	if (colorText == "veryDarkGreen") { return Color::veryDarkGreen; }
	if (colorText == "blue") { return Color::blue; }
	if (colorText == "darkBlue") { return Color::darkBlue; }
	if (colorText == "veryDarkBlue") { return Color::veryDarkBlue; }
	if (colorText == "yellow") { return Color::yellow; }
	if (colorText == "darkYellow") { return Color::darkYellow; }
	if (colorText == "veryDarkYellow") { return Color::veryDarkYellow; }
	if (colorText == "magenta") { return Color::magenta; }
	if (colorText == "cyan") { return Color::cyan; }
	if (colorText == "darkCyan") { return Color::darkCyan; }
	if (colorText == "veryDarkCyan") { return Color::veryDarkCyan; }
	if (colorText == "orange") { return Color::orange; }
	if (colorText == "darkOrange") { return Color::darkOrange; }
	if (colorText == "lightGray") { return Color::lightGray; }
	if (colorText == "gray") { return Color::gray; }
	if (colorText == "darkGray") { return Color::darkGray; }
	if (colorText == "sky") { return Color::sky; }
	if (colorText == "transparent") { return Color::transparent; }

	u32 r, g, b, a;
	sscanf(colorText.c_str(), "%d %d %d %d", &r, &g, &b, &a);

	return Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
}

Color getColorFromText(const char* colorText)
{
	return getColorFromText(std::string(colorText));
}

static u8 hexByte(const char* p)
{
	auto hex = [](char c) -> u8
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return 0;
		};

	return (hex(p[0]) << 4) | hex(p[1]);
}

Color colorFromHex(const char* hexText)
{
	Color out{ 1.f, 1.f, 1.f, 1.f };

	if (!hexText)
		return out;

	// Skip optional '#'
	if (hexText[0] == '#')
		hexText++;

	const size_t len = std::strlen(hexText);

	if (len != 6 && len != 8)
		return out;

	u8 r = hexByte(hexText + 0);
	u8 g = hexByte(hexText + 2);
	u8 b = hexByte(hexText + 4);
	u8 a = (len == 8) ? hexByte(hexText + 6) : 255;

	out.r = r / 255.0f;
	out.g = g / 255.0f;
	out.b = b / 255.0f;
	out.a = a / 255.0f;

	return out;
}

u32 intColorFromHex(const char* hexText)
{
	return colorFromHex(hexText).getRgba();
}

std::string colorToHex(const Color& color)
{
	auto clampToByte = [](float v) -> u8
		{
			v = std::clamp(v, 0.0f, 1.0f);
			return static_cast<u8>(v * 255.0f + 0.5f);
		};

	u8 r = clampToByte(color.r);
	u8 g = clampToByte(color.g);
	u8 b = clampToByte(color.b);
	u8 a = clampToByte(color.a);

	char buf[9];

	std::snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", r, g, b, a);

	return std::string(buf);
}

std::string intColorToHex(const u32 color)
{
	return colorToHex(Color(color));
}

Color hsvToRgb(const Color& hsv)
{
	f32 h = hsv.r;
	f32 s = hsv.g;
	f32 v = hsv.b;
	f32 r = 0;
	f32 g = 0;
	f32 b = 0;

	if (s <= 0.0f)
	{
		// Gray
		r = g = b = v;
		return Color(r, g, b, hsv.a);
	}

	h = std::fmod(h, 1.0f) * 6.0f;
	i32 i = (int)std::floor(h);
	f32 f = h - i;

	f32 p = v * (1.0f - s);
	f32 q = v * (1.0f - s * f);
	f32 t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	default: r = v; g = p; b = q; break;
	}

	return Color(r, g, b, hsv.a);
}

Color rgbToHsv(const Color& rgb)
{
	f32 r = rgb.r, g = rgb.g, b = rgb.b;
	f32 h = 0, s = 0, v = 0;

	f32 max = std::max(r, std::max(g, b));
	f32 min = std::min(r, std::min(g, b));
	f32 delta = max - min;

	v = max;

	if (max <= 0.0f)
	{
		// Black
		s = 0.0f;
		h = 0.0f;

		return Color(h, s, v, rgb.a);
	}

	s = delta / max;

	if (delta <= 0.0f)
	{
		// Gray
		h = 0.0f;
		return Color(h, s, v, rgb.a);
	}

	if (max == r)
		h = (g - b) / delta;
	else if (max == g)
		h = 2.0f + (b - r) / delta;
	else
		h = 4.0f + (r - g) / delta;

	h /= 6.0f;

	if (h < 0.0f)
		h += 1.0f;

	return Color(h, s, v, rgb.a);
}

Color hueToRgb(f32 h, f32 alpha)
{
	h = std::fmod(h, 1.0f);
	if (h < 0.0f) h += 1.0f;

	f32 r, g, b;

	f32 i = std::floor(h * 6.0f);
	f32 f = h * 6.0f - i;

	f32 q = 1.0f - f;
	f32 t = f;

	switch (i32(i) % 6)
	{
	case 0: r = 1; g = t; b = 0; break;
	case 1: r = q; g = 1; b = 0; break;
	case 2: r = 0; g = 1; b = t; break;
	case 3: r = 0; g = q; b = 1; break;
	case 4: r = t; g = 0; b = 1; break;
	default:r = 1; g = 0; b = q; break;
	}

	return { r, g, b, alpha };
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

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	bgColor = getColorFromText(color);
	txtColor = getColorFromText(textColor);

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

	bgColor = getColorFromText(color);
	txtColor = getColorFromText(textColor);

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
	HTheme theme = hui::createTheme(hui::getSettings().defaultAtlasSize);

	Json::Reader reader;
	Json::Value root;
	auto json = readTextFile(filename);
	bool ok = reader.parse(json, root);
	std::string themePath = getPath(filename) + "/";

	if (!ok)
	{
		if (errorTextBuffer)
		{
			strncpy(errorTextBuffer, reader.getFormatedErrorMessages().c_str(), std::min(errorTextBufferSize, reader.getFormatedErrorMessages().size()));
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