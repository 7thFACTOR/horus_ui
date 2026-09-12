#include <horus.h>
#include "native_file_dialogs.h"
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

namespace hui
{

struct DemoState
{
	// button
	int buttonClickCount = 0;

	// textInput
	char textBasic[256] = "Hello World";
	char textNumeric[256] = "42";
	char textHex[256] = "FF";
	char textDefault[256] = "";
	char textPassword[256] = "secret";
	char textAutoSelect[256] = "Select me";

	// textInput Multiline
	char multiText[4096] =
		"-- demo.lua\n"
		"local function greet(name)\n"
		"\tlocal message = \"Hello, \" .. name\n"
		"\tprint(message)\n"
		"\treturn #message\n"
		"end\n"
		"\n"
		"for i = 1, 10 do\n"
		"\tif i % 2 == 0 then\n"
		"\t\tgreet('user' .. i)\n"
		"\tend\n"
		"end\n";

	// slider
	i32 sliderIntVal = 50;
	i32 sliderIntStepped = 0;
	f32 sliderFloatVal = 0.5f;
	f32 sliderFloatStepped = 0.0f;

	// comboSlider
	i32 comboSliderInt = 0;
	i32 comboSliderIntRanged = 50;
	f32 comboSliderFloat = 0.0f;
	f32 comboSliderFloatCustomString = 0.0f;
	f32 comboSliderFloatRanged = 0.5f;
	f32 comboSliderNullFloat = 0.75f;
	bool comboSliderNullAssigned = false;

	// circularSlider
	f32 circularVal = 0.5f;
	f32 circularTwoSide = 0.0f;
	f32 circularVal2 = 50.0f;
	f32 circularVal3 = 0.7f;

	// progress
	f32 progressValue = 0.0f;
	f32 progressValueReal = 0.0f;

	// check
	bool checkA = true;
	bool checkB = false;
	bool triStateSelectAll = false;
	bool triStateIndeterminate = false;
	bool triState1 = true;
	bool triState2 = false;

	// button Group
	u32 buttonGroupVal = 0;

	// radio
	i32 radioVal = 0;

	// dropdown
	i32 dropdownSel = 0;
	i32 dropdownNoSel = -1;

	// list
	bool listSelected[5] = {};

	// expandable states (managed internally by expandable())
	bool expandButton = true;
	bool expandTextInput = false;
	bool expandMultiline = false;
	bool expandSlider = false;
	bool expandComboSlider = false;
	bool expandCircularSlider = false;
	bool expandProgress = false;
	bool expandCheck = false;
	bool expandRadio = false;
	bool expandLabel = false;
	bool expandExpandable = false;
	bool expandDropdown = false;
	bool expandList = false;
	bool expandSelectable = false;
	bool expandSeparators = false;
	bool expandLink = false;
	bool expandTabs = false;
	bool expandBox = false;
	bool expandColorPicker = false;
	bool expandVecEditors = false;
	bool expandImages = false;
	bool expandMenus = false;
	bool expandViewport = false;
	bool expandCustomWidget = false;
	bool expandObjectRef = false;
	bool expandTable = false;
	bool expandTooltip = false;
	bool expandPopup = false;
	bool expandScrollView = false;
	bool expandCustomFileDialog = false;

	// added: Tree nodes demo expand flag
	bool expandTreeNodes = false;

	// disabled states
	bool disableButton = false;
	bool disableTextInput = false;
	bool disableMultiline = false;
	bool disableSlider = false;
	bool disableComboSlider = false;
	bool disableCircularSlider = false;
	bool disableProgress = false;
	bool disableCheck = false;
	bool disableRadio = false;
	bool disableLabel = false;
	bool disableExpandable = false;
	bool disableDropdown = false;
	bool disableList = false;
	bool disableSelectable = false;
	bool disableSeparators = false;
	bool disableLink = false;
	bool disableTabs = false;
	bool disableBox = false;
	bool disableColorPicker = false;
	bool disableVecEditors = false;
	bool disableImages = false;
	bool disableMenus = false;
	bool disableViewport = false;
	bool disableCustomWidget = false;
	bool disableObjectRef = false;
	bool disableTable = false;
	bool disableTooltip = false;
	bool disablePopup = false;
	bool disableScrollView = false;
	bool disableCustomFileDialog = false;

	// added: Tree nodes demo disable flag
	bool disableTreeNodes = false;

	// popup demo
	bool showPopup = false;

	// custom file dialog demo
	char customFileDialogResult[256] = "";
	u32 customFileDialogMode = 0;

	// link demo
	int linkClickCount = 0;

	// paragraph demo
	int paragraphLinkClicks = 0;
	int paragraphButtonClicks = 0;

	// tabs
	TabIndex selectedTab = 0;

	// color picker
	Color pickerColor = Color(1.0f, 0.0f, 0.0f, 1.0f);
	Color customPickerColors[16] = {};
	u32 customPickerColorCount = 0;

	// vec editors
	f32 vec2x = 1.0f, vec2y = 2.0f;
	f32 vec3x = 1.0f, vec3y = 2.0f, vec3z = 3.0f;
	f64 dvec2x = 1.0, dvec2y = 2.0;
	f64 dvec3x = 1.0, dvec3y = 2.0, dvec3z = 3.0;
	f32 multiBaseX = 1.0f, multiBaseY = 2.0f, multiBaseZ = 3.0f;
	bool multiDiffX = true, multiDiffY = false, multiDiffZ = true;

	// image & Texture
	HImage demoImage = nullptr;
	HTexture demoTexture = nullptr;

	// objectRef
	void* objectRefValue1 = nullptr;
	bool objectRefModified1 = false;
	void* objectRefValue2 = nullptr;
	bool objectRefModified2 = false;
	void* objectRefValue3 = nullptr;
	bool objectRefModified3 = false;
	void* objectRefEmpty = nullptr;
	bool objectRefEmptyModified = false;

	// virtual List
	bool expandVirtualList = false;
	bool disableVirtualList = false;
	i32 virtualListCount = 1000;
	
	// drag & Drop Demo
	bool expandDragDrop = false;
	int dragDropSrcA = 0;
	int dragDropSrcB = 0;
	void* dragDropTargetAValue = nullptr;
	void* dragDropTargetBValue = nullptr;
	void* dragDropTargetCValue = nullptr;
	std::vector<std::string> dragListA = { "Apple", "Banana", "Cherry", "Date" };
	std::vector<std::string> dragListB = { "Eclair", "Fig", "Grape" };
	i32 dragListIdxA = -1;
	i32 dragListIdxB = -1;
	Point dragScrollListA = { 0, 0 };
	Point dragScrollListB = { 0, 0 };
	bool imageSlots[3] = {};

	// property grid (mesh inspector)
	bool expandPropertyGrid = false;
	bool disablePropertyGrid = false;
	bool pgAltRowBg = false;
	char pgMeshName[128] = "MonkeyMesh.obj";
	char pgTag[64] = "enemy_medium";
	i32 pgLayer = 3;
	i32 pgVertexCount = 25464;
	i32 pgTriangleCount = 50972;
	i32 pgLodLevel = 0;
	f32 pgPosX = 0.0f, pgPosY = 0.0f, pgPosZ = 0.0f;
	f32 pgRotX = 0.0f, pgRotY = 0.0f, pgRotZ = 0.0f;
	f32 pgScaleX = 1.0f, pgScaleY = 1.0f, pgScaleZ = 1.0f;
	f32 pgBoundsMinX = -1.5f, pgBoundsMinY = -0.8f, pgBoundsMinZ = -0.7f;
	f32 pgBoundsMaxX = 1.5f, pgBoundsMaxY = 0.8f, pgBoundsMaxZ = 0.7f;
	Color pgAlbedoColor = Color(0.8f, 0.6f, 0.3f, 1.0f);
	Color pgEmissionColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
	f32 pgRoughness = 0.65f;
	f32 pgMetalness = 0.1f;
	f32 pgOpacity = 1.0f;
	i32 pgShadingMode = 3;
	char pgTexturePath[256] = "textures/marble_diffuse.png";
	bool pgCastShadows = true;
	bool pgReceiveShadows = true;
	bool pgDoubleSided = false;
	bool pgSmoothNormals = true;
	bool pgWireframe = false;
	bool pgFrustumCulling = true;
	u32 pgCullMode = 1;
	i32 pgDrawPriority = 100;
	bool pgGroupIdentity = true;
	bool pgGroupGeometry = true;
	bool pgGroupTransform = true;
	bool pgGroupMaterial = true;
	bool pgGroupRender = true;

	Point scrollPos = { 0, 0 };

	bool initialized = false;

	Point demoScrollPos = { 0, 0 };

	// tree demo internal state (controlled expansion)
	bool treeRootExpanded = true;
	bool treeFolderAExpanded = false;
	bool treeFolderBExpanded = false;

	// invisible table with 3 lists
	std::vector<std::string> tableListA = { "Apple", "Banana", "Cherry", "Date", "Elderberry" };
	std::vector<std::string> tableListB = { "Fig", "Grape", "Honeydew", "Kiwi" };
	std::vector<std::string> tableListC = { "Lemon", "Mango", "Nectarine", "Orange", "Papaya", "Quince" };
	i32 tableListIdxA = -1;
	i32 tableListIdxB = -1;
	i32 tableListIdxC = -1;
	Point tableScrollA = { 0, 0 };
	Point tableScrollB = { 0, 0 };
	Point tableScrollC = { 0, 0 };
};

static DemoState demo;

template <typename EditorFn>
static void propGridRow(const char* name, EditorFn&& editorFn)
{
	tableRowNext();
	label(name, HAlignType::Right);
	tableCellNext();
	editorFn();
}

// ------------------------------------------------------------------
// paragraph demo helpers (rich text style help text)
// ------------------------------------------------------------------
enum class ParagraphItemType
{
	Text,
	Link,
	Button,
	Image
};

struct ParagraphItem
{
	ParagraphItemType type = ParagraphItemType::Text;
	std::string text;
	HImage image = 0;
};

static f32 paragraphItemWidth(const ParagraphItem& item, f32 imageHeight)
{
	WidgetElementInfo labelBody;
	themeGetWidgetElementInfo(WidgetElementId::LabelBody, WidgetStateType::Normal, labelBody);
	WidgetElementInfo linkBody;
	themeGetWidgetElementInfo(WidgetElementId::LinkBody, WidgetStateType::Normal, linkBody);
	WidgetElementInfo btnBody;
	themeGetWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, btnBody);

