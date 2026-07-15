#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

namespace hui
{

struct DemoState
{
	// Button
	int buttonClickCount = 0;

	// TextInput
	char textBasic[256] = "Hello World";
	char textNumeric[256] = "42";
	char textHex[256] = "FF";
	char textDefault[256] = "";
	char textPassword[256] = "secret";
	char textAutoSelect[256] = "Select me";

	// TextInput Multiline
	char multiText[4096] = "Line 1\nLine 2\nLine 3\nfloat foo = 3.14f;\nint bar = 42;";

	// Slider
	i32 sliderIntVal = 50;
	i32 sliderIntStepped = 0;
	f32 sliderFloatVal = 0.5f;
	f32 sliderFloatStepped = 0.0f;

	// ComboSlider
	i32 comboSliderInt = 0;
	i32 comboSliderIntRanged = 50;
	f32 comboSliderFloat = 0.0f;
	f32 comboSliderFloatCustomString = 0.0f;
	f32 comboSliderFloatRanged = 0.5f;

	// RotarySlider
	f32 rotaryVal = 0.5f;
	f32 rotaryTwoSide = 0.0f;
	f32 rotaryVal2 = 50.0f;
	f32 rotaryVal3 = 0.7f;

	// Progress
	f32 progressValue = 0.0f;
	f32 progressValueReal = 0.0f;

	// Check
	bool checkA = true;
	bool checkB = false;

	// Button Group
	u32 buttonGroupVal = 0;

	// Radio
	i32 radioVal = 0;

	// Dropdown
	i32 dropdownSel = 0;

	// List
	bool listSelected[5] = {};

	// Expandable states (managed internally by expandable())
	bool expandButton = true;
	bool expandTextInput = false;
	bool expandMultiline = false;
	bool expandSlider = false;
	bool expandComboSlider = false;
	bool expandRotarySlider = false;
	bool expandProgress = false;
	bool expandCheck = false;
	bool expandRadio = false;
	bool expandLabel = false;
	bool expandExpandable = false;
	bool expandDropdown = false;
	bool expandList = false;
	bool expandSelectable = false;
	bool expandSeparators = false;
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

	// Added: Tree nodes demo expand flag
	bool expandTreeNodes = false;

	// Disabled states
	bool disableButton = false;
	bool disableTextInput = false;
	bool disableMultiline = false;
	bool disableSlider = false;
	bool disableComboSlider = false;
	bool disableRotarySlider = false;
	bool disableProgress = false;
	bool disableCheck = false;
	bool disableRadio = false;
	bool disableLabel = false;
	bool disableExpandable = false;
	bool disableDropdown = false;
	bool disableList = false;
	bool disableSelectable = false;
	bool disableSeparators = false;
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

	// Added: Tree nodes demo disable flag
	bool disableTreeNodes = false;

	// Popup demo
	bool showPopup = false;

	// Tabs
	TabIndex selectedTab = 0;

	// Color picker
	Color pickerColor = Color(1.0f, 0.0f, 0.0f, 1.0f);
	Color customPickerColors[16] = {};
	u32 customPickerColorCount = 0;

	// Vec editors
	f32 vec2x = 1.0f, vec2y = 2.0f;
	f32 vec3x = 1.0f, vec3y = 2.0f, vec3z = 3.0f;
	f64 dvec2x = 1.0, dvec2y = 2.0;
	f64 dvec3x = 1.0, dvec3y = 2.0, dvec3z = 3.0;

	// Image & Texture
	HImage demoImage = nullptr;
	HTexture demoTexture = nullptr;

	// ObjectRef
	void* objectRefValue1 = nullptr;
	bool objectRefModified1 = false;
	void* objectRefValue2 = nullptr;
	bool objectRefModified2 = false;
	void* objectRefValue3 = nullptr;
	bool objectRefModified3 = false;

	// Virtual List
	bool expandVirtualList = false;
	bool disableVirtualList = false;
	i32 virtualListCount = 1000;
	
	// Drag & Drop Demo
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

	Point scrollPos = { 0, 0 };

	bool initialized = false;

	Point demoScrollPos = { 0, 0 };

