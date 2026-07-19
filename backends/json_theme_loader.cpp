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
	outImage = {};
	i32 width = 0;
	i32 height = 0;
	i32 comp = 0;
	stbi_uc* imgFileData = nullptr;
	HFile file = contextGetSettings().services.open(path, "rb");
	u64 fsize = 0;

	if (!file)
	{
		return false;
	}

	if (file)
	{
		contextGetSettings().services.seek(file, FileSeekMode::End, 0);
		fsize = contextGetSettings().services.tell(file);

		if (fsize == 0)
		{
			contextGetSettings().services.close(file);
			return false;
		}

		contextGetSettings().services.seek(file, FileSeekMode::Set, 0);
		imgFileData = new stbi_uc[fsize];
		auto readSize = contextGetSettings().services.read(file, imgFileData, fsize);

		if (fsize != readSize)
		{
			delete[] imgFileData;
			contextGetSettings().services.close(file);

			return false;
		}
	}

	contextGetSettings().services.close(file);

	stbi_uc* data = stbi_load_from_memory(imgFileData, fsize, &width, &height, &comp, 4);

	bool result = !(!data || !width || !height || !comp);

	if (result)
	{
		outImage.pixels = new Rgba32[width * height];
		memcpy(outImage.pixels, (Rgba32*)data, sizeof(Rgba32) * width * height);
		outImage.width = width;
		outImage.height = height;
	}

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
		HFile file = contextGetSettings().services.open(path, "wb");

		if (!file)
			return;

		contextGetSettings().services.write(file, data, size);
		contextGetSettings().services.close(file);
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
	auto file = contextGetSettings().services.open(path, "rb");

	if (!file)
		return std::string("");

	contextGetSettings().services.seek(file, FileSeekMode::End, 0);
	auto size = contextGetSettings().services.tell(file);
	std::string text;

	if (size != -1)
	{
		contextGetSettings().services.seek(file, FileSeekMode::Set, 0);

		char* buffer = new char[size + 1];
		memset(buffer, 0, size + 1);
		auto readBytes = contextGetSettings().services.read(file, buffer, size);

		if (readBytes == size)
			text = buffer;

		delete[] buffer;
	}

	contextGetSettings().services.close(file);

	return text;
}

static WidgetType getWidgetTypeFromName(std::string name)
{
	if (name == "window") return WidgetType::Window;
	if (name == "tooltip") return WidgetType::Tooltip;
	if (name == "button") return WidgetType::Button;
	if (name == "buttonGroup") return WidgetType::ButtonGroup;
	if (name == "imageButton") return WidgetType::ImageButton;
	if (name == "textInput") return WidgetType::TextInput;
	if (name == "slider") return WidgetType::Slider;
	if (name == "progress") return WidgetType::Progress;
	if (name == "image") return WidgetType::Image;
	if (name == "check") return WidgetType::Check;
	if (name == "radio") return WidgetType::Radio;
	if (name == "label") return WidgetType::Label;
	if (name == "expandable") return WidgetType::Expandable;
	if (name == "treeNode") return WidgetType::TreeNode;
	if (name == "popup") return WidgetType::Popup;
	if (name == "dropdown") return WidgetType::Dropdown;
	if (name == "list") return WidgetType::List;
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
	if (name == "circularSlider") return WidgetType::CircularSlider;
	if (name == "colorPicker") return WidgetType::ColorPicker;
	if (name == "table") return WidgetType::Table;
	if (name == "multilineTextInput") return WidgetType::MultilineTextInput;

	return WidgetType::None;
}

