#include "json_theme_loader.h"
#include <json/json.h>
#include <json/reader.h>
#include <unordered_map>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace hui
{
bool loadPngImage(const char* path, ImageData& outImage)
{
	i32 width = 0;
	i32 height = 0;
	i32 comp = 0;
	stbi_uc* imgFileData = nullptr;
	HFile file = getSettings().services.open(path, "rb");
	u64 fsize = 0;

	if (file)
	{
		getSettings().services.seek(file, FileSeekMode::End, 0);
		fsize = getSettings().services.tell(file);
		getSettings().services.seek(file, FileSeekMode::Set, 0);
		imgFileData = new stbi_uc[fsize];
		auto readSize = getSettings().services.read(file, imgFileData, fsize);

		if (fsize != readSize)
		{
			getSettings().services.close(file);

			return false;
		}
	}

	getSettings().services.close(file);

	stbi_uc* data = stbi_load_from_memory(imgFileData, fsize, &width, &height, &comp, 4);

	outImage.pixels = new Rgba32[width * height];
	memcpy(outImage.pixels, (Rgba32*)data, sizeof(Rgba32) * width * height);
	outImage.width = width;
	outImage.height = height;

	bool result = !(!data || !width || !height || !comp);

	if (data)
	{
		stbi_image_free(data);
	}

	delete[] imgFileData;

	return result;
}

bool savePngImage(const char* path, const ImageData& image)
{
	auto write_func = [](void* context, void* data, int size)
	{
		const char* path = (const char*)context;
		HFile file = getSettings().services.open(path, "wb");

		if (!file)
			return;

		getSettings().services.write(file, data, size);
		getSettings().services.close(file);
	};

	return 0 != stbi_write_png_to_func(write_func, (void*)path, image.width, image.height, 32 / 8, image.pixels, 0);
}

void deleteImageData(ImageData& image)
{
	delete [] image.pixels;
	image.pixels = nullptr;
	image.width = 0;
	image.height = 0;
}

static std::string readTextFile(const char* path)
{
	auto file = getSettings().services.open(path, "rb");

	if (!file)
		return std::string("");

	getSettings().services.seek(file, FileSeekMode::End, 0);
	auto size = getSettings().services.tell(file);
	std::string text;

	if (size != -1)
	{
		getSettings().services.seek(file, FileSeekMode::Set, 0);

		char* buffer = new char[size + 1];
		memset(buffer, 0, size + 1);
		auto readBytes = getSettings().services.read(file, buffer, size);

		if (readBytes == size)
			text = buffer;

		delete[] buffer;
	}

	getSettings().services.close(file);

	return text;
}

static WidgetType getWidgetTypeFromName(std::string name)
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
	if (name == "multilineTextInput") return WidgetType::MultilineTextInput;

	return WidgetType::None;
}

static WidgetElementId getWidgetElementFromName(std::string name)
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
	if (name == "scrollViewScrollBarV") return WidgetElementId::ScrollViewScrollBarV;
	if (name == "scrollViewScrollThumbV") return WidgetElementId::ScrollViewScrollThumbV;
	if (name == "scrollViewScrollBarH") return WidgetElementId::ScrollViewScrollBarH;
	if (name == "scrollViewScrollThumbH") return WidgetElementId::ScrollViewScrollThumbH;
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
	if (name == "multilineTextInputBody") return WidgetElementId::MultilineTextInputBody;
	if (name == "multilineTextInputLineNumbers") return WidgetElementId::MultilineTextInputLineNumbers;
	if (name == "multilineTextInputCurrentLineHighlight") return WidgetElementId::MultilineTextInputCurrentLineHighlight;
	if (name == "multilineTextInputWordWrap") return WidgetElementId::MultilineTextInputWordWrap;

	return WidgetElementId::Custom;
}

static std::string getPath(const std::string& fname)
{
	return std::filesystem::path(fname).parent_path().string();
}

static void setThemeElement(
	HTheme theme,
	const std::string& themePath,
	const char* styleName,
	WidgetType widgetType,
	WidgetElementId elemId,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	WidgetElementInfo elemInfo;

	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	HImage image = hui::getThemeImage(theme, imageFilename.c_str());

	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (!image && !imageName.empty())
	{
		image = loadThemeImage(theme, imageFilename.c_str());
	}

	auto font = hui::getThemeFont(theme, fontName.c_str());

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	bgColor = getColorFromText(color.c_str());
	txtColor = getColorFromText(textColor.c_str());

	elemInfo.image = image;
	elemInfo.border = border;
	elemInfo.color = bgColor;
	elemInfo.textColor = txtColor;
	elemInfo.font = font;
	elemInfo.width = width;
	elemInfo.height = height;

	hui::setThemeWidgetElement(theme, elemId, widgetStateType, elemInfo, styleName);
}

static void setUserElement(
	HTheme theme,
	const std::string& themePath,
	const char* styleName,
	const std::string& widgetName,
	const std::string& elemName,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	WidgetElementInfo elemInfo;
	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	HImage image = hui::getThemeImage(theme, imageFilename.c_str());
	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (!image && !imageName.empty())
	{
		image = loadThemeImage(theme, imageFilename.c_str());
	}

	auto font = hui::getThemeFont(theme, fontName.c_str());

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	bgColor = getColorFromText(color.c_str());
	txtColor = getColorFromText(textColor.c_str());

	elemInfo.image = image;
	elemInfo.border = border;
	elemInfo.color = bgColor;
	elemInfo.textColor = txtColor;
	elemInfo.font = font;
	elemInfo.width = width,
	elemInfo.height = height;

	setThemeUserWidgetElement(theme, elemName.c_str(), widgetStateType, elemInfo, styleName);
}

static WidgetStateType widgetStateFromText(const std::string& stateName)
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
		auto& name = fontNames[i];
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
		auto& name = settingNames[i];
		auto val = settings.get(name.c_str(), Json::Value());
		hui::setThemeUserSetting(theme, name.c_str(), val.asCString());
	}

	Json::Value widgets = root.get("widgets", Json::Value());
	auto widgetNames = widgets.getMemberNames();

	for (size_t i = 0; i < widgetNames.size(); i++)
	{
		auto& widgetName = widgetNames[i];
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
				auto& styleName = styleNames[k];
				auto styleElem = styles.get(styleName, Json::Value());

				if (widgetType != WidgetType::None)
				{
					readElements(styleName, styleElem);
				}
				else
				{
					readUserElements(styleName, styleElem);
				}
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

	return theme;
}

HImage loadThemeImage(HTheme theme, const char* pngFilename)
{
	ImageData img;
	bool ret = loadPngImage(pngFilename, img);

	if (ret && img.pixels)
	{
		return addThemeImage(theme, pngFilename, img);
	}

	return 0;
}


}