	switch (item.type)
	{
	case ParagraphItemType::Text:
		return fontGetTextSize(labelBody.font, item.text.c_str()).x;

	case ParagraphItemType::Link:
		return fontGetTextSize(linkBody.font, item.text.c_str()).x;

	case ParagraphItemType::Button:
		return (btnBody.border * 2.0f + fontGetTextSize(btnBody.font, item.text.c_str()).x) * scaleGet();

	case ParagraphItemType::Image:
	{
		Point imgSize = imageGetDimensions(item.image);

		if (imgSize.x <= 0.0f || imgSize.y <= 0.0f)
		{
			return 0.0f;
		}

		f32 imgWidth = imgSize.x * scaleGet();
		f32 imgHeight = imgSize.y * scaleGet();
		f32 targetHeight = imageHeight * scaleGet();

		if (imgHeight >= targetHeight && targetHeight > 0)
		{
			imgWidth *= targetHeight / imgHeight;
		}

		return imgWidth;
	}
	}

	return 0.0f;
}

static void paragraphItemEmit(const ParagraphItem& item, u32 index, f32 imageHeight, int& linkClicks, int& buttonClicks)
{
	switch (item.type)
	{
	case ParagraphItemType::Text:
		label((item.text + "##paraText" + std::to_string(index)).c_str());
		break;

	case ParagraphItemType::Link:
		if (link((item.text + "##paraLink" + std::to_string(index)).c_str()))
		{
			linkClicks++;
		}
		break;

	case ParagraphItemType::Button:
		if (button((item.text + "##paraButton" + std::to_string(index)).c_str()))
		{
			buttonClicks++;
		}
		break;

	case ParagraphItemType::Image:
		if (item.image)
		{
			image(item.image, imageHeight, HAlignType::Left);
		}
		break;
	}
}

// draws the given items as a flowing paragraph, wrapping inside the current layout width
static void paragraphDraw(const ParagraphItem* items, u32 count, f32 imageHeight, int& linkClicks, int& buttonClicks)
{
	auto& padding = widgetGetPadding();
	f32 availableWidth = layoutGetSize().x - padding.x * 2.0f * scaleGet();
	f32 itemSpacing = sameLineSpacingGet() * scaleGet();
	f32 lineWidth = 0.0f;
	bool firstOnLine = true;

	for (u32 i = 0; i < count; i++)
	{
		f32 itemWidth = paragraphItemWidth(items[i], imageHeight);
		f32 gap = firstOnLine ? 0.0f : itemSpacing;

		if (!firstOnLine && lineWidth + gap + itemWidth > availableWidth)
		{
			// item does not fit on the current line, move it to the next one
			firstOnLine = true;
			lineWidth = 0.0f;
			gap = 0.0f;
		}

		if (!firstOnLine)
		{
			sameLine();
		}

		paragraphItemEmit(items[i], i, imageHeight, linkClicks, buttonClicks);
		lineWidth += gap + itemWidth;
		firstOnLine = false;
	}
}

// ------------------------------------------------------------------
// custom file dialog demo: a tiny hardcoded virtual file system
// ------------------------------------------------------------------
namespace
{
struct DemoVfsEntry
{
	const char* name;
	bool isDirectory;
};

const DemoVfsEntry demoVfsRoot[] = {
	{ "Assets", true },
	{ "Scenes", true },
	{ "Shaders", true },
	{ "game.lua", false },
	{ "readme.txt", false },
	{ "splash.png", false },
};

const DemoVfsEntry demoVfsAssets[] = {
	{ "Audio", true },
	{ "Meshes", true },
	{ "Textures", true },
	{ "car.fbx", false },
	{ "house.fbx", false },
};

const DemoVfsEntry demoVfsAssetsAudio[] = {
	{ "shoot.wav", false },
	{ "theme.ogg", false },
};

const DemoVfsEntry demoVfsScenes[] = {
	{ "level1.lua", false },
	{ "level2.lua", false },
};

const DemoVfsEntry demoVfsShaders[] = {
	{ "default.frag", false },
	{ "default.vert", false },
};

void demoCustomFileDialogList(const char* path, std::vector<CustomFileDialogEntry>& outEntries, void* userData)
{
	const DemoVfsEntry* src = nullptr;
	size_t count = 0;

	if (strcmp(path, "/") == 0)
	{
		src = demoVfsRoot;
		count = sizeof(demoVfsRoot) / sizeof(demoVfsRoot[0]);
	}
	else if (strcmp(path, "/Assets") == 0)
	{
		src = demoVfsAssets;
		count = sizeof(demoVfsAssets) / sizeof(demoVfsAssets[0]);
	}
	else if (strcmp(path, "/Assets/Audio") == 0)
	{
		src = demoVfsAssetsAudio;
		count = sizeof(demoVfsAssetsAudio) / sizeof(demoVfsAssetsAudio[0]);
	}
	else if (strcmp(path, "/Scenes") == 0)
	{
		src = demoVfsScenes;
		count = sizeof(demoVfsScenes) / sizeof(demoVfsScenes[0]);
	}
	else if (strcmp(path, "/Shaders") == 0)
	{
		src = demoVfsShaders;
		count = sizeof(demoVfsShaders) / sizeof(demoVfsShaders[0]);
	}

	if (src)
	{
		for (size_t i = 0; i < count; i++)
		{
			CustomFileDialogEntry entry;
			entry.name = src[i].name;
			entry.isDirectory = src[i].isDirectory;
			outEntries.push_back(entry);
		}
	}
}

static void demoCustomFileDialogPreview(const char* path, const Rect& previewRect, void* userData)
{
	renderSetFillStyle(Color::fromU8(42, 42, 42, 255));
	renderDrawSolidRectangle(previewRect);
	
	renderSetColor(Color::white);
	renderDrawTextInBox(path, previewRect, HAlignType::Center, VAlignType::Center);
}
} // namespace