static WidgetElementId getWidgetElementFromName(std::string name)
{
	if (name == "windowBody") return WidgetElementId::WindowBody;
	if (name == "buttonBody") return WidgetElementId::ButtonBody;
	if (name == "buttonGroupLeftBody") return WidgetElementId::ButtonGroupLeftBody;
	if (name == "buttonGroupMiddleBody") return WidgetElementId::ButtonGroupMiddleBody;
	if (name == "buttonGroupRightBody") return WidgetElementId::ButtonGroupRightBody;
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
	if (name == "treeNodeBody") return WidgetElementId::TreeNodeBody;
	if (name == "treeNodeCollapsedArrow") return WidgetElementId::TreeNodeCollapsedArrow;
	if (name == "treeNodeExpandedArrow") return WidgetElementId::TreeNodeExpandedArrow;
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
	if (name == "circularSliderBody") return WidgetElementId::CircularSliderBody;
	if (name == "circularSliderMark") return WidgetElementId::CircularSliderMark;
	if (name == "circularSliderValueDot") return WidgetElementId::CircularSliderValueDot;
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

static void setErrorText(char* errorTextBuffer, size_t errorTextBufferSize, const std::string& errorText)
{
	if (!errorTextBuffer || errorTextBufferSize == 0)
	{
		return;
	}

	size_t size = std::min(errorTextBufferSize - 1, errorText.size());
	memcpy(errorTextBuffer, errorText.c_str(), size);
	errorTextBuffer[size] = 0;
}

static HImage getFallbackImage(HTheme theme)
{
	return hui::themeGetImage(theme, "__WHITEIMAGE__");
}

static WidgetElementInfo getElementInfoFromState(
	HTheme theme,
	const std::string& themePath,
	Json::Value state,
	const WidgetElementInfo& fallback,
	i32 width,
	i32 height)
{
	WidgetElementInfo elemInfo = fallback;

	if (!state.isObject())
	{
		if (!elemInfo.image)
		{
			elemInfo.image = getFallbackImage(theme);
		}

		return elemInfo;
	}

	if (state.isMember("image") && state["image"].isString())
	{
		auto imageName = state["image"].asString();
		auto imageFilename = themePath + imageName + ".png";
		HImage image = hui::themeGetImage(theme, imageFilename.c_str());

		if (!image && !imageName.empty())
		{
			image = loadThemeImage(theme, imageFilename.c_str());
		}

		elemInfo.image = image;
	}

	if (!elemInfo.image)
	{
		elemInfo.image = getFallbackImage(theme);
	}

	if (state.isMember("border") && state["border"].isNumeric())
		elemInfo.border = state["border"].asInt();

	if (state.isMember("color") && state["color"].isString())
		elemInfo.color = colorFromText(state["color"].asString().c_str());

	if (state.isMember("textColor") && state["textColor"].isString())
		elemInfo.textColor = colorFromText(state["textColor"].asString().c_str());

	if (state.isMember("font") && state["font"].isString())
		elemInfo.font = hui::themeFontGetFromTheme(theme, state["font"].asString().c_str());

	if (state.isMember("width") && state["width"].isNumeric())
		elemInfo.width = state["width"].asInt();

	if (state.isMember("height") && state["height"].isNumeric())
		elemInfo.height = state["height"].asInt();

	return elemInfo;
}

static void setThemeElementInfo(
	HTheme theme,
	const char* styleName,
	WidgetElementId elemId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elemInfo)
{
	hui::themeSetWidgetElement(theme, elemId, widgetStateType, elemInfo, styleName);
}

static void setUserElementInfo(
	HTheme theme,
	const char* styleName,
	const std::string& elemName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elemInfo)
{
	themeSetUserWidgetElement(theme, elemName.c_str(), widgetStateType, elemInfo, styleName);
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
	if (!filename)
	{
		setErrorText(errorTextBuffer, errorTextBufferSize, "Filename is null");
		return 0;
	}

	HTheme theme = hui::themeCreate(hui::contextGetSettings().defaultAtlasSize);

	// Initialize all elements with white image
	HImage whiteImage = getFallbackImage(theme);
	WidgetElementInfo defInfo;
	defInfo.image = whiteImage;
	defInfo.color = Color::white;
	defInfo.textColor = Color::white;
	defInfo.border = 0;
	defInfo.width = 0;
	defInfo.height = 0;
	defInfo.font = 0;

	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
		{
			hui::themeSetWidgetElement(theme, (WidgetElementId)i, (WidgetStateType)j, defInfo, "default");
		}
	}

	Json::Reader reader;
	Json::Value root;
	auto json = readTextFile(filename);
	bool ok = reader.parse(json, root);
	std::string themePath = getPath(filename) + "/";

	if (!ok || json.empty())
	{
		setErrorText(errorTextBuffer, errorTextBufferSize, reader.getFormattedErrorMessages().c_str());
		// delete the empty theme created before
		hui::themeDestroy(theme);
		return hui::createBuiltinTheme(hui::contextGetSettings().defaultAtlasSize);
	}

	if (!root.isObject())
	{
		setErrorText(errorTextBuffer, errorTextBufferSize, "Theme JSON root must be an object.");
		themeDestroy(theme);
		return 0;
	}

	Json::Value fonts = root.get("fonts", Json::Value());
	if (fonts.isObject())
	{
		auto fontNames = fonts.getMemberNames();

		for (size_t i = 0; i < fontNames.size(); i++)
		{
			auto& name = fontNames[i];
			auto fnt = fonts.get(name.c_str(), Json::Value());

			if (!fnt.isObject())
				continue;

			if (!fnt.isMember("file") || !fnt["file"].isString()
				|| !fnt.isMember("size") || !fnt["size"].isNumeric())
				continue;

			std::string fontFilename = fnt["file"].asString();
			u32 fontSize = fnt["size"].asUInt();

			if (fontFilename.empty() || fontSize == 0)
				continue;

			if (fontFilename.find_first_of(':') == std::string::npos)
			{
				fontFilename = themePath + fontFilename;
			}

			themeFontCreate(theme, name.c_str(), fontFilename.c_str(), fontSize);
		}
	}

	Json::Value settings = root.get("settings", Json::Value());
	if (settings.isObject())
	{
		auto settingNames = settings.getMemberNames();

		for (size_t i = 0; i < settingNames.size(); i++)
		{
			auto& name = settingNames[i];
			auto val = settings.get(name.c_str(), Json::Value());

			if (val.isString())
				hui::themeSetUserSetting(theme, name.c_str(), val.asCString());
		}
	}

	Json::Value widgets = root.get("widgets", Json::Value());
	if (!widgets.isObject())
	{
		setErrorText(errorTextBuffer, errorTextBufferSize, "Theme JSON must contain a widgets object.");
		themeDestroy(theme);
		return 0;
	}

	auto widgetNames = widgets.getMemberNames();

	for (size_t i = 0; i < widgetNames.size(); i++)
	{
		auto& widgetName = widgetNames[i];
		auto widget = widgets.get(widgetName.c_str(), Json::Value());

		if (!widget.isObject())
			continue;

		WidgetType widgetType = getWidgetTypeFromName(widgetName);
		auto styles = widget.get("styles", Json::Value());

		auto readElements = [theme, themePath](const std::string& styleName, Json::Value& parentElem)
		{
			if (!parentElem.isObject())
				return;

			auto elementNames = parentElem.getMemberNames();
			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto elemType = getWidgetElementFromName(elementNames[l]);
				auto elem = parentElem.get(elementNames[l], Json::Value());

				if (!elem.isObject())
					continue;

				auto width = elem.isMember("width") && elem["width"].isNumeric() ? elem["width"].asInt() : 0;
				auto height = elem.isMember("height") && elem["height"].isNumeric() ? elem["height"].asInt() : 0;
				auto elemStates = elem.getMemberNames();
				WidgetElementInfo defaultInfo;
				defaultInfo.color = Color::white;
				defaultInfo.textColor = Color::white;
				defaultInfo.width = width;
				defaultInfo.height = height;
				auto normalInfo = getElementInfoFromState(
					theme,
					themePath,
					elem.get("normal", Json::Value()),
					defaultInfo,
					width,
					height);
				bool stateSet[(u32)WidgetStateType::Unknown] = {};

				setThemeElementInfo(theme, styleName.c_str(), elemType, WidgetStateType::Normal, normalInfo);
				stateSet[(u32)WidgetStateType::Normal] = true;

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = widgetStateFromText(stateName);

					if (widgetStateType != WidgetStateType::Unknown)
					{
						auto stateInfo = getElementInfoFromState(
							theme,
							themePath,
							elemState,
							normalInfo,
							width,
							height);
						setThemeElementInfo(theme, styleName.c_str(), elemType, widgetStateType, stateInfo);
						stateSet[(u32)widgetStateType] = true;
					}
					else
					{
						if (elemState.isString())
							hui::themeSetWidgetElementParameter(theme, elemType, styleName.c_str(), stateName.c_str(), elemState.asString().c_str());
					}
				}

				for (u32 stateIndex = 0; stateIndex < (u32)WidgetStateType::Unknown; stateIndex++)
				{
					if (!stateSet[stateIndex])
					{
						setThemeElementInfo(theme, styleName.c_str(), elemType, (WidgetStateType)stateIndex, normalInfo);
					}
				}
			}
		};

		auto readUserElements = [theme, themePath](const std::string& styleName, Json::Value& parentElem)
		{
			if (!parentElem.isObject())
				return;

			auto elementNames = parentElem.getMemberNames();

			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto& elementName = elementNames[l];
				auto elem = parentElem.get(elementName, Json::Value());

				if (!elem.isObject())
					continue;

				auto width = elem.isMember("width") && elem["width"].isNumeric() ? elem["width"].asInt() : 0;
				auto height = elem.isMember("height") && elem["height"].isNumeric() ? elem["height"].asInt() : 0;
				auto elemStates = elem.getMemberNames();
				WidgetElementInfo defaultInfo;
				defaultInfo.color = Color::white;
				defaultInfo.textColor = Color::white;
				defaultInfo.width = width;
				defaultInfo.height = height;
				auto normalInfo = getElementInfoFromState(
					theme,
					themePath,
					elem.get("normal", Json::Value()),
					defaultInfo,
					width,
					height);
				bool stateSet[(u32)WidgetStateType::Unknown] = {};

				setUserElementInfo(theme, styleName.c_str(), elementName, WidgetStateType::Normal, normalInfo);
				stateSet[(u32)WidgetStateType::Normal] = true;

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = widgetStateFromText(stateName);

					if (widgetStateType != WidgetStateType::Unknown)
					{
						auto stateInfo = getElementInfoFromState(
							theme,
							themePath,
							elemState,
							normalInfo,
							width,
							height);
						setUserElementInfo(theme, styleName.c_str(), elementName, widgetStateType, stateInfo);
						stateSet[(u32)widgetStateType] = true;
					}
					else
					{
						if (elemState.isString())
							hui::themeSetUserWidgetElementParameter(theme, elementName.c_str(), styleName.c_str(), stateName.c_str(), elemState.asString().c_str());
					}
				}

				for (u32 stateIndex = 0; stateIndex < (u32)WidgetStateType::Unknown; stateIndex++)
				{
					if (!stateSet[stateIndex])
					{
						setUserElementInfo(theme, styleName.c_str(), elementName, (WidgetStateType)stateIndex, normalInfo);
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
	HImage image = 0;

	if (ret && img.pixels)
	{
		image = themeAddImage(theme, pngFilename, img);
		deleteImageData(img);
	}

	return image;
}


}