	// Tree demo internal state (controlled expansion)
	bool treeRootExpanded = true;
	bool treeFolderAExpanded = false;
	bool treeFolderBExpanded = false;
};

static DemoState demo;

void showDemo()
{
	if (!demo.initialized)
	{
		demo.initialized = true;
	}

	// Animate progress bars
	f32 dt = contextGetSettings().deltaTime;
	demo.progressValue += dt * 0.1f;
	if (demo.progressValue > 1.0f) demo.progressValue = 0.0f;

	demo.progressValueReal += dt * 20.0f;
	if (demo.progressValueReal > 1000.0f) demo.progressValueReal = 0.0f;

	// Keep repainting as long as the progress bar section is expanded
	if (demo.expandProgress)
	{
		forceRepaint();
	}

	scrollViewBegin("##demoScroll", 0, demo.demoScrollPos, { 0, 0 }, ScrollViewFlags::None);

	{
		char buf[64];
		snprintf(buf, sizeof(buf), "Scale: %.2f", scaleGet());
		label(buf);
		space();
	}

	//------------------------------------------------------------------
	// Button
	//------------------------------------------------------------------
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
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Text Input
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Multiline Text Input
	//------------------------------------------------------------------
	if (expandableBegin("Multiline Text Input", &demo.expandMultiline))
	{
		check("Disable##MultilineTextInput", &demo.disableMultiline);
		widgetPushDisabled(demo.disableMultiline);

		label("Basic (10 visible lines):");
		textInputMultiline("##mtiBasic", demo.multiText, sizeof(demo.multiText), 10);

		space();
		label("With line numbers:");
		textInputMultiline("##mtiLineNums", demo.multiText, sizeof(demo.multiText), 8, MultilineTextInputFlags::LineNumbers);

		space();
		label("With word wrap:");
		textInputMultiline("##mtiWordWrap", demo.multiText, sizeof(demo.multiText), 6, MultilineTextInputFlags::WordWrap);

		space();
		label("Line numbers + highlight current line:");
		textInputMultiline("##mtiHighlight", demo.multiText, sizeof(demo.multiText), 8,
			MultilineTextInputFlags::LineNumbers | MultilineTextInputFlags::HighlightCurrentLine);
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Slider
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// ComboSlider
	//------------------------------------------------------------------
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
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Rotary Slider
	//------------------------------------------------------------------
	if (expandableBegin("Rotary Sliders", &demo.expandRotarySlider))
	{
		check("Disable##RotarySlider", &demo.disableRotarySlider);
		widgetPushDisabled(demo.disableRotarySlider);

		rotarySliderFloat("Volume", &demo.rotaryVal, 0.0f, 1.0f, 0.01f);
		sameLine();
		rotarySliderFloat("Pan", &demo.rotaryTwoSide, -1.0f, 1.0f, 0.01f, true);
		sameLine();
		rotarySliderFloat("Speed", &demo.rotaryVal2, 0.0f, 100.0f, 1.0f);

		space();
		rotarySliderFloat("Test", &demo.rotaryVal3, 0.0f, 100.0f, 1.0f, false, 10.0f, RotarySliderFlags::ShowValueInCenter);
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Progress
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Check
	//------------------------------------------------------------------
	if (expandableBegin("Checkboxes", &demo.expandCheck))
	{
		check("Disable##Check", &demo.disableCheck);
		widgetPushDisabled(demo.disableCheck);

		check("Option A (checked)", &demo.checkA);
		check("Option B (unchecked)", &demo.checkB);

		space();
		label("Disabled checkbox:");
		widgetSetNextDisabled();
		bool disabledCheck = true;
		check("Cannot change", &disabledCheck);
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Radio
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Label
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Expandable (nested)
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Tree Nodes (new demo section)
	//------------------------------------------------------------------
	if (expandableBegin("Tree Nodes", &demo.expandTreeNodes))
	{
		check("Disable##TreeNodes", &demo.disableTreeNodes);
		widgetPushDisabled(demo.disableTreeNodes);

		label("Controlled tree (passes booleans to preserve expansion state):");

		// Root node with controlled expansion
		if (treeNodeBegin("Root", &demo.treeRootExpanded))
		{
			// Folder A
			if (treeNodeBegin("Folder A", &demo.treeFolderAExpanded))
			{
				selectable("File A1");
				selectable("File A2");
				treeNodeEnd();
			}

			// Folder B
			if (treeNodeBegin("Folder B", &demo.treeFolderBExpanded))
			{
				selectable("File B1");
				treeNodeEnd();
			}

			// A plain item at root level
			selectable("README.md");

			treeNodeEnd(); // close Root
		}

		space();

		label("Anonymous nodes (internal expansion state):");

		// Use internal expansion state by passing nullptr for expansion var
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

		// Tree nodes with single-click toggle on the label
		label("ToggleOnSelect flag (single-click label toggles):");
		if (treeNodeBegin("Quick Access", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
		{
			if (treeNodeBegin("Documents", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
			{
				selectable("report.pdf");
				selectable("notes.txt");
				treeNodeEnd();
			}

			if (treeNodeBegin("Pictures", nullptr, SelectableFlags::Normal, TreeNodeFlags::ToggleOnSelect))
			{
				selectable("photo.jpg");
				treeNodeEnd();
			}

			treeNodeEnd();
		}

		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Dropdown
	//------------------------------------------------------------------
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

		char buf[64];
		snprintf(buf, sizeof(buf), "Selected index: %d", demo.dropdownSel);
		label(buf);
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// List
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Selectable
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Separators / Spacing
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Tabs
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Box
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Color Picker
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Vec Editors
	//------------------------------------------------------------------
	if (expandableBegin("Vector Editors", &demo.expandVecEditors))
	{
		check("Disable##VecEditors", &demo.disableVecEditors);
		widgetPushDisabled(demo.disableVecEditors);

		label("vec2 (float):");
		vec2Editor("##v2f", demo.vec2x, demo.vec2y);

		space();
		label("vec3 (float):");
		vec3Editor("##v3f", demo.vec3x, demo.vec3y, demo.vec3z);

		space();
		label("vec2 (double):");
		vec2Editor("##v2d", demo.dvec2x, demo.dvec2y);

		space();
		label("vec3 (double):");
		vec3Editor("##v3d", demo.dvec3x, demo.dvec3y, demo.dvec3z);
		widgetPopDisabled();
		expandableEnd();
	}
	//------------------------------------------------------------------
	// Images & Textures
	//------------------------------------------------------------------
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
		image(demo.demoImage, 32);
		sameLine();
		image(demo.demoImage, 32);
		sameLine();
		image(demo.demoImage, 32);

		space();
		label("Texture widget (using color check checkers image as texture):");
		// Just using an image as a texture for demo purposes if no real texture available
		
		texture(themeGetAtlasTexture(), 64, 64, 64);
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Menus
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Viewport
	//------------------------------------------------------------------
	if (expandableBegin("Viewport", &demo.expandViewport))
	{
		check("Disable##Viewport", &demo.disableViewport);
		widgetPushDisabled(demo.disableViewport);

		label("A custom viewport area (100px height):");
		Rect vprect = viewportBegin("##demoViewport", 100);
		// In a real app, you'd use vprect to draw your 3D scene/etc.
		renderDrawSolidRectangle(vprect);
		renderDrawTextInBox("Custom Viewport Content", vprect, HAlignType::Center, VAlignType::Center);
		viewportEnd();
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Custom Widget
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Object Reference
	//------------------------------------------------------------------
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
	//------------------------------------------------------------------
	// Drag & Drop API Demo
	//------------------------------------------------------------------
	if (expandableBegin("Drag & Drop API Demo", &demo.expandDragDrop))
	{
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
	//------------------------------------------------------------------
	// Virtual List
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Scroll View
	//------------------------------------------------------------------
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

	//------------------------------------------------------------------
	// Table
	//------------------------------------------------------------------
	if (expandableBegin("Tables", &demo.expandTable))
	{
		check("Disable##Table", &demo.disableTable);
		widgetPushDisabled(demo.disableTable);

		label("Basic table (3 columns):");
		if (tableBegin("##demoTable", 3, 200, TableFlags::Borders | TableFlags::Resizable | TableFlags::Reorderable | TableFlags::AltRowBg))
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
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Tooltip
	//------------------------------------------------------------------
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
			label("This is a CUSTOM tooltip area");
			label("You can embed any widgets here:");
			check("Check inside tooltip", &demo.checkA);
			button("Button inside tooltip");
			customTooltipEnd();
		}
		widgetPopDisabled();
		expandableEnd();
	}

	//------------------------------------------------------------------
	// Popup
	//------------------------------------------------------------------
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

	demo.demoScrollPos = scrollViewEnd();
}

}