void showDemo()
{
	if (!demo.initialized)
	{
		demo.initialized = true;
	}

	// animate progress bars
	f32 dt = contextGetSettings().deltaTime;
	demo.progressValue += dt * 0.1f;
	if (demo.progressValue > 1.0f) demo.progressValue = 0.0f;

	demo.progressValueReal += dt * 20.0f;
	if (demo.progressValueReal > 1000.0f) demo.progressValueReal = 0.0f;

	// keep repainting as long as the progress bar section is expanded
	if (demo.expandProgress)
	{
		forceRepaint();
	}

	scrollViewBegin("##demoScroll", 0, demo.demoScrollPos, { 0, 0 }, ScrollViewFlags::None);

	space();

	// ------------------------------------------------------------------
	// button
	// ------------------------------------------------------------------
	if (expandableBegin("Buttons", &demo.expandButton))
	{
		check("Disable##Button", &demo.disableButton);
		widgetPushDisabled(demo.disableButton);

		label("Basic button:");
		if (button("Click Me"))
		{
			demo.buttonClickCount++;
		}
		char countBuf[64];
		snprintf(countBuf, sizeof(countBuf), "Button clicked %d time(s)", demo.buttonClickCount);
		label(countBuf);

		space();
		label("Disabled button:");
		widgetSetNextDisabled();
		button("Disabled");

		space();
		label("Image button:");
		HImage btnImg = themeGetImage(themeGet(), "ic_attach_file_white_24dp");
		if (imageButton(btnImg, 32, 22))
		{
			demo.buttonClickCount++;
		}

		space();
		label("Button Group (3 items):");
		static const char* groupItems[] = { "Left", "Center", "Right" };
		if (buttonGroup("##btnGrp1", groupItems, 3, &demo.buttonGroupVal))
		{
			forceRepaint();
		}
		char groupBuf[64];
		snprintf(groupBuf, sizeof(groupBuf), "Selected: %u", demo.buttonGroupVal);
		label(groupBuf);

		space();
		label("Button Group (2 items):");
		static const char* groupItems2[] = { "On", "Off" };
		static u32 binaryGroupVal = 0;
		if (buttonGroup("##btnGrp2", groupItems2, 2, &binaryGroupVal))
		{
			forceRepaint();
		}

		space();
		label("Button Group (4 items):");
		static const char* groupItems4[] = { "Spring", "Summer", "Fall", "Winter" };
		static u32 seasonVal = 0;
		if (buttonGroup("##btnGrp3", groupItems4, 4, &seasonVal))
		{
			forceRepaint();
		}

		space();
		label("Disabled button group:");
		widgetSetNextDisabled();
		static u32 disabledGroupVal = 1;
		if (buttonGroup("##btnGrp4", groupItems, 3, &disabledGroupVal)) {}
		widgetPopDisabled();

		space();
		label("SameLineGroup (3 equal-width buttons):");
		sameLineGroupBegin(3);
		if (button("Left")) {}
		sameLineGroupNext();
		if (button("Center")) {}
		sameLineGroupNext();
		if (button("Right")) {}
		sameLineGroupEnd();

		expandableEnd();
	}

	// ------------------------------------------------------------------
	// text Input
	// ------------------------------------------------------------------
	if (expandableBegin("Text Input", &demo.expandTextInput))
	{
		check("Disable##TextInput", &demo.disableTextInput);
		widgetPushDisabled(demo.disableTextInput);

		label("Basic:");
		textInput("##tiBasic", demo.textBasic, sizeof(demo.textBasic));

		space();
		label("Numeric only:");
		textInput("##tiNumeric", demo.textNumeric, sizeof(demo.textNumeric), TextInputFlags::NumericOnly);

		space();
		label("Hex only:");
		textInput("##tiHex", demo.textHex, sizeof(demo.textHex), TextInputFlags::HexOnly);

		space();
		label("With default placeholder text:");
		textInput("##tiDefault", demo.textDefault, sizeof(demo.textDefault), TextInputFlags::None, "Type here...");

		space();
		label("Auto select all on focus:");
		textInput("##tiAutoSel", demo.textAutoSelect, sizeof(demo.textAutoSelect), TextInputFlags::AutoSelectAll);

		space();
		label("Password:");
		textInput("##tiPass", demo.textPassword, sizeof(demo.textPassword), TextInputFlags::None, nullptr, 0, true);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// multiline Text Input
	// ------------------------------------------------------------------
	if (expandableBegin("Multiline Text Input", &demo.expandMultiline))
	{
		check("Disable##MultilineTextInput", &demo.disableMultiline);
		widgetPushDisabled(demo.disableMultiline);

		// Lua syntax highlighting
		static const Color luaKeywordColor = Color::fromU8(86, 156, 214);
		static const Color luaStringColor = Color::fromU8(206, 145, 120);
		static const Color luaCommentColor = Color::fromU8(106, 153, 85);

		static const KeywordInfo luaKeywords[] = {
			{ "and", luaKeywordColor },
			{ "break", luaKeywordColor },
			{ "do", luaKeywordColor },
			{ "else", luaKeywordColor },
			{ "elseif", luaKeywordColor },
			{ "end", luaKeywordColor },
			{ "false", luaKeywordColor },
			{ "for", luaKeywordColor },
			{ "function", luaKeywordColor },
			{ "goto", luaKeywordColor },
			{ "if", luaKeywordColor },
			{ "in", luaKeywordColor },
			{ "local", luaKeywordColor },
			{ "nil", luaKeywordColor },
			{ "not", luaKeywordColor },
			{ "or", luaKeywordColor },
			{ "repeat", luaKeywordColor },
			{ "return", luaKeywordColor },
			{ "then", luaKeywordColor },
			{ "true", luaKeywordColor },
			{ "until", luaKeywordColor },
			{ "while", luaKeywordColor },
		};

		static const RangeHighlight luaRangeHighlights[] = {
			{ "\"", "\"", luaStringColor, "\\" },
			{ "'", "'", luaStringColor, "\\" },
			{ "--", "", luaCommentColor },
		};

		static const u32 luaKeywordCount = sizeof(luaKeywords) / sizeof(luaKeywords[0]);
		static const u32 luaRangeHighlightCount = sizeof(luaRangeHighlights) / sizeof(luaRangeHighlights[0]);

		label("Basic (10 visible lines):");
		textInputMultiline("##mtiBasic", demo.multiText, sizeof(demo.multiText), 10, MultilineTextInputFlags::None, luaKeywords, luaKeywordCount, luaRangeHighlights, luaRangeHighlightCount);

		space();
		label("With line numbers:");
		textInputMultiline("##mtiLineNums", demo.multiText, sizeof(demo.multiText), 8, MultilineTextInputFlags::LineNumbers, luaKeywords, luaKeywordCount, luaRangeHighlights, luaRangeHighlightCount);

		space();
		label("With word wrap:");
		textInputMultiline("##mtiWordWrap", demo.multiText, sizeof(demo.multiText), 6, MultilineTextInputFlags::WordWrap, luaKeywords, luaKeywordCount, luaRangeHighlights, luaRangeHighlightCount);

		space();
		label("Lua syntax + line numbers + highlight current line:");
		textInputMultiline("##mtiHighlight", demo.multiText, sizeof(demo.multiText), 8,
			MultilineTextInputFlags::LineNumbers | MultilineTextInputFlags::HighlightCurrentLine,
			luaKeywords, luaKeywordCount, luaRangeHighlights, luaRangeHighlightCount);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// slider
	// ------------------------------------------------------------------
	if (expandableBegin("Sliders", &demo.expandSlider))
	{
		check("Disable##Slider", &demo.disableSlider);
		widgetPushDisabled(demo.disableSlider);

		char buf[64];

		label("Integer slider (0..100):");
		sliderInt("##slInt", 0, 100, demo.sliderIntVal);
		snprintf(buf, sizeof(buf), "Value: %d", demo.sliderIntVal);
		label(buf);

		space();
		label("Integer slider with step=10:");
		sliderInt("##slIntStep", 0, 100, demo.sliderIntStepped, true, 10);
		snprintf(buf, sizeof(buf), "Value: %d", demo.sliderIntStepped);
		label(buf);

		space();
		label("Float slider (0..1):");
		sliderFloat("##slFloat", 0.0f, 1.0f, demo.sliderFloatVal);
		snprintf(buf, sizeof(buf), "Value: %.3f", demo.sliderFloatVal);
		label(buf);

		space();
		label("Float slider with step=0.1:");
		sliderFloat("##slFloatStep", 0.0f, 1.0f, demo.sliderFloatStepped, true, 0.1f);
		snprintf(buf, sizeof(buf), "Value: %.3f", demo.sliderFloatStepped);
		label(buf);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// comboSlider
	// ------------------------------------------------------------------
	if (expandableBegin("Combo Sliders", &demo.expandComboSlider))
	{
		check("Disable##ComboSlider", &demo.disableComboSlider);
		widgetPushDisabled(demo.disableComboSlider);

		label("Integer combo slider (unbounded):");
		comboSliderInt(&demo.comboSliderInt);

		space();
		label("Integer combo slider ranged (0..100):");
		comboSliderIntRanged(&demo.comboSliderIntRanged, 0, 100);

		space();
		label("Float combo slider (unbounded):");
		comboSliderFloat(&demo.comboSliderFloat);

		space();
		label("Float combo slider ranged (0..1):");
		comboSliderFloatRanged(&demo.comboSliderFloatRanged, 0.0f, 1.0f, 0.01f, 0.05f);

		space();
		label("With custom format string:");
		comboSliderFloat(&demo.comboSliderFloatCustomString, 1.0f, 1.0f, "%.2f units");

		space();
		label("No value assigned (indeterminate, editable, starts at 0):");
		check("Assign value##ComboSliderNull", &demo.comboSliderNullAssigned);
		if (demo.comboSliderNullAssigned)
		{
			comboSliderFloat(&demo.comboSliderNullFloat, 1.0f, 1.0f, "%.2f units");
		}
		else
		{
			comboSliderFloat(nullptr, 1.0f, 1.0f, "%.2f units", "Indeterminate");
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// circular Slider
	// ------------------------------------------------------------------
	if (expandableBegin("Circular Sliders", &demo.expandCircularSlider))
	{
		check("Disable##CircularSlider", &demo.disableCircularSlider);
		widgetPushDisabled(demo.disableCircularSlider);

		circularSliderFloat("Volume", &demo.circularVal, 0.0f, 1.0f, 0.01f);
		sameLine();
		circularSliderFloat("Pan", &demo.circularTwoSide, -1.0f, 1.0f, 0.01f, true);
		sameLine();
		circularSliderFloat("Speed", &demo.circularVal2, 0.0f, 100.0f, 1.0f);

		space();
		circularSliderFloat("Test", &demo.circularVal3, 0.0f, 100.0f, 1.0f, false, 10.0f, CircularSliderFlags::ShowValueInCenter);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// progress
	// ------------------------------------------------------------------
	if (expandableBegin("Progress Bars", &demo.expandProgress))
	{
		check("Disable##Progress", &demo.disableProgress);
		widgetPushDisabled(demo.disableProgress);

		label("Basic progress (65%):");
		progress(demo.progressValue);

		space();
		label("With text overlay:");
		progress(demo.progressValue, 0.0f, true);

		space();
		label("With real values:");
		progress(demo.progressValueReal, 1000, true, true);

		space();
		label("Indeterminate:");
		progress(-1.0f);

		space();
		label("Indeterminate (with text):");
		progress(-1.0f, 0.0f, true, false, "Loading, please wait...");

		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// check
	// ------------------------------------------------------------------
	if (expandableBegin("Checkboxes", &demo.expandCheck))
	{
		check("Disable##Check", &demo.disableCheck);
		widgetPushDisabled(demo.disableCheck);

		check("Option A (checked)", &demo.checkA);
		check("Option B (unchecked)", &demo.checkB);

		space();
		label("Tri-state checkbox (indeterminate when only some options are on):");

		// recompute the parent state from its children, the parent shows the
		// indeterminate minus (default) or check (all on)
		demo.triStateIndeterminate = demo.triState1 != demo.triState2;
		demo.triStateSelectAll = demo.triState1 && demo.triState2;

		if (check("Select all", &demo.triStateSelectAll, &demo.triStateIndeterminate))
		{
			demo.triState1 = demo.triStateSelectAll;
			demo.triState2 = demo.triStateSelectAll;
		}

		check("Option 1", &demo.triState1);
		check("Option 2", &demo.triState2);

		space();
		label("Disabled checkbox:");
		widgetSetNextDisabled();
		bool disabledCheck = true;
		check("Cannot change", &disabledCheck);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// radio
	// ------------------------------------------------------------------
	if (expandableBegin("Radio Buttons", &demo.expandRadio))
	{
		check("Disable##Radio", &demo.disableRadio);
		widgetPushDisabled(demo.disableRadio);

		radio("Choice 1", &demo.radioVal, 0);
		radio("Choice 2", &demo.radioVal, 1);
		radio("Choice 3", &demo.radioVal, 2);

		char buf[64];
		snprintf(buf, sizeof(buf), "Selected: %d", demo.radioVal);
		label(buf);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// label
	// ------------------------------------------------------------------
	if (expandableBegin("Labels", &demo.expandLabel))
	{
		check("Disable##Label", &demo.disableLabel);
		widgetPushDisabled(demo.disableLabel);

		label("Left aligned (default)");
		label("Center aligned", HAlignType::Center);
		label("Right aligned", HAlignType::Right);

		space();
		label("Custom color labels:");
		labelCustomColor("Red left", Color::red);
		labelCustomColor("Green center", Color::green, HAlignType::Center);
		labelCustomColor("Blue right", Color::blue, HAlignType::Right);
		labelCustomColor("Orange left (multiline)", Color::orange);
		labelCustomColorMultiline("This is a cyan multiline label with custom color that wraps across lines.", Color::cyan, HAlignType::Left);

		space();
		label("Custom font + color labels:");
		HFont titleFont = themeFontGet("title");
		HFont headingFont = themeFontGet("heading");
		HFont italicFont = themeFontGet("normal-italic");

		labelCustomFont("Title Font Label", titleFont);
		labelCustomFont("Heading Font Label", headingFont);
		labelCustom("Title + magenta center", titleFont, Color::magenta, HAlignType::Center);
		labelCustom("Heading + yellow right", headingFont, Color::yellow, HAlignType::Right);
		labelCustom("Italic + sky left", italicFont, Color::sky);
		labelCustomMultiline("Italic + green multiline wrapping text with custom font and color.", italicFont, Color::green, HAlignType::Center);

		space();
		label("Italic label:");
		labelCustomFont("This label uses italic font", italicFont);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// expandable (nested)
	// ------------------------------------------------------------------
	if (expandableBegin("Expandable (nested)", &demo.expandExpandable))
	{
		check("Disable##Expandable", &demo.disableExpandable);
		widgetPushDisabled(demo.disableExpandable);

		label("Expandables can be nested:");

		if (expandableBegin("Nested A"))
		{
			label("Content of Nested A");
			expandableEnd();
		}

		if (expandableBegin("Nested B"))
		{
			label("Content of Nested B");

			if (expandableBegin("Nested C (inside B)"))
			{
				label("Deep nested content");
				expandableEnd();
			}
			expandableEnd();
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// tree Nodes (new demo section)
	// ------------------------------------------------------------------
	if (expandableBegin("Tree Nodes", &demo.expandTreeNodes))
	{
		check("Disable##TreeNodes", &demo.disableTreeNodes);
		widgetPushDisabled(demo.disableTreeNodes);

		label("Controlled tree (passes booleans to preserve expansion state):");

		// root node with controlled expansion
		if (treeNodeBegin("Root", &demo.treeRootExpanded))
		{
			// folder A
			if (treeNodeBegin("Folder A", &demo.treeFolderAExpanded))
			{
				selectable("File A1");
				selectable("File A2");
				treeNodeEnd();
			}

			// folder B
			if (treeNodeBegin("Folder B", &demo.treeFolderBExpanded))
			{
				selectable("File B1");
				treeNodeEnd();
			}

			// a plain item at root level
			selectable("README.md");

			treeNodeEnd(); // close Root
		}

		space();

		label("Anonymous nodes (internal expansion state):");

		// use internal expansion state by passing nullptr for expansion var
		if (treeNodeBegin("Library"))
		{
			if (treeNodeBegin("src"))
			{
				selectable("main.cpp");
				selectable("util.cpp");
				treeNodeEnd();
			}

			if (treeNodeBegin("include"))
			{
				selectable("lib.h");
				treeNodeEnd();
			}

			treeNodeEnd();
		}

		// tree nodes with single-click toggle on the label
		label("ToggleOnSelect flag (single-click label toggles):");
		if (treeNodeBegin("Quick Access", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
		{
			if (treeNodeBegin("Documents", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
			{
				HImage icon = themeGetImage(themeGet(), "__WHITEIMAGE__");
				if (icon)
				{
					WidgetElementInfo btnBody;
					themeGetWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, btnBody);
					image(icon, btnBody.height, HAlignType::Left);
					sameLine();
				}
				if (button("Open##report"))
				{
				}
				sameLine();
				selectable("report.pdf");
				selectable("notes.txt");
				treeNodeEnd();
			}

			if (treeNodeBegin("Pictures", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
			{
				HImage icon = themeGetImage(themeGet(), "__WHITEIMAGE__");
				if (icon)
				{
					WidgetElementInfo btnBody;
					themeGetWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, btnBody);
					image(icon, btnBody.height, HAlignType::Left);
					sameLine();
				}
				if (button("View##photo"))
				{
				}
				sameLine();
				selectable("photo.jpg");
				treeNodeEnd();
			}

			treeNodeEnd();
		}

		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// dropdown
	// ------------------------------------------------------------------
	if (expandableBegin("Dropdown", &demo.expandDropdown))
	{
		check("Disable##Dropdown", &demo.disableDropdown);
		widgetPushDisabled(demo.disableDropdown);

		static const char* items[] = { "Apple", "Banana", "Cherry", "Date", "Elderberry" };

		label("Basic dropdown:");
		dropdown("##ddBasic", demo.dropdownSel, items, 5);

		space();
		label("With max visible items = 3:");
		dropdown("##ddMax3", demo.dropdownSel, items, 5, 3);

		space();
		label("No selection at start, with indeterminate text:");
		dropdown("##ddNoSel", demo.dropdownNoSel, items, 5, ~0, "Indeterminate");

		space();
		label("Indeterminate with disabled dropdown:");
		widgetPushDisabled(true);
		dropdown("##ddNoSelDisabled", demo.dropdownNoSel, items, 5, ~0, "Indeterminate");
		widgetPopDisabled();

		char buf[64];
		snprintf(buf, sizeof(buf), "Selected index: %d", demo.dropdownSel);
		label(buf);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// list
	// ------------------------------------------------------------------
	if (expandableBegin("List", &demo.expandList))
	{
		check("Disable##List", &demo.disableList);
		widgetPushDisabled(demo.disableList);

		static const char* listItems[] = { "Item A", "Item B", "Item C", "Item D", "Item E" };

		label("Single selection list:");
		list("##listSingle", demo.listSelected, ListSelectionMode::Single, listItems, 5, 120.0f);

		space();
		label("Multiple selection list:");
		static bool multiSel[5] = {};
		list("##listMulti", multiSel, ListSelectionMode::Multiple, listItems, 5, 120.0f);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// selectable
	// ------------------------------------------------------------------
	if (expandableBegin("Selectable", &demo.expandSelectable))
	{
		check("Disable##Selectable", &demo.disableSelectable);
		widgetPushDisabled(demo.disableSelectable);

		label("Normal selectable:");
		selectable("Selectable item 1");
		selectable("Selectable item 2");

		space();
		label("Selected state:");
		selectable("Selected item", SelectableFlags::Selected);

		space();
		label("Custom font selectables:");
		HFont headingFont = themeFontGet("heading");
		selectableCustomFont("Selectable with Heading Font", headingFont);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// separators / Spacing
	// ------------------------------------------------------------------
	if (expandableBegin("Separators & Spacing", &demo.expandSeparators))
	{
		check("Disable##Separators", &demo.disableSeparators);
		widgetPushDisabled(demo.disableSeparators);

		label("Line separator below:");
		line();
		label("Content after line");

		space();
		label("Default space() above this");

		space(30.0f);
		label("Custom space(30) above this");

		space();
		label("sameLine demo:");
		button("A");
		sameLine();
		button("B");
		sameLine();
		button("C");
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// link
	// ------------------------------------------------------------------
	if (expandableBegin("Links", &demo.expandLink))
	{
		check("Disable##Link", &demo.disableLink);
		widgetPushDisabled(demo.disableLink);

		label("Basic links:");
		if (link("Click me - Open URL"))
		{
			demo.linkClickCount++;
		}

		space();
		if (link("Another link"))
		{
			demo.linkClickCount++;
		}

		space();
		char countBuf[64];
		snprintf(countBuf, sizeof(countBuf), "Link clicked %d time(s)", demo.linkClickCount);
		label(countBuf);

		space();
		label("Disabled link:");
		widgetSetNextDisabled();
		link("Cannot click this");
		widgetPopDisabled();

		space();
		label("Paragraph (rich text help style):");

		WidgetElementInfo labelBody;
		themeGetWidgetElementInfo(WidgetElementId::LabelBody, WidgetStateType::Normal, labelBody);
		// fontGetMetrics().height is already scaled with the theme, so normalize it here
		f32 imageHeight = labelBody.height > fontGetMetrics(labelBody.font).height / scaleGet()
			? labelBody.height
			: fontGetMetrics(labelBody.font).height / scaleGet();

		WidgetElementInfo msgBoxImg;
		themeGetWidgetElementInfo(WidgetElementId::MessageBoxImageInfo, WidgetStateType::Normal, msgBoxImg);
		ParagraphItem helpParagraph[] = {
			{ ParagraphItemType::Text, "Welcome to the Horus UI demo. " },
			{ ParagraphItemType::Link, "Visit the homepage" },
			{ ParagraphItemType::Text, " for more info or " },
			{ ParagraphItemType::Link, "read the documentation" },
			{ ParagraphItemType::Text, " to learn about every widget. You can " },
			{ ParagraphItemType::Button, "Apply" },
			{ ParagraphItemType::Text, " or " },
			{ ParagraphItemType::Button, "Reset" },
			{ ParagraphItemType::Text, " the settings at any time. " },
			{ ParagraphItemType::Image, "", msgBoxImg.image },
			{ ParagraphItemType::Text, " This icon highlights the helpful tips in this guide." },
		};

		paragraphDraw(helpParagraph, (u32)(sizeof(helpParagraph) / sizeof(helpParagraph[0])), imageHeight, demo.paragraphLinkClicks, demo.paragraphButtonClicks);

		char paraCountBuf[128];
		snprintf(paraCountBuf, sizeof(paraCountBuf), "Paragraph links clicked: %d, buttons clicked: %d", demo.paragraphLinkClicks, demo.paragraphButtonClicks);
		label(paraCountBuf);
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// tabs
	// ------------------------------------------------------------------
	if (expandableBegin("Tabs", &demo.expandTabs))
	{
		check("Disable##Tabs", &demo.disableTabs);
		widgetPushDisabled(demo.disableTabs);

		tabGroupBegin(demo.selectedTab);
		tab("Tab 1", 0);
		tab("Tab 2", 0);
		tab("Tab 3", 0);
		demo.selectedTab = tabGroupEnd();

		switch (demo.selectedTab)
		{
		case 0:
			label("Content of Tab 1");
			button("Tab 1 Button");
			break;
		case 1:
			label("Content of Tab 2");
			check("Tab 2 Check", &demo.checkA);
			break;
		case 2:
			label("Content of Tab 3");
			label("Just some text in tab 3");
			break;
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// box
	// ------------------------------------------------------------------
	if (expandableBegin("Box", &demo.expandBox))
	{
		check("Disable##Box", &demo.disableBox);
		widgetPushDisabled(demo.disableBox);

		label("Box with default element:");
		boxBegin("##boxDefault", Color::white);
		label("Content inside the box");
		button("Box Button");
		boxEnd();

		space();
		label("Tinted box:");
		boxBegin("##boxTinted", Color(0.3f, 0.8f, 0.5f, 1.0f));
		label("Green tinted box content");
		boxEnd();
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// color Picker
	// ------------------------------------------------------------------
	if (expandableBegin("Color Picker", &demo.expandColorPicker))
	{
		check("Disable##ColorPicker", &demo.disableColorPicker);
		widgetPushDisabled(demo.disableColorPicker);

		label("Color picker:");
		colorPicker("##cpDefault", &demo.pickerColor);

		space();
		label("Color picker popup (instant):");
		colorPickerPopup("##cpPopup", &demo.pickerColor);

		space();
		label("Color picker popup (OK/Cancel):");
		colorPickerPopup("##cpPopupOkCancel", &demo.pickerColor, ColorPickerFlags::PopupApplyButtons);

		space();
		label("Color picker with palette + custom swatches:");
		colorPicker("##cpPalette", &demo.pickerColor,
			ColorPickerFlags::ShowPalette, nullptr,
			demo.customPickerColors, &demo.customPickerColorCount, 16);

		space();
		label("Color picker popup with palette:");
		colorPickerPopup("##cpPopupPalette", &demo.pickerColor,
			ColorPickerFlags::ShowPalette, nullptr,
			demo.customPickerColors, &demo.customPickerColorCount, 16);

		char buf[128];
		snprintf(buf, sizeof(buf), "R: %.2f G: %.2f B: %.2f A: %.2f",
			demo.pickerColor.r, demo.pickerColor.g, demo.pickerColor.b, demo.pickerColor.a);
		label(buf);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// vec Editors
	// ------------------------------------------------------------------
	if (expandableBegin("Vector Editors", &demo.expandVecEditors))
	{
		check("Disable##VecEditors", &demo.disableVecEditors);
		widgetPushDisabled(demo.disableVecEditors);

		label("vec2 (float, auto select all):");
		vec2Editor("##v2f", demo.vec2x, demo.vec2y, 0.03f, VectorEditorFlags::AutoSelectAll);

		space();
		label("vec3 (float):");
		vec3Editor("##v3f", demo.vec3x, demo.vec3y, demo.vec3z);

		space();
		label("vec2 (double):");
		vec2Editor("##v2d", demo.dvec2x, demo.dvec2y);

		space();
		label("vec3 (double):");
		vec3Editor("##v3d", demo.dvec3x, demo.dvec3y, demo.dvec3z);

		space();
		label("vec3 multiple selection (indeterminate components show 'Indeterminate' as input hint, drag is disabled, empty edit returns FLT_MAX):");

		// simulate two selected objects: a component that differs between them
		// is flagged indeterminate and shows the indeterminate text
		check("X differs", &demo.multiDiffX);
		sameLine();
		check("Y differs", &demo.multiDiffY);
		sameLine();
		check("Z differs", &demo.multiDiffZ);

		VectorEditorFlags multiFlags = VectorEditorFlags::None;

		if (demo.multiDiffX)
			multiFlags |= VectorEditorFlags::IndeterminateX;
		if (demo.multiDiffY)
			multiFlags |= VectorEditorFlags::IndeterminateY;
		if (demo.multiDiffZ)
			multiFlags |= VectorEditorFlags::IndeterminateZ;

		f32 multiX = demo.multiDiffX ? FLT_MAX : demo.multiBaseX;
		f32 multiY = demo.multiDiffY ? FLT_MAX : demo.multiBaseY;
		f32 multiZ = demo.multiDiffZ ? FLT_MAX : demo.multiBaseZ;

		if (vec3Editor("##multiPos", multiX, multiY, multiZ, 0.03f, multiFlags, 6, "Indeterminate"))
		{
			// a committed edit applies to all selected objects and clears the
			// indeterminate state for that component
			if (multiX != FLT_MAX)
			{
				demo.multiBaseX = multiX;
				demo.multiDiffX = false;
			}
			if (multiY != FLT_MAX)
			{
				demo.multiBaseY = multiY;
				demo.multiDiffY = false;
			}
			if (multiZ != FLT_MAX)
			{
				demo.multiBaseZ = multiZ;
				demo.multiDiffZ = false;
			}
		}

		char multiResult[96];
		snprintf(multiResult, sizeof(multiResult), "Shared value: (%.2f, %.2f, %.2f)", demo.multiBaseX, demo.multiBaseY, demo.multiBaseZ);
		label(multiResult);
		widgetPopDisabled();
		expandableEnd();
	}
	// ------------------------------------------------------------------
	// images & Textures
	// ------------------------------------------------------------------
	if (expandableBegin("Images & Textures", &demo.expandImages))
	{
		check("Disable##Images", &demo.disableImages);
		widgetPushDisabled(demo.disableImages);

		if (!demo.demoImage)
		{
			demo.demoImage = themeGetImage(themeGet(), "__WHITEIMAGE__");
		}

		label("Basic image (KeepAspect):");
		image(demo.demoImage, 64);

		space();
		label("Image Stretch:");
		image(demo.demoImage, 64, HAlignType::Center, VAlignType::Center, ImageFitType::Stretch);

		space();
		label("Image Center alignment:");
		image(demo.demoImage, 64, HAlignType::Center);

		space();
		label("Image Left alignment:");
		image(demo.demoImage, 64, HAlignType::Left);

		space();
		label("Image Right alignment:");
		image(demo.demoImage, 64, HAlignType::Right);

		space();
		label("Images on the same line:");
		image(demo.demoImage, 32, HAlignType::Left);
		sameLine();
		image(demo.demoImage, 32);
		sameLine();
		image(demo.demoImage, 32);

		space();
		label("Texture widget (using color check checkers image as texture):");
		// just using an image as a texture for demo purposes if no real texture available
		
		texture(themeGetAtlasTexture(), 64, 64, 64);
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// menus
	// ------------------------------------------------------------------
	if (expandableBegin("Menus", &demo.expandMenus))
	{
		check("Disable##Menus", &demo.disableMenus);
		widgetPushDisabled(demo.disableMenus);

		label("Menu Bar (Nested below):");
		if (menuBarBegin())
		{
			if (menuBegin("File"))
			{
				if (menuItem("New", "Ctrl+N")) {}
				if (menuItem("Open", "Ctrl+O")) {}
				widgetPushDisabled(true);
				if (menuItem("Save", "Ctrl+S")) {}
				widgetPopDisabled();
				menuSeparator();
				if (menuItem("Exit", "Alt+F4")) {}
				menuEnd();
			}
			if (menuBegin("Edit"))
			{
				if (menuItem("Cut", "Ctrl+X")) {}
				if (menuItem("Copy", "Ctrl+C")) {}
				if (menuItem("Paste", "Ctrl+V")) {}
				menuEnd();
			}
			menuBarEnd();
		}

		space();
		label("Context Menu (Right click the label below):");
		label("Right click me!");
		if (contextMenuBegin())
		{
			if (menuItem("Action 1")) {}
			if (menuItem("Action 2")) {}
			menuSeparator();
			if (menuBegin("Sub Menu"))
			{
				if (menuItem("Sub Action 1")) {}
				menuEnd();
			}
			contextMenuEnd();
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// viewport
	// ------------------------------------------------------------------
	if (expandableBegin("Viewport", &demo.expandViewport))
	{
		check("Disable##Viewport", &demo.disableViewport);
		widgetPushDisabled(demo.disableViewport);

		label("A custom viewport area (100px height):");
		Rect vprect = viewportBegin("##demoViewport", 100);
		// in a real app, you'd use vprect to draw your 3D scene/etc.
		renderDrawSolidRectangle(vprect);
		renderDrawTextInBox("Custom Viewport Content", vprect, HAlignType::Center, VAlignType::Center);
		viewportEnd();
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// custom Widget
	// ------------------------------------------------------------------
	if (expandableBegin("Custom Widget", &demo.expandCustomWidget))
	{
		check("Disable##CustomWidget", &demo.disableCustomWidget);
		widgetPushDisabled(demo.disableCustomWidget);

		label("Custom widget area:");
		Rect cwRect = customWidgetBegin("##demoCustom", 50);
		renderDrawRectangle(cwRect);
		renderDrawLine(cwRect.topLeft(), cwRect.bottomRight());
		renderDrawLine(cwRect.topRight(), cwRect.bottomLeft());
		customWidgetEnd();
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// object Reference
	// ------------------------------------------------------------------
	if (expandableBegin("Object Reference Editor", &demo.expandObjectRef))
	{
		enum MyTypeIds
		{
			MyTypeId1,
			MyTypeId2,
			MyTypeId3
		};

		check("Disable##ObjectRef", &demo.disableObjectRef);
		widgetPushDisabled(demo.disableObjectRef);


		std::string v1;
		std::string v2;
		std::string v3;
		std::string vEmpty;

		if (demo.objectRefValue1)
		{
			v1 = *(std::string*)demo.objectRefValue1;
		}

		if (demo.objectRefValue2)
		{
			v2 = *(std::string*)demo.objectRefValue2;
		}

		if (demo.objectRefValue3)
		{
			v3 = *(std::string*)demo.objectRefValue3;
		}

		if (demo.objectRefEmpty)
		{
			vEmpty = *(std::string*)demo.objectRefEmpty;
		}

		label("Without custom button images:");
		{
			static std::string refVal2a = "Mesh01";
			static std::string refVal2b = "Mesh02";
			static std::string refVal2c = "ArchVizModel";
			const char* refNames2[] = { "Mesh01", "Mesh02", "ArchVizModel" };
			void* refVals2[] = { &refVal2a, &refVal2b, &refVal2c };
			objectRefEditor("##demoObjRefNoIcons", 0, 0, 0, "MyObjectType2", v2.c_str(), MyTypeId2, &demo.objectRefValue2, &demo.objectRefModified2, 3, refNames2, refVals2);
		}
		space();

		{
			label("With indeterminate text (no object assigned yet):");
			static std::string refValP1 = "Mesh01";
			static std::string refValP2 = "Mesh02";
			static std::string refValP3 = "ArchVizModel";
			const char* refNamesP[] = { "Mesh01", "Mesh02", "ArchVizModel" };
			void* refValsP[] = { &refValP1, &refValP2, &refValP3 };
			objectRefEditor("##demoObjRefPlaceholder", 0, 0, 0, "MyObjectType2", vEmpty.c_str(), MyTypeId2, &demo.objectRefEmpty, &demo.objectRefEmptyModified, 3, refNamesP, refValsP, 0, "Indeterminate");
		}
		space();

		{
			label("With icon:");
			HImage icon = themeGetImage(themeGet(), "../themes/default/sign-info.png");
			objectRefEditor("##demoObjRefWithIcon", 0, 0, icon ? icon : themeGetImage(themeGet(), "__WHITEIMAGE__"), "MyObjectType3", v3.c_str(), MyTypeId3, &demo.objectRefValue3, &demo.objectRefModified3);
		}
		space();

		{
			label("With icon (custom size 44):");
			HImage icon = themeGetImage(themeGet(), "../themes/default/sign-info.png");
			objectRefEditor("##demoObjRefWithIconSize", 0, 0, icon ? icon : themeGetImage(themeGet(), "__WHITEIMAGE__"), "MyObjectType3", v3.c_str(), MyTypeId3, &demo.objectRefValue3, &demo.objectRefModified3, 0, nullptr, nullptr, 44);
		}
		space();

		label("Object reference editor (with custom button images):");
		objectRefEditor("##demoObjRef", themeGetImage(themeGet(), "__WHITEIMAGE__"), themeGetImage(themeGet(), "__WHITEIMAGE__"), 0, "MyObjectType1", v1.c_str(), MyTypeId1, &demo.objectRefValue1, &demo.objectRefModified1);

		label("Drag source (drag button into the proper editor):");
		{
			static std::string dragSampleObject1 = "ShinyMetalA";
			button("MyObjectType1##dragSrc");
			if (dragDropWantsTo())
			{
				dragDropBegin(MyTypeId1, &dragSampleObject1);
			}
			static std::string dragSampleObject2 = "Mesh01";
			button("MyObjectType2##dragSrc");
			if (dragDropWantsTo())
			{
				dragDropBegin(MyTypeId2, &dragSampleObject2);
			}
			static std::string dragSampleObject3 = "SkyShader";
			button("MyObjectType3##dragSrc");
			if (dragDropWantsTo())
			{
				dragDropBegin(MyTypeId3, &dragSampleObject3);
			}
		}
		widgetPopDisabled();
		expandableEnd();
	}
	// ------------------------------------------------------------------
	// drag & Drop API Demo
	// ------------------------------------------------------------------
	if (expandableBegin("Drag & Drop API Demo", &demo.expandDragDrop))
	{
		label("Drag sources:");

		sameLine(0, 20);
		tintPush(Color::orange, TintColorType::Text);
		if (button(("Type A (int): " + std::to_string(demo.dragDropSrcA)).c_str()))
			demo.dragDropSrcA++;
		tintPop();
		if (dragDropWantsTo())
		{
			static int dragObjA = 0;
			dragObjA = demo.dragDropSrcA;
			dragDropBegin(1, &dragObjA);
		}

		sameLine(0, 10);
		tintPush(Color::sky, TintColorType::Text);
		if (button(("Type B (int): " + std::to_string(demo.dragDropSrcB)).c_str()))
			demo.dragDropSrcB++;
		tintPop();
		if (dragDropWantsTo())
		{
			static int dragObjB = 0;
			dragObjB = demo.dragDropSrcB;
			dragDropBegin(2, &dragObjB);
		}

		space();
		label("Drop Targets");
		space();

		label("Target A (accepts type 1):");
		{
			std::string ddTxt;
			if (demo.dragDropTargetAValue)
				ddTxt = "Dropped: " + std::to_string(*(int*)demo.dragDropTargetAValue);
			else
				ddTxt = "Drop type 1 here";
			tintPush(Color::orange, TintColorType::Text);
			label((ddTxt + "##ddTargetA").c_str());
			tintPop();

			if (dragDropGetObjectType() == 1)
				dragDropAllow();

			if (dragDropDroppedOnWidget() && dragDropGetObjectType() == 1)
			{
				static int val;
				val = *(int*)dragDropGetObject();
				demo.dragDropTargetAValue = &val;
				dragDropEnd();
				forceRepaint();
			}
		}

		space();

		label("Target B (accepts type 2):");
		{
			std::string ddTxt;
			if (demo.dragDropTargetBValue)
				ddTxt = "Dropped: " + std::to_string(*(int*)demo.dragDropTargetBValue);
			else
				ddTxt = "Drop type 2 here";
			tintPush(Color::sky, TintColorType::Text);
			label((ddTxt + "##ddTargetB").c_str());
			tintPop();

			if (dragDropGetObjectType() == 2)
				dragDropAllow();

			if (dragDropDroppedOnWidget() && dragDropGetObjectType() == 2)
			{
				static int val;
				val = *(int*)dragDropGetObject();
				demo.dragDropTargetBValue = &val;
				dragDropEnd();
				forceRepaint();
			}
		}

		space();

		label("Target C (accepts types 1 & 2):");
		{
			std::string ddTxt;
			if (demo.dragDropTargetCValue)
				ddTxt = "Dropped: " + std::to_string(*(int*)demo.dragDropTargetCValue);
			else
				ddTxt = "Drop type 1 or 2 here";
			tintPush(Color::yellow, TintColorType::Text);
			label((ddTxt + "##ddTargetC").c_str());
			tintPop();

			u32 type = dragDropGetObjectType();
			if (type == 1 || type == 2)
				dragDropAllow();

			if (dragDropDroppedOnWidget() && (type == 1 || type == 2))
			{
				static int val;
				val = *(int*)dragDropGetObject();
				demo.dragDropTargetCValue = &val;
				dragDropEnd();
				forceRepaint();
			}
		}

		space();
		label("Tip: drag from Type A or Type B buttons into the drop targets above.");

		space();
		label("Drag items between lists:");
		space();

		// deferred move: avoid modifying lists during iteration
		static std::string pendingMoveStr;
		static bool pendingMoveToListA = false;

		{
			std::vector<const char*> itemsA;
			for (auto& s : demo.dragListA)
				itemsA.push_back(s.c_str());

			paddingPush(PaddingType::Layout, Point(0, 0));
			paddingPush(PaddingType::ScrollView, Point(0, 0));
			idPush("dragListA");
			scrollViewBegin("listScrollView", 150, demo.dragScrollListA.y, 0, ScrollViewFlags::NoHorizontalScroll);

			spacingPush(0.0f);
			for (u32 i = 0; i < (u32)itemsA.size(); i++)
			{
				bool isSelected = i == (u32)demo.dragListIdxA;
				if (selectable(itemsA[i], isSelected ? SelectableFlags::Selected : SelectableFlags::Normal))
					demo.dragListIdxA = i;

				if (dragDropWantsTo())
					dragDropBegin(3, (void*)itemsA[i]);
			}
			spacingPop();

			demo.dragScrollListA = scrollViewEnd();
			idPop();
			paddingPop(PaddingType::ScrollView);
			paddingPop(PaddingType::Layout);

			if (dragDropGetObjectType() == 3)
			{
				bool isSelfDrop = false;
				void* draggedObj = dragDropGetObject();
				for (auto& s : demo.dragListA)
					if (s.c_str() == (const char*)draggedObj) { isSelfDrop = true; break; }
				if (!isSelfDrop)
					dragDropAllow();
			}

			if (dragDropDroppedOnWidget() && dragDropGetObjectType() == 3)
			{
				const char* droppedStr = (const char*)dragDropGetObject();
				for (auto& s : demo.dragListB)
				{
					if (s.c_str() == droppedStr)
					{
						pendingMoveStr = s;
						pendingMoveToListA = true;
						dragDropEnd();
						forceRepaint();
						break;
					}
				}
			}
		}

		{
			std::vector<const char*> itemsB;
			for (auto& s : demo.dragListB)
				itemsB.push_back(s.c_str());

			paddingPush(PaddingType::Layout, Point(0, 0));
			paddingPush(PaddingType::ScrollView, Point(0, 0));
			idPush("dragListB");
			scrollViewBegin("listScrollView", 150, demo.dragScrollListB.y, 0, ScrollViewFlags::NoHorizontalScroll);

			spacingPush(0.0f);
			for (u32 i = 0; i < (u32)itemsB.size(); i++)
			{
				bool isSelected = i == (u32)demo.dragListIdxB;
				if (selectable(itemsB[i], isSelected ? SelectableFlags::Selected : SelectableFlags::Normal))
					demo.dragListIdxB = i;

				if (dragDropWantsTo())
					dragDropBegin(3, (void*)itemsB[i]);
			}
			spacingPop();

			demo.dragScrollListB = scrollViewEnd();
			idPop();
			paddingPop(PaddingType::ScrollView);
			paddingPop(PaddingType::Layout);

			if (dragDropGetObjectType() == 3)
			{
				bool isSelfDrop = false;
				void* draggedObj = dragDropGetObject();
				for (auto& s : demo.dragListB)
					if (s.c_str() == (const char*)draggedObj) { isSelfDrop = true; break; }
				if (!isSelfDrop)
					dragDropAllow();
			}

			if (dragDropDroppedOnWidget() && dragDropGetObjectType() == 3)
			{
				const char* droppedStr = (const char*)dragDropGetObject();
				for (auto& s : demo.dragListA)
				{
					if (s.c_str() == droppedStr)
					{
						pendingMoveStr = s;
						pendingMoveToListA = false;
						dragDropEnd();
						forceRepaint();
						break;
					}
				}
			}
		}

		// apply deferred move
		if (!pendingMoveStr.empty())
		{
			if (pendingMoveToListA)
			{
				for (size_t i = 0; i < demo.dragListB.size(); i++)
				{
					if (demo.dragListB[i] == pendingMoveStr)
					{
						demo.dragListA.push_back(std::move(demo.dragListB[i]));
						demo.dragListB.erase(demo.dragListB.begin() + i);
						break;
					}
				}
			}
			else
			{
				for (size_t i = 0; i < demo.dragListA.size(); i++)
				{
					if (demo.dragListA[i] == pendingMoveStr)
					{
						demo.dragListB.push_back(std::move(demo.dragListA[i]));
						demo.dragListA.erase(demo.dragListA.begin() + i);
						break;
					}
				}
			}
			pendingMoveStr.clear();
		}

		space();
		space();
		label("Drag the white image into a slot:");

		auto* whiteImg = themeGetImage(themeGet(), "__WHITEIMAGE__");

		sameLine(0, 20);
		image(whiteImg, 64, HAlignType::Left);
		if (dragDropWantsTo())
		{
			static int payload = 0;
			dragDropBegin(99, &payload);
		}

		space();

		for (int i = 0; i < 3; i++)
		{
			if (i > 0) sameLine(0, 10);

			widgetSetNextWidth(64);
			Rect r = customWidgetBegin(("##slotRect" + std::to_string(i)).c_str(), 64);

			if (demo.imageSlots[i])
			{
				renderSetFillStyle(Color::fromU8(255, 255, 255));
				renderDrawStretchedImage(whiteImg, r);
			}
			else
			{
				renderSetFillStyle(Color::fromU8(150, 50, 50));
				renderDrawSolidRectangle(r);
			}
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", i);
			renderSetColor(Color::fromU8(255, 255, 255));
			renderDrawTextInBox(buf, r, HAlignType::Center, VAlignType::Center);

			customWidgetEnd();

			// only accept drops when drag is active AND the mouse is over THIS slot
			if (dragDropGetObjectType() == 99 && widgetIsHovered())
				dragDropAllow();

			if (dragDropDroppedOnWidget() && dragDropGetObjectType() == 99)
			{
				demo.imageSlots[i] = true;
				dragDropEnd();
				forceRepaint();
			}
		}

		space();
		if (button("Reset"))
		{
			for (int i = 0; i < 3; i++)
				demo.imageSlots[i] = false;
		}

		// drag preview: show the white image at cursor while dragging
		if (dragDropGetObjectType() == 99)
		{
			Point cursor = mouseGetPosition();
			renderDrawStretchedImage(whiteImg, Rect(cursor.x - 32, cursor.y - 32, 64, 64));
		}

		expandableEnd();
	}
	// ------------------------------------------------------------------
	// virtual List
	// ------------------------------------------------------------------
	if (expandableBegin("Virtual List (1000 items)", &demo.expandVirtualList))
	{
		check("Disable##VirtualList", &demo.disableVirtualList);
		widgetPushDisabled(demo.disableVirtualList);

		// persistent virtual list state: only provide item count here
		static hui::VirtualScrollInfo vinfo(1000); 
		static f32 virtualListScrollPos = 0.0f;

		scrollViewBegin("##virtualListScroll", 150, virtualListScrollPos);

		virtualListContentBegin(vinfo);

		while (vinfo.nextStep())
		{
			for (u32 i = vinfo.startIndex; i <= vinfo.endIndex; ++i)
			{
				char buf[64];
				snprintf(buf, sizeof(buf), "Item %u", i);
				selectable(buf);
			}
		}
		virtualListContentEnd();

		virtualListScrollPos = scrollViewEnd().y;
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// scroll View
	// ------------------------------------------------------------------
	if (expandableBegin("Scroll View", &demo.expandScrollView))
	{
		check("Disable##ScrollView", &demo.disableScrollView);
		widgetPushDisabled(demo.disableScrollView);

		label("A nested scroll view (150px height):");
		scrollViewBegin("##nestedSV", 150, demo.scrollPos.y);
		{
			for (int i = 0; i < 20; ++i)
			{
				char buf[64];
				snprintf(buf, sizeof(buf), "Inside ScrollView %d", i);
				button(buf);
			}
		}
		demo.scrollPos = scrollViewEnd();
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// table
	// ------------------------------------------------------------------
	if (expandableBegin("Tables", &demo.expandTable))
	{
		check("Disable##Table", &demo.disableTable);
		widgetPushDisabled(demo.disableTable);

		label("Basic table (3 columns):");
		if (tableBegin("##demoTable", 3, 200, TableFlags::Borders | TableFlags::Resizable | TableFlags::Reorderable | TableFlags::AltRowBg | TableFlags::FixedSize))
		{
			tableColumnSetup(0, 50, TableColumnFlags::FixedResize);
			tableColumnSetup(1, 150, TableColumnFlags::Stretch);
			tableColumnSetup(2, 100, TableColumnFlags::Stretch);

			tableStartHeader();
			label("ID"); tableCellNext(); 
			label("Name"); tableCellNext(); 
			label("Status");

			for (int i = 0; i < 10; ++i)
			{
				tableRowNext();
				char idBuf[16], nameBuf[32];
				snprintf(idBuf, sizeof(idBuf), "%d", i + 1);
				snprintf(nameBuf, sizeof(nameBuf), "Item %d", i + 1);

				label(idBuf); tableCellNext(); 
				label(nameBuf); tableCellNext(); 
				label(i % 2 == 0 ? "Active" : "Inactive");
			}
			tableEnd();
		}

		label("Table with merged (colspan) cells:");
		if (tableBegin("##mergedTable", 3, 0, TableFlags::Borders | TableFlags::AltRowBg | TableFlags::FixedSize))
		{
			tableColumnSetup(0, 100, TableColumnFlags::FixedResize);
			tableColumnSetup(1, 120, TableColumnFlags::Stretch);
			tableColumnSetup(2, 120, TableColumnFlags::Stretch);

			tableStartHeader();
			label("Col A"); tableCellNext(); 
			label("Col B"); tableCellNext(); 
			label("Col C");

			tableRowNext();
			tableCellNext(2);
			label("Merged row spanning columns A+B", HAlignType::Center);

			tableRowNext();
			label("A"); tableCellNext();
			label("B"); tableCellNext();
			label("C");

			tableRowNext();
			tableCellNext(3);
			label("Row spanning all three columns", HAlignType::Center);

			tableEnd();
		}

		label("Invisible table as 3-column list layout:");
		if (tableBegin("##invisibleTable", 3, 0, TableFlags::Stretch | TableFlags::Borders))
		{
			tableColumnSetup(0, 0, TableColumnFlags::Stretch);
			tableColumnSetup(1, 0, TableColumnFlags::Stretch);
			tableColumnSetup(2, 0, TableColumnFlags::Stretch);

			tableRowNext();

			// col A
			{
				std::vector<const char*> items;
				for (auto& s : demo.tableListA) items.push_back(s.c_str());
				spacingPush(0.0f);
				for (u32 i = 0; i < (u32)items.size(); i++)
				{
					bool sel = i == (u32)demo.tableListIdxA;
					if (selectable(items[i], sel ? SelectableFlags::Selected : SelectableFlags::Normal))
						demo.tableListIdxA = i;
				}
				spacingPop();
			}

			tableCellNext();

			// col B
			{
				std::vector<const char*> items;
				for (auto& s : demo.tableListB) items.push_back(s.c_str());
				spacingPush(0.0f);
				for (u32 i = 0; i < (u32)items.size(); i++)
				{
					bool sel = i == (u32)demo.tableListIdxB;
					if (selectable(items[i], sel ? SelectableFlags::Selected : SelectableFlags::Normal))
						demo.tableListIdxB = i;
				}
				spacingPop();
			}

			tableCellNext();

			// col C
			{
				std::vector<const char*> items;
				for (auto& s : demo.tableListC) items.push_back(s.c_str());
				spacingPush(0.0f);
				for (u32 i = 0; i < (u32)items.size(); i++)
				{
					bool sel = i == (u32)demo.tableListIdxC;
					if (selectable(items[i], sel ? SelectableFlags::Selected : SelectableFlags::Normal))
						demo.tableListIdxC = i;
				}
				spacingPop();
			}

			tableEnd();
		}

		space();

		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// property grid (mesh inspector)
	// ------------------------------------------------------------------
	if (expandableBegin("Property Grid (Mesh Inspector)", &demo.expandPropertyGrid))
	{
		check("Disable##PropertyGrid", &demo.disablePropertyGrid);
		check("Alternate Row Background##PropertyGrid", &demo.pgAltRowBg);
		widgetPushDisabled(demo.disablePropertyGrid);

		static const char* layers[] = { "Default", "Environment", "Player", "Enemy", "UI" };
		static const char* lodLevels[] = { "LOD 0", "LOD 1", "LOD 2", "LOD 3" };
		static const char* shadingModes[] = { "Unlit", "Lambert", "Blinn-Phong", "PBR" };
		static const char* cullModes[] = { "None", "Back", "Front" };

		label("A two-column property table with expandable groups:");
		space();

		if (tableBegin("##propGrid", 2, 0, TableFlags::Borders | TableFlags::Resizable | TableFlags::FixedSize | (demo.pgAltRowBg ? TableFlags::AltRowBg : TableFlags::None)))
		{
			tableColumnSetup(0, 150, TableColumnFlags::FixedResize);
			tableColumnSetup(1, 0, TableColumnFlags::Stretch);
			tableCellPaddingPush(10.0f, 6.0f);

			// ---- identity ----
			tableRowNext();
			tableCellNext(2);
			bool identityOpen = expandable("Identity", &demo.pgGroupIdentity);

			if (identityOpen)
			{
				propGridRow("Name", [&]() { textInput("##pgName", demo.pgMeshName, sizeof(demo.pgMeshName)); });
				propGridRow("Tag", [&]() { textInput("##pgTag", demo.pgTag, sizeof(demo.pgTag)); });
				propGridRow("Layer", [&]() { dropdown("##pgLayer", demo.pgLayer, layers, 5); });
				propGridRow("Mesh Asset", [&]() { label("assets/models/monkey.obj"); });
			}

			// ---- geometry ----
			tableRowNext();
			tableCellNext(2);
			bool geometryOpen = expandable("Geometry", &demo.pgGroupGeometry);

			if (geometryOpen)
			{
				propGridRow("Vertices", [&]() {
					char buf[64];
					snprintf(buf, sizeof(buf), "%d", demo.pgVertexCount);
					label(buf);
				});
				propGridRow("Triangles", [&]() {
					char buf[64];
					snprintf(buf, sizeof(buf), "%d", demo.pgTriangleCount);
					label(buf);
				});
				propGridRow("LOD Level", [&]() { dropdown("##pgLod", demo.pgLodLevel, lodLevels, 4); });
			}

			// ---- transform ----
			tableRowNext();
			tableCellNext(2);
			bool transformOpen = expandable("Transform", &demo.pgGroupTransform);

			if (transformOpen)
			{
				propGridRow("Position", [&]() { vec3Editor("##pgPos", demo.pgPosX, demo.pgPosY, demo.pgPosZ); });
				propGridRow("Rotation", [&]() { vec3Editor("##pgRot", demo.pgRotX, demo.pgRotY, demo.pgRotZ); });
				propGridRow("Scale", [&]() { vec3Editor("##pgScale", demo.pgScaleX, demo.pgScaleY, demo.pgScaleZ); });
				propGridRow("Bounds Min", [&]() { vec3Editor("##pgBMin", demo.pgBoundsMinX, demo.pgBoundsMinY, demo.pgBoundsMinZ); });
				propGridRow("Bounds Max", [&]() { vec3Editor("##pgBMax", demo.pgBoundsMaxX, demo.pgBoundsMaxY, demo.pgBoundsMaxZ); });
			}

			// ---- material ----
			tableRowNext();
			tableCellNext(2);
			bool materialOpen = expandable("Material", &demo.pgGroupMaterial);

			if (materialOpen)
			{
				propGridRow("Albedo", [&]() { colorPickerPopup("##pgAlbedo", &demo.pgAlbedoColor); });
				propGridRow("Emission", [&]() { colorPickerPopup("##pgEmission", &demo.pgEmissionColor); });
				propGridRow("Roughness", [&]() { sliderFloat("##pgRough", 0.0f, 1.0f, demo.pgRoughness); });
				propGridRow("Metalness", [&]() { sliderFloat("##pgMetal", 0.0f, 1.0f, demo.pgMetalness); });
				propGridRow("Opacity", [&]() { sliderFloat("##pgOpacity", 0.0f, 1.0f, demo.pgOpacity); });
				propGridRow("Shading Mode", [&]() { dropdown("##pgShading", demo.pgShadingMode, shadingModes, 4); });
				propGridRow("Texture Map", [&]() {
					WidgetElementInfo btnBody;
					themeGetWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, btnBody);
					f32 btnWidth = (btnBody.border * 2.0f + renderGetTextSize("Browse").x) * scaleGet();
					f32 spacingPx = 5.0f * scaleGet();
					widgetSetNextWidth((layoutGetSize().x - btnWidth - spacingPx) / scaleGet());
					textInput("##pgTexture", demo.pgTexturePath, sizeof(demo.pgTexturePath));
					sameLine();
					if (button("Browse"))
					{
						char path[512];
						if (openFileDialog("*.*;*.png;*.jpg;*.jpeg;*.bmp;*.tga", "", path, sizeof(path)))
							snprintf(demo.pgTexturePath, sizeof(demo.pgTexturePath), "%s", path);
					}
				});
			}

			// ---- render ----
			tableRowNext();
			tableCellNext(2);
			bool renderOpen = expandable("Render", &demo.pgGroupRender);

			if (renderOpen)
			{
				propGridRow("Cast Shadows", [&]() {
					idPush("pgCastShadows");
					check("", &demo.pgCastShadows);
					idPop();
				});
				propGridRow("Receive Shadows", [&]() {
					idPush("pgReceiveShadows");
					check("", &demo.pgReceiveShadows);
					idPop();
				});
				propGridRow("Double-Sided", [&]() {
					idPush("pgDoubleSided");
					check("", &demo.pgDoubleSided);
					idPop();
				});
				propGridRow("Smooth Normals", [&]() {
					idPush("pgSmoothNormals");
					check("", &demo.pgSmoothNormals);
					idPop();
				});
				propGridRow("Wireframe Overlay", [&]() {
					idPush("pgWireframe");
					check("", &demo.pgWireframe);
					idPop();
				});
				propGridRow("Frustum Culling", [&]() {
					idPush("pgFrustumCulling");
					check("", &demo.pgFrustumCulling);
					idPop();
				});
				propGridRow("Cull Mode", [&]() { buttonGroup("##pgCull", cullModes, 3, &demo.pgCullMode); });
				propGridRow("Draw Priority", [&]() { comboSliderIntRanged(&demo.pgDrawPriority, 0, 1000, 1.0f, 10, "Priority %.0f"); });
			}

			tableCellPaddingPop();
			tableEnd();
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// tooltip
	// ------------------------------------------------------------------
	if (expandableBegin("Tooltips", &demo.expandTooltip))
	{
		check("Disable##Tooltip", &demo.disableTooltip);
		widgetPushDisabled(demo.disableTooltip);

		label("Hover me for a basic tooltip:");
		button("Hover Me (Basic)");
		tooltip("This is a basic text tooltip!");

		space();
		label("Hover me for a custom tooltip:");
		button("Hover Me (Custom)");
		if (customTooltipBegin(250))
		{
			// the tooltip body is white so labels and checks need dark text here,
			// the button keeps its theme default text color.
			// checks draw text with the state text color (hovered/checked/...),
			// so all their states are overridden and then restored
			WidgetElementId tooltipElements[] = { WidgetElementId::LabelBody, WidgetElementId::CheckBody };
			const WidgetStateType tooltipStates[] = { WidgetStateType::Normal, WidgetStateType::Focused, WidgetStateType::Pressed, WidgetStateType::Hovered, WidgetStateType::Disabled };
			const u32 tooltipElementCount = sizeof(tooltipElements) / sizeof(tooltipElements[0]);
			const u32 tooltipStateCount = sizeof(tooltipStates) / sizeof(tooltipStates[0]);

			WidgetElementInfo originalTextColors[tooltipElementCount][tooltipStateCount];

			for (u32 i = 0; i < tooltipElementCount; i++)
			{
				for (u32 s = 0; s < tooltipStateCount; s++)
				{
					WidgetElementInfo info;
					themeGetWidgetElementInfo(tooltipElements[i], tooltipStates[s], info);
					originalTextColors[i][s] = info;
					info.textColor = Color::black;
					themeSetWidgetElement(themeGet(), tooltipElements[i], tooltipStates[s], info, "default");
				}
			}

			label("This is a CUSTOM tooltip area");
			label("You can embed any widgets here:");
			check("Check inside tooltip", &demo.checkA);
			button("Button inside tooltip");

			for (u32 i = 0; i < tooltipElementCount; i++)
			{
				for (u32 s = 0; s < tooltipStateCount; s++)
				{
					themeSetWidgetElement(themeGet(), tooltipElements[i], tooltipStates[s], originalTextColors[i][s], "default");
				}
			}
			customTooltipEnd();
		}
		label("Tip: hold Ctrl to keep the custom tooltip open (freeze it) so you can interact with its widgets.");
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// popup
	// ------------------------------------------------------------------
	if (expandableBegin("Popups", &demo.expandPopup))
	{
		check("Disable##Popup", &demo.disablePopup);
		widgetPushDisabled(demo.disablePopup);

		label("Click the button to open a modal popup:");
		if (button("Open Popup"))
		{
			demo.showPopup = true;
		}

		if (demo.showPopup)
		{
			popupBegin("##demoPopup", 300, PopupFlags::BelowLastWidget);
			label("Welcome to the modal popup!");
			label("It's positioned below the button.");
			space();
			if (button("Close Popup") || popupMustClose())
			{
				popupClose();
				demo.showPopup = false;
			}
			popupEnd();
		}
		widgetPopDisabled();
		expandableEnd();
	}

	// ------------------------------------------------------------------
	// custom file dialog
	// ------------------------------------------------------------------
	if (expandableBegin("Custom File Dialog", &demo.expandCustomFileDialog))
	{
		check("Disable##CustomFileDialog", &demo.disableCustomFileDialog);
		widgetPushDisabled(demo.disableCustomFileDialog);

		label("A popup that browses a virtual file system:");
		static const char* cfdModeLabels[3] = { "Open File", "Save File", "Pick Folder" };
		buttonGroup("##cfdMode", cfdModeLabels, 3, &demo.customFileDialogMode);

		CustomFileDialogFlags cfdFlags = CustomFileDialogFlags::None;

		if (demo.customFileDialogMode == 1)
			cfdFlags = CustomFileDialogFlags::SaveFile;
		else if (demo.customFileDialogMode == 2)
			cfdFlags = CustomFileDialogFlags::PickFolder;

		static bool usePreview = true;
		check("Show Preview Panel", &usePreview);

		space();

		customFileDialog("##demoCustomFileDialog", demoCustomFileDialogList, nullptr, demo.customFileDialogResult, sizeof demo.customFileDialogResult, cfdFlags, usePreview ? demoCustomFileDialogPreview : nullptr, nullptr);

		std::string cfdResultLabel = "Chosen path: " + std::string(demo.customFileDialogResult);
		label(cfdResultLabel.c_str());
		widgetPopDisabled();
		expandableEnd();
	}

	demo.demoScrollPos = scrollViewEnd();
}

}